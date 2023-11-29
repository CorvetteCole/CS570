#include "../VirtualDisk.h"
#include <cstdio>
#include <fstream>
#include <gtest/gtest.h>
#include <string>

class VirtualDiskTest : public ::testing::Test {
protected:
    VirtualDisk *virtualDisk;
    const std::string diskPath = "./test_virtual_fs";
                                                                                                                                 
    void SetUp() override {
        // Setup code before each test...
        virtualDisk = new VirtualDisk(diskPath);
    }
                                                                                                                                 
    void TearDown() override {
        // Clean up code after each test...
        delete virtualDisk;
        // Optionally, remove the test virtual disk file if it was created
        std::remove(diskPath.c_str());
    }
};
                                                                                                                                 
TEST_F(VirtualDiskTest, OpenNewFile) {
    // Test opening a new file
    int fd = virtualDisk->open("user", "newfile");
    ASSERT_GT(fd, 0); // File descriptor should be greater than 0
    virtualDisk->close(fd);
}
                                                                                                                                 
TEST_F(VirtualDiskTest, OpenExistingFile) {
    // Test opening an existing file
    int fd = virtualDisk->open("user", "existingfile");
    ASSERT_GT(fd, 0);
    virtualDisk->close(fd);
    // Open again to ensure it can be reopened
    int fd2 = virtualDisk->open("user", "existingfile");
    ASSERT_GT(fd2, 0);
    virtualDisk->close(fd2);
}
                                                                                                                                 
TEST_F(VirtualDiskTest, ReadWriteFile) {
    // Test writing to and reading from a file
    const std::string content = "Hello, Virtual Disk!";
    char buffer[1024] = {0};
    int fd = virtualDisk->open("user", "file");
    ASSERT_GT(fd, 0);

    // Write content to the file
    ssize_t bytes_written = virtualDisk->write(fd, content.c_str(), content.size());
    ASSERT_EQ(bytes_written, content.size()) << "Write operation failed or did not write the expected number of bytes.";

    // Reset file position to the beginning of the file
    off_t new_pos = virtualDisk->seek(fd, 0, SEEK_SET);
    ASSERT_EQ(new_pos, 0) << "Seek operation failed to reset file position.";

    // Read content from the file
    ssize_t bytes_read = virtualDisk->read(fd, buffer, content.size());
    ASSERT_EQ(bytes_read, content.size()) << "Read operation failed or did not read the expected number of bytes.";

    // Check if the read content matches the written content
    buffer[bytes_read] = '\0'; // Null-terminate the buffer
    ASSERT_STREQ(buffer, content.c_str()) << "Content read from disk does not match content written.";

    // Close the file
    int status = virtualDisk->close(fd);
    ASSERT_EQ(status, 0) << "Close operation failed.";
}
                                                                                                                                 
TEST_F(VirtualDiskTest, SeekFile) {
    // Test seeking within a file
    const std::string content = "Seek within this file.";
    int fd = virtualDisk->open("user", "seekfile");
    ASSERT_GT(fd, 0);
                                                                                                                                 
    virtualDisk->write(fd, content.c_str(), content.size());
                                                                                                                                 
    off_t new_pos = virtualDisk->seek(fd, 5, SEEK_SET);
    ASSERT_EQ(new_pos, 5);
                                                                                                                                 
    char buffer[1024] = {0};
    ssize_t bytes_read = virtualDisk->read(fd, buffer, content.size() - 5);
    ASSERT_EQ(bytes_read, content.size() - 5);
    ASSERT_STREQ(buffer, content.c_str() + 5);
                                                                                                                                 
    virtualDisk->close(fd);
}
                                                                                                                                 
TEST_F(VirtualDiskTest, RemoveFile) {
    // Test removing a file
    int fd = virtualDisk->open("user", "filetoremove");
    ASSERT_GT(fd, 0);
    virtualDisk->close(fd);
                                                                                                                                 
    int status = virtualDisk->remove("user", "filetoremove");
    ASSERT_EQ(status, 0);
}
                                                                                                                                 
