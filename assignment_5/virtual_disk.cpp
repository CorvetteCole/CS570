#include "virtual_disk.h"
#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

VirtualDisk::VirtualDisk(const std::string &disk_path)
    : disk_path_(disk_path), disk_fd_(-1), next_fd_(3) {// File descriptors 0, 1, 2 are reserved
    initialize_disk();
}

VirtualDisk::~VirtualDisk() {
    if (disk_fd_ != -1) {
        close(disk_fd_);
    }
}

void VirtualDisk::initialize_disk() {
    // Check if the virtual disk file exists
    struct stat st;
    if (stat(disk_path_.c_str(), &st) == -1) {
        // Virtual disk file does not exist, create it
        disk_fd_ = ::open(disk_path_.c_str(), O_RDWR | O_CREAT, 0600);
        if (disk_fd_ == -1) {
            throw std::runtime_error("Failed to create virtual disk file");
        }
        // Allocate space for the virtual disk
        if (ftruncate(disk_fd_, DISK_CAPACITY) == -1) {
            close(disk_fd_);
            throw std::runtime_error("Failed to allocate space for virtual disk");
        }
    } else {
        // Virtual disk file exists, open it
        disk_fd_ = ::open(disk_path_.c_str(), O_RDWR);
        if (disk_fd_ == -1) {
            throw std::runtime_error("Failed to open virtual disk file");
        }
    }
}

// Helper method to construct file path for a given user and file
std::string VirtualDisk::get_file_path(const std::string &user_name, const std::string &file_name) {
    return disk_path_ + "/" + user_name + "/" + file_name;
}

int VirtualDisk::open(const std::string &user_name, const std::string &file_name) {
    // Check if we have reached the maximum number of open files
    if (file_table_.size() >= MAX_FILES) {
        errno = EMFILE;// Too many open files
        return -1;
    }

    // Make sure the user name and file name are not too long
    if (user_name.length() >= USER_NAME_SIZE || file_name.length() >= FILE_NAME_SIZE) {
        errno = ENAMETOOLONG;// File name too long
        return -1;
    }

    // Construct the full file path within the virtual disk
    std::string file_path = get_file_path(user_name, file_name);

    // Check if the file is already open
    for (const auto &entry: file_table_) {
        if (entry.second.user_name == user_name && entry.second.file_name == file_name) {
            // File is already open, return the existing file descriptor
            return entry.first;
        }
    }

    // Check if the file's metadata exists on the virtual disk
    FileMetadata metadata;
    __off_t current_disk_offset = 0;
    bool file_found = false;
    lseek(disk_fd_, 0, SEEK_SET); // Start from the beginning of the disk
    while (::read(disk_fd_, &metadata, sizeof(metadata)) == sizeof(metadata)) {
        if (strncmp(metadata.file_name, file_name.c_str(), FILE_NAME_SIZE) == 0 &&
            strncmp(metadata.user_name, user_name.c_str(), USER_NAME_SIZE) == 0) {
            // File's metadata found
            file_found = true;
            break;
        }
        // Skip over the file contents to the next metadata entry
        current_disk_offset += sizeof(FileMetadata) + metadata.size;
        lseek(disk_fd_, current_disk_offset, SEEK_SET);
    }

    if (file_found) {
        // File's metadata found, update the file info in the file table
        int fd = next_fd_++;
        FileInfo file_info;
        strncpy(file_info.file_name, metadata.file_name, FILE_NAME_SIZE);
        strncpy(file_info.user_name, metadata.user_name, USER_NAME_SIZE);
        file_info.size = metadata.size;
        file_info.disk_offset = current_disk_offset;
        file_table_[fd] = file_info;
        return fd;
    } else {
        // File's metadata not found, create a new file descriptor and file info
        int fd = next_fd_++;
        FileInfo file_info;
        strncpy(file_info.file_name, file_name.c_str(), FILE_NAME_SIZE);
        strncpy(file_info.user_name, user_name.c_str(), USER_NAME_SIZE);
        file_info.size = DEFAULT_FILE_SIZE;
        file_info.disk_offset = current_disk_offset;
        file_info.current_position = 0;// Start at the beginning of the file
        file_table_[fd] = file_info;

        // Write new FileMetadata struct to the virtual disk
        FileMetadata new_metadata;
        strncpy(new_metadata.file_name, file_name.c_str(), FILE_NAME_SIZE);
        strncpy(new_metadata.user_name, user_name.c_str(), USER_NAME_SIZE);
        new_metadata.size = DEFAULT_FILE_SIZE;
        lseek(disk_fd_, current_disk_offset, SEEK_SET);
        ::write(disk_fd_, &new_metadata, sizeof(new_metadata));

        // Allocate space for the new file
        lseek(disk_fd_, current_disk_offset + sizeof(FileMetadata) + DEFAULT_FILE_SIZE - 1, SEEK_SET);
        ::write(disk_fd_, "", 1);

        return fd;
    }
}

