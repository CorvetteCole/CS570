#include <string>
#include <vector>

class IVirtualDisk {
public:
    virtual ~IVirtualDisk() = default;

    virtual int open(const std::string &user_name, const std::string &file_name) = 0;
    virtual ssize_t read(int file_descriptor, void *buffer, size_t count) = 0;
    virtual ssize_t write(int file_descriptor, const void *buffer, size_t count) = 0;
    virtual off_t seek(int file_descriptor, off_t offset, int whence) = 0;
    virtual int close(int file_descriptor) = 0;
    virtual int remove(const std::string &user_name, const std::string &file_name) = 0;
    virtual std::vector<std::string> list(const std::string &user_name) = 0;
};