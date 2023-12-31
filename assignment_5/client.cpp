#include <pwd.h>
#include <rpc/rpc.h>
#include <cstdio>
#include <cstdlib>
#include <unistd.h>

#include "ssnfs.h"

CLIENT *clnt;

void ssnfsprog_1(char *host) {
    clnt = clnt_create(host, SSNFSPROG, SSNFSVER, "udp");
    if (clnt == nullptr) {
        clnt_pcreateerror(host);
        exit(1);
    }
}

int Open(const char *filename) {
    open_output *result;
    open_input open_file_arg;
    strcpy(open_file_arg.user_name, (getpwuid(getuid()))->pw_name);
    strcpy(open_file_arg.file_name, filename);
    result = open_file_1(&open_file_arg, clnt);
    if (result == nullptr) {
        fprintf(stderr, "Open error: RPC call failed\n");
        return -1;// Indicate error
    }
    if (result->fd == -1) {
        fprintf(stderr, "Open error: %s\n", result->out_msg.out_msg_val);
        return -1;// Indicate error
    }
    return result->fd;
}


int Write(int fd, const char *data, int numbytes) {
    write_output *result;
    write_input write_file_arg;

    // Set the username from the current user's information
    strcpy(write_file_arg.user_name, (getpwuid(getuid()))->pw_name);
    write_file_arg.fd = fd;
    write_file_arg.numbytes = numbytes;

    // Allocate memory for the buffer to avoid using the pointer to potentially temporary data
    write_file_arg.buffer.buffer_val = static_cast<char *>(malloc(numbytes));
    if (write_file_arg.buffer.buffer_val == nullptr) {
        fprintf(stderr, "Write error: Memory allocation for buffer failed\n");
        return -1;// Indicate error
    }
    // Copy the data into the newly allocated buffer
    memcpy(write_file_arg.buffer.buffer_val, data, numbytes);
    write_file_arg.buffer.buffer_len = numbytes;

    // Perform the RPC call
    result = write_file_1(&write_file_arg, clnt);

    // Check if the RPC call failed
    if (result == nullptr) {
        clnt_perror(clnt, "Write RPC call failed");
        return -1;// Indicate error
    }

    // Check if the write operation was not successful
    if (result->success == 0) {
        fprintf(stderr, "Write error: %s\n", result->out_msg.out_msg_val);
        return -1;// Indicate error
    }

    // Write operation was successful, return the number of bytes written
    return numbytes;
}

int Close(int fd) {
    close_output *result;
    close_input close_file_arg;

    // Set the username from the current user's information
    strcpy(close_file_arg.user_name, (getpwuid(getuid()))->pw_name);
    close_file_arg.fd = fd;

    result = close_file_1(&close_file_arg, clnt);
    if (result == nullptr) {
        clnt_perror(clnt, "call failed");
        return -1;// Indicate error
    }

    // Check if the close operation was successful
    if (result->out_msg.out_msg_len != 0) {
        // Close operation failed, handle the error
        fprintf(stderr, "Close error: %s\n", result->out_msg.out_msg_val);
        return -1;// Indicate error
    }

    // Close operation was successful
    return 0;
}

int Seek(int fd, int position) {
    seek_output *result;
    seek_input seek_position_arg;

    // Set the username from the current user's information
    strcpy(seek_position_arg.user_name, (getpwuid(getuid()))->pw_name);
    seek_position_arg.fd = fd;
    seek_position_arg.position = position;

    result = seek_position_1(&seek_position_arg, clnt);
    if (result == nullptr) {
        clnt_perror(clnt, "call failed");
        return -1;// Indicate error
    }

    // Check if the seek operation was successful
    if (result->success == 0) {
        // Seek operation failed, handle the error
        fprintf(stderr, "Seek error: %s\n", result->out_msg.out_msg_val);
        return -1;// Indicate error
    }

    // Seek operation was successful
    return 0;
}

int Delete(const char *filename) {
    delete_output *result;
    delete_input delete_file_arg;

    // Set the username from the current user's information
    strcpy(delete_file_arg.user_name, (getpwuid(getuid()))->pw_name);
    strcpy(delete_file_arg.file_name, filename);

    result = delete_file_1(&delete_file_arg, clnt);
    if (result == nullptr) {
        clnt_perror(clnt, "call failed");
        return -1;// Indicate error
    }

    // Check if the delete operation was successful
    if (result->out_msg.out_msg_len != 0) {
        // Delete operation failed, handle the error
        fprintf(stderr, "Delete error: %s\n", result->out_msg.out_msg_val);
        return -1;// Indicate error
    }

    // Delete operation was successful
    return 0;
}

u_int Read(int fd, char *buffer, int numbytes) {
    read_output *result;
    read_input read_file_arg;

    // Set the username from the current user's information
    strcpy(read_file_arg.user_name, (getpwuid(getuid()))->pw_name);
    read_file_arg.fd = fd;
    read_file_arg.numbytes = numbytes;

    result = read_file_1(&read_file_arg, clnt);
    if (result == nullptr) {
        clnt_perror(clnt, "call failed");
        return -1;// Indicate error
    }

    // Check if the read operation was successful
    if (result->success == 0) {
        // Read operation failed, handle the error
        fprintf(stderr, "Read error: %s\n", result->out_msg.out_msg_val);
        return -1;// Indicate error
    }

    // Ensure buffer is large enough and zero-initialized
    memset(buffer, 0, numbytes + 1);

    // Read operation was successful, copy the data to the buffer
    memcpy(buffer, result->buffer.buffer_val, result->buffer.buffer_len);

    // Null-terminate the buffer
    buffer[result->buffer.buffer_len] = '\0';

    // Return the number of bytes read
    return result->buffer.buffer_len;
}

void List() {
    list_output *result;
    list_input list_files_arg;

    // Set the username from the current user's information
    strcpy(list_files_arg.user_name, (getpwuid(getuid()))->pw_name);

    result = list_files_1(&list_files_arg, clnt);
    if (result == nullptr) {
        clnt_perror(clnt, "call failed");
        return;
    }

    // Print the list of files
    printf("List of files:\n%s", result->out_msg.out_msg_val);
}

int main(int argc, char *argv[]) {
    char *host;

    if (argc < 2) {
        printf("usage: %s server_host\n", argv[0]);
        exit(1);
    }
    host = argv[1];
    ssnfsprog_1(host);

    int i, j;
    int fd1, fd2;
    char buffer[100];
    fd1 = Open("File1");// opens the file "File1"
    for (i = 0; i < 20; i++) {
        Write(fd1, "This is a test program for cs570 assignment 4", 15);
    }
    Close(fd1);
    fd2 = Open("File1");
    for (j = 0; j < 20; j++) {
        Read(fd2, buffer, 10);
        printf("%s\n", buffer);
    }
    Seek(fd2, 40);
    Read(fd2, buffer, 20);
    printf("%s\n", buffer);
    Close(fd2);
    Delete("File1");
    List();


    exit(0);
}
