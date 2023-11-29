#ifndef VIRTUAL_DISK_H
#define VIRTUAL_DISK_H

#include <string>
#include <unordered_map>
#include <vector>

// Define constants for the virtual disk
const size_t BLOCK_SIZE = 512; // Block size in bytes
const size_t DISK_CAPACITY = 16 * 1024 * 1024; // 16MB
const size_t MAX_FILES = 20; // Maximum number of open files
const size_t METADATA_BLOCKS = 32; // Number of blocks reserved for metadata

// Structure to hold file information
struct FileInfo {
    std::string user_name;
    std::string file_name;
    __off_t current_position;
};

class VirtualDisk {
public:
    VirtualDisk(const std::string &disk_path);
    ~VirtualDisk();

    // File operations
    int open(const std::string &user_name, const std::string &file_name);
    ssize_t read(int file_descriptor, void *buffer, size_t count);
    ssize_t write(int file_descriptor, const void *buffer, size_t count);
    off_t seek(int file_descriptor, off_t offset, int whence);
    int close(int file_descriptor);
    int remove(const std::string &user_name, const std::string &file_name);

    // Directory operations
    std::vector<std::string> list_files(const std::string &user_name);

private:
    std::string disk_path_;
    int disk_fd_;
    std::unordered_map<int, FileInfo> file_table_;
    int next_fd_;

    // Helper methods
    void initialize_disk();
    std::string get_file_path(const std::string &user_name, const std::string &file_name);
};

#endif // VIRTUAL_DISK_H