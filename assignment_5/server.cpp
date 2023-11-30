#include "VirtualDisk.h"
extern "C" {
#include "ssnfs.h"
}

#include <cerrno>
#include <fcntl.h>
#include <iostream>

// Create a global VirtualDisk object
VirtualDisk virtualDisk("./virtual_fs");

open_output *
open_file_1_svc(open_input *argp, struct svc_req *rqstp) {
    static open_output result;

    // Initialize output message
    result.out_msg.out_msg_len = 0;
    result.out_msg.out_msg_val = nullptr;

    // Use VirtualDisk to open the file
    int fd = virtualDisk.open(argp->user_name, argp->file_name);
    if (fd == -1) {
        // Failed to open or create file
        result.fd = -1;
        const char *error_message = strerror(errno);
        result.out_msg.out_msg_val = strdup(error_message);
        result.out_msg.out_msg_len = strlen(error_message) + 1;
        return &result;
    }

    // File opened successfully, set output message
    result.fd = fd;
    result.out_msg.out_msg_len = strlen(argp->file_name) + 1;
    result.out_msg.out_msg_val = strdup(argp->file_name);

    // Set output message
    result.out_msg.out_msg_len = strlen(argp->file_name) + 1;
    result.out_msg.out_msg_val = (char *) malloc(result.out_msg.out_msg_len);
    strcpy(result.out_msg.out_msg_val, argp->file_name);

    return &result;
}

read_output *
read_file_1_svc(read_input *argp, struct svc_req *rqstp) {
    static read_output result;
    int fd = argp->fd;
    int numbytes = argp->numbytes;

    // Reset the result structure
    result.out_msg.out_msg_val = nullptr;
    result.out_msg.out_msg_len = 0;
    result.success = 0;// Assume failure by default
    result.buffer.buffer_len = 0;
    result.buffer.buffer_val = nullptr;

    // Attempt to read from the file using VirtualDisk
    char *read_buffer = new char[numbytes];
    ssize_t bytes_read = virtualDisk.read(fd, read_buffer, numbytes);
    if (bytes_read < 0) {
        // Read failed, handle the error
        delete[] read_buffer;
        // get the error from the errno
        const char *error_message = strerror(errno);
        result.out_msg.out_msg_val = strdup(error_message);
        result.out_msg.out_msg_len = strlen(error_message) + 1;
    } else {
        // Read was successful
        result.buffer.buffer_val = read_buffer;
        result.buffer.buffer_len = bytes_read;
        result.success = 1;
    }

    return &result;
}

write_output *
write_file_1_svc(write_input *argp, struct svc_req *rqstp) {
    static write_output result;
    int fd = argp->fd;
    int numbytes = argp->numbytes;
    char *buffer = argp->buffer.buffer_val;

    // Initialize output message
    result.success = 0;// Assume failure
    result.out_msg.out_msg_len = 0;
    result.out_msg.out_msg_val = nullptr;

    std::cout << "writing buffer: " << buffer << std::endl;

    // Attempt to write to the file using VirtualDisk
    ssize_t bytes_written = virtualDisk.write(fd, buffer, numbytes);

    std::cout << "bytes_written: " << bytes_written << std::endl;

    if (bytes_written < 0) {
        // Write failed, handle the error
        const char *error_message = strerror(errno);
        result.out_msg.out_msg_val = strdup(error_message);
        result.out_msg.out_msg_len = strlen(error_message) + 1;
        return &result;
    }

    // Write was successful
    result.success = 1;
    return &result;
}

list_output *
list_files_1_svc(list_input *argp, struct svc_req *rqstp) {
    static list_output result;

    // Initialize output message
    result.out_msg.out_msg_len = 0;
    result.out_msg.out_msg_val = nullptr;

    // Use VirtualDisk to list the files
    std::vector<std::string> files = virtualDisk.list(argp->user_name);
    std::string file_list_str;
    for (const auto &file_name: files) {
        file_list_str += file_name + "\n";
    }

    // Allocate memory for the file list
    char *file_list = strdup(file_list_str.c_str());

    // Set the file list in the result
    result.out_msg.out_msg_val = file_list;
    result.out_msg.out_msg_len = strlen(file_list) + 1;

    return &result;
}

delete_output *
delete_file_1_svc(delete_input *argp, struct svc_req *rqstp) {
    static delete_output result;

    // Initialize output message
    result.out_msg.out_msg_len = 0;
    result.out_msg.out_msg_val = nullptr;

    // Use VirtualDisk to delete the file
    int status = virtualDisk.remove(argp->user_name, argp->file_name);
    if (status == -1) {
        const char *error_message = strerror(errno);
        result.out_msg.out_msg_val = strdup(error_message);
        result.out_msg.out_msg_len = strlen(error_message) + 1;
        return &result;
    }

    return &result;
}

close_output *
close_file_1_svc(close_input *argp, struct svc_req *rqstp) {
    static close_output result;
    int fd = argp->fd;

    // Initialize output message
    result.out_msg.out_msg_len = 0;
    result.out_msg.out_msg_val = nullptr;

    // Attempt to close the file using VirtualDisk
    if (virtualDisk.close(fd) == -1) {
        // Close failed, handle the error
        const char *error_message = strerror(errno);
        result.out_msg.out_msg_val = strdup(error_message);
        result.out_msg.out_msg_len = strlen(error_message) + 1;
        return &result;
    }

    return &result;
}


seek_output *
seek_position_1_svc(seek_input *argp, struct svc_req *rqstp) {
    static seek_output result;
    int fd = argp->fd;
    off_t position = argp->position;
    off_t new_position;

    // Initialize output message
    result.success = 0;// Assume failure
    result.out_msg.out_msg_len = 0;
    result.out_msg.out_msg_val = nullptr;

    // Attempt to seek to the specified position using VirtualDisk
    new_position = virtualDisk.seek(fd, position, SEEK_SET);
    if (new_position == (off_t) -1) {
        // Seek failed, handle the error
        const char *error_message = strerror(errno);
        result.out_msg.out_msg_val = strdup(error_message);
        result.out_msg.out_msg_len = strlen(error_message) + 1;
        return &result;
    }

    // Seek was successful
    result.success = 1;

    return &result;
}
