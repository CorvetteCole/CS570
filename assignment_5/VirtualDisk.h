#ifndef VIRTUAL_DISK_H
#define VIRTUAL_DISK_H

#include "IVirtualDisk.h"
#include "ssnfs.h"
#include <string>
#include <unordered_map>
#include <vector>

// Define constants for the virtual disk
const size_t DISK_CAPACITY = 16 * 1024 * 1024;// 16MB
const size_t MAX_FILES = 20;                  // Maximum number of open files
const size_t DEFAULT_FILE_SIZE = 32768;       // 32KB

// Structure to hold file information
struct FileInfo {
    char file_name[FILE_NAME_SIZE];
    char user_name[USER_NAME_SIZE];
    __off_t current_position;// relative to the start of the actual data
    size_t size;
    __off_t disk_offset;
};

struct FileMetadata {
    char file_name[FILE_NAME_SIZE];
    char user_name[USER_NAME_SIZE];
    size_t size;
};


class VirtualDisk : public IVirtualDisk {
public:
    explicit VirtualDisk(const std::string &disk_path);
    ~VirtualDisk() override;

    // File operations
    int open(const std::string &user_name, const std::string &file_name) override;
    ssize_t read(int file_descriptor, void *buffer, size_t count) override;
    ssize_t write(int file_descriptor, const void *buffer, size_t count) override;
    off_t seek(int file_descriptor, off_t offset, int whence) override;
    int close(int file_descriptor) override;
    int remove(const std::string &user_name, const std::string &file_name) override;

    // Directory operations
    std::vector<std::string> list(const std::string &user_name) override;

private:
    std::string disk_path_;
    int disk_fd_;
    std::unordered_map<int, FileInfo> file_table_;
    int next_fd_;

    // Helper methods
    void initialize_disk();
    std::string get_file_path(const std::string &user_name, const std::string &file_name);
};

#endif// VIRTUAL_DISK_H
