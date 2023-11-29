#include "../virtual_disk.h"
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
                                                                                                                                 
    ssize_t bytes_written = virtualDisk->write(fd, content.c_str(), content.size());
    ASSERT_EQ(bytes_written, content.size());
                                                                                                                                 
    ssize_t bytes_read = virtualDisk->read(fd, buffer, content.size());
    ASSERT_EQ(bytes_read, content.size());
    ASSERT_STREQ(buffer, content.c_str());
                                                                                                                                 
    virtualDisk->close(fd);
}
                                                                                                                                 
TEST_F(VirtualDiskTest, WriteBeyondFileSize) {
    // Test writing beyond the file size limit
    const std::string content = "This content exceeds the default file size limit.";
    int fd = virtualDisk->open("user", "bigfile");
    ASSERT_GT(fd, 0);
                                                                                                                                 
    ssize_t bytes_written = virtualDisk->write(fd, content.c_str(), content.size());
    ASSERT_EQ(bytes_written, -1); // Should fail as it exceeds the file size
                                                                                                                                 
    virtualDisk->close(fd);
}
                                                                                                                                 
TEST_F(VirtualDiskTest, ReadNonExistentFile) {
    // Test reading from a non-existent file
    char buffer[1024] = {0};
    int fd = virtualDisk->open("user", "nonexistentfile");
    ASSERT_GT(fd, 0);
                                                                                                                                 
    ssize_t bytes_read = virtualDisk->read(fd, buffer, sizeof(buffer));
    ASSERT_EQ(bytes_read, -1); // Should fail as the file does not exist
                                                                                                                                 
    virtualDisk->close(fd);
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
                                                                                                                                 
TEST_F(VirtualDiskTest, ListFiles) {
    // Test listing files
    virtualDisk->open("user", "file1");
    virtualDisk->open("user", "file2");
    virtualDisk->open("user", "file3");
                                                                                                                                 
    std::vector<std::string> files = virtualDisk->list("user");
    ASSERT_EQ(files.size(), 3);
}