ssize_t VirtualDisk::read(int file_descriptor, void *buffer, size_t count) {
    // Check if the file descriptor is valid
    auto it = file_table_.find(file_descriptor);
    if (it == file_table_.end()) {
        errno = EBADF;// Bad file descriptor
        return -1;
    }

    FileInfo &file_info = it->second;

    // Make sure the file has enough data for the read
    if (file_info.current_position + count > file_info.size) {
        // file is not big enough for the read
        errno = ENODATA;// No data available
        return -1;
    }

    // Seek to the current position of the file
    if (lseek(disk_fd_, file_info.disk_offset + sizeof(FileMetadata) + file_info.current_position, SEEK_SET) == (off_t) -1) {
        return -1;// lseek failed
    }

    // Read the data from the virtual disk file
    ssize_t bytes_read = ::read(disk_fd_, buffer, count);
    if (bytes_read == -1) {
        return -1;// read failed
    }

    // Update the current position
    file_info.current_position += bytes_read;

    return bytes_read;
}

ssize_t VirtualDisk::write(int file_descriptor, const void *buffer, size_t count) {
    // Check if the file descriptor is valid
    auto it = file_table_.find(file_descriptor);
    if (it == file_table_.end()) {
        errno = EBADF; // Bad file descriptor
        return -1;
    }

    FileInfo &file_info = it->second;

    // make sure the file has enough space for the write
    if (file_info.current_position + count > file_info.size) {
        errno = ENOSPC;// No space left on device
        return -1;
    }

    // Seek to the current position of the file
    if (lseek(disk_fd_, file_info.disk_offset + sizeof(FileMetadata) + file_info.current_position, SEEK_SET) == (off_t) -1) {
        return -1;// lseek failed
    }

    // Write the data to the virtual disk file
    ssize_t bytes_written = ::write(disk_fd_, buffer, count);
    if (bytes_written == -1) {
        return -1;// write failed
    }

    // Update the current position
    file_info.current_position += bytes_written;

    return bytes_written;
}

off_t VirtualDisk::seek(int file_descriptor, off_t offset, int whence) {
    // Check if the file descriptor is valid
    auto it = file_table_.find(file_descriptor);
    if (it == file_table_.end()) {
        errno = EBADF;// Bad file descriptor
        return -1;
    }

    FileInfo &file_info = it->second;
    off_t new_position = 0;

    // Determine the new position based on the 'whence' parameter
    switch (whence) {
        case SEEK_SET:
            new_position = offset;
            break;
        case SEEK_CUR:
            new_position = file_info.current_position + offset;
            break;
        case SEEK_END:
            new_position = file_info.current_position + file_info.size;
            break;
        default:
            errno = EINVAL;// Invalid argument
            return -1;
    }

    // Check for negative position
    if (new_position < 0) {
        errno = EINVAL;
        return -1;
    }

    // make sure new position is actually possible
    if (new_position > file_info.size) {
        errno = ENOSPC;// No space left on device
        return -1;
    }

    // Update the current position
    file_info.current_position = new_position;

    return new_position;
}

int VirtualDisk::close(int file_descriptor) {
    // Check if the file descriptor is valid and exists in the file table
    auto it = file_table_.find(file_descriptor);
    if (it == file_table_.end()) {
        errno = EBADF;// Bad file descriptor
        return -1;
    }

    // Remove the file descriptor from the file table
    file_table_.erase(it);
    return 0;// Success
}
int VirtualDisk::remove(const std::string &user_name, const std::string &file_name) {
    // Iterate through the file table to find the file
    for (auto it = file_table_.begin(); it != file_table_.end(); ++it) {
        if (it->second.user_name == user_name && it->second.file_name == file_name) {
            // delete metadata from disk, shift rest of disk to fill gap
            auto file_info = it->second;
            // metadata is at file_info.disk_offset
            // file contents are at file_info.disk_offset + sizeof(FileMetadata)





            file_table_.erase(it);





            return 0;// Success
        }
    }
    // File not found
    errno = ENOENT;// No such file or directory
    return -1;
}
std::vector<std::string> VirtualDisk::list(const std::string &user_name) {
    std::vector<std::string> file_list;
    for (const auto &entry: file_table_) {
        if (entry.second.user_name == user_name) {
            file_list.emplace_back(entry.second.file_name);
        }
    }
    return file_list;
}