TEST_F(VirtualDiskTest, OpenMaxFiles) {
    // Test opening files up to the maximum limit
    for (size_t i = 0; i < MAX_FILES; ++i) {
        std::string file_name = "file" + std::to_string(i);
        int fd = virtualDisk->open("user", file_name);
        ASSERT_GT(fd, 0) << "Failed to open file: " << file_name;
    }
    // Attempt to open one more file beyond the limit
    int fd = virtualDisk->open("user", "extrafile");
    ASSERT_EQ(fd, -1);
    ASSERT_EQ(errno, EMFILE);
}

TEST_F(VirtualDiskTest, OpenWithLongFileName) {
    // Test opening a file with a name that exceeds the maximum length
    std::string long_file_name(FILE_NAME_SIZE + 1, 'a');
    int fd = virtualDisk->open("user", long_file_name);
    ASSERT_EQ(fd, -1);
    ASSERT_EQ(errno, ENAMETOOLONG);
}

TEST_F(VirtualDiskTest, ReadExceedFileSize) {
    // Test reading more bytes than the file size
    const std::string content = "Short content";
    int fd = virtualDisk->open("user", "shortfile");
    ASSERT_GT(fd, 0);
    virtualDisk->write(fd, content.c_str(), content.size());
    char buffer[1024] = {0};
    ssize_t bytes_read = virtualDisk->read(fd, buffer, sizeof(buffer)); // Read more than the content size
    ASSERT_EQ(bytes_read, -1);
    ASSERT_EQ(errno, ENODATA);
    virtualDisk->close(fd);
}

TEST_F(VirtualDiskTest, WriteExceedFileSize) {
    // Test writing more bytes than the file size allows
    std::string large_content(DEFAULT_FILE_SIZE + 1, 'a');
    int fd = virtualDisk->open("user", "largefile");
    ASSERT_GT(fd, 0);
    ssize_t bytes_written = virtualDisk->write(fd, large_content.c_str(), large_content.size());
    ASSERT_EQ(bytes_written, -1);
    ASSERT_EQ(errno, ENOSPC);
    virtualDisk->close(fd);
}

TEST_F(VirtualDiskTest, SeekBeyondFileSize) {
    // Test seeking beyond the file size
    const std::string content = "Content";
    int fd = virtualDisk->open("user", "seektest");
    ASSERT_GT(fd, 0);
    virtualDisk->write(fd, content.c_str(), content.size());
    off_t new_pos = virtualDisk->seek(fd, DEFAULT_FILE_SIZE + 1, SEEK_SET); // Seek beyond the file size
    ASSERT_EQ(new_pos, -1);
    ASSERT_EQ(errno, ENOSPC);
    virtualDisk->close(fd);
}

TEST_F(VirtualDiskTest, SeekWithInvalidWhence) {
    // Test seeking with an invalid 'whence' parameter
    int fd = virtualDisk->open("user", "seektest");
    ASSERT_GT(fd, 0);
    off_t new_pos = virtualDisk->seek(fd, 0, -1); // Invalid 'whence'
    ASSERT_EQ(new_pos, -1);
    ASSERT_EQ(errno, EINVAL);
    virtualDisk->close(fd);
}

TEST_F(VirtualDiskTest, CloseInvalidFileDescriptor) {
    // Test closing an invalid file descriptor
    int status = virtualDisk->close(-1); // Invalid file descriptor
    ASSERT_EQ(status, -1);
    ASSERT_EQ(errno, EBADF);
}

TEST_F(VirtualDiskTest, RemoveNonExistentFile) {
    // Test removing a file that does not exist
    int status = virtualDisk->remove("user", "nonexistentfile");
    ASSERT_EQ(status, -1);
    ASSERT_EQ(errno, ENOENT);
}

TEST_F(VirtualDiskTest, ListFilesForUserWithNoFiles) {
    // Test listing files for a user with no files
    std::vector<std::string> files = virtualDisk->list("emptyuser");
    ASSERT_TRUE(files.empty());
}
