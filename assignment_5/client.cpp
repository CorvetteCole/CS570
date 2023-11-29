#include <pwd.h>
#include <rpc/rpc.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "ssnfs.h"

CLIENT *clnt;

void ssnfsprog_1(char *host) {
    clnt = clnt_create(host, SSNFSPROG, SSNFSVER, "udp");
    if (clnt == NULL) {
        clnt_pcreateerror(host);
        exit(1);
    }
}

int Open(char *filename_to_open) {
    open_output *result_1;
    open_input open_file_1_arg;
    strcpy(open_file_1_arg.user_name, (getpwuid(getuid()))->pw_name);
    strcpy(open_file_1_arg.file_name, filename_to_open);
    result_1 = open_file_1(&open_file_1_arg, clnt);
    if (result_1 == (open_output *) NULL) {
        clnt_perror(clnt, "call failed");
    }
    printf(" File name is %s\n", (*result_1).out_msg.out_msg_val);
    return ((*result_1).fd);
}


/*

	read_output  *result_2;
	read_input  read_file_1_arg;
	write_output  *result_3;
	write_input  write_file_1_arg;
	list_output  *result_4;
	list_input  list_files_1_arg;
	delete_output  *result_5;
	delete_input  delete_file_1_arg;
	close_output  *result_6;
	close_input  close_file_1_arg;
        seek_output * result_7;
        seek_input seek_position_1_arg;


	result_2 = read_file_1(&read_file_1_arg, clnt);
	if (result_2 == (read_output *) NULL) {
		clnt_perror (clnt, "call failed");
	}
	result_3 = write_file_1(&write_file_1_arg, clnt);
	if (result_3 == (write_output *) NULL) {
		clnt_perror (clnt, "call failed");
	}
	result_4 = list_files_1(&list_files_1_arg, clnt);
	if (result_4 == (list_output *) NULL) {
		clnt_perror (clnt, "call failed");
	}
	result_5 = delete_file_1(&delete_file_1_arg, clnt);
	if (result_5 == (delete_output *) NULL) {
		clnt_perror (clnt, "call failed");
	}
	result_6 = close_file_1(&close_file_1_arg, clnt);
	if (result_6 == (close_output *) NULL) {
		clnt_perror (clnt, "call failed");
	}
       	result_7 = seek_position_1(&seek_position_1_arg, clnt);
	if (result_7 == (seek_output *) NULL) {
		clnt_perror (clnt, "call failed");
	}


*/

int Write(int fd, const char *data, int numbytes) {
    write_output *result_3;
    write_input write_file_1_arg;

    // Set the user name from the current user's information
    strcpy(write_file_1_arg.user_name, (getpwuid(getuid()))->pw_name);
    write_file_1_arg.fd = fd;
    write_file_1_arg.numbytes = numbytes;
    write_file_1_arg.buffer.buffer_val = (char *) data;
    write_file_1_arg.buffer.buffer_len = numbytes;

    result_3 = write_file_1(&write_file_1_arg, clnt);
    if (result_3 == (write_output *) NULL) {
        clnt_perror(clnt, "call failed");
        return -1;// Indicate error
    }
    if (result_3->success == 0) {
        // Write operation was not successful
        fprintf(stderr, "Write error: %s\n", result_3->out_msg.out_msg_val);
        return -1;// Indicate error
    }

    // Write operation was successful, return the number of bytes written
    return numbytes;
}

int Close(int fd) {
    close_output *result_6;
    close_input close_file_1_arg;

    // Set the user name from the current user's information
    strcpy(close_file_1_arg.user_name, (getpwuid(getuid()))->pw_name);
    close_file_1_arg.fd = fd;

    result_6 = close_file_1(&close_file_1_arg, clnt);
    if (result_6 == (close_output *) NULL) {
        clnt_perror(clnt, "call failed");
        return -1;// Indicate error
    }

    // Check if the close operation was successful
    if (result_6->out_msg.out_msg_val == NULL || strcmp(result_6->out_msg.out_msg_val, "") == 0) {
        // Close operation failed, handle the error
        fprintf(stderr, "Close error: %s\n", result_6->out_msg.out_msg_val);
        return -1;// Indicate error
    }

    // Close operation was successful
    return 0;
}

int Seek(int fd, int position) {
    // ... (existing Seek function code remains unchanged)
}

int Delete(const char *filename) {
    delete_output *result_5;
    delete_input delete_file_1_arg;

    // Set the user name from the current user's information
    strcpy(delete_file_1_arg.user_name, (getpwuid(getuid()))->pw_name);
    strcpy(delete_file_1_arg.file_name, filename);

    result_5 = delete_file_1(&delete_file_1_arg, clnt);
    if (result_5 == (delete_output *) NULL) {
        clnt_perror(clnt, "call failed");
        return -1;// Indicate error
    }

    // Check if the delete operation was successful
    if (result_5->out_msg.out_msg_val == NULL || strcmp(result_5->out_msg.out_msg_val, "") == 0) {
        // Delete operation failed, handle the error
        fprintf(stderr, "Delete error: %s\n", result_5->out_msg.out_msg_val);
        return -1;// Indicate error
    }

    // Delete operation was successful
    return 0;
}

int Read(int fd, char *buffer, int numbytes) {
    read_output *result_2;
    read_input read_file_1_arg;

    // Set the user name from the current user's information
    strcpy(read_file_1_arg.user_name, (getpwuid(getuid()))->pw_name);
    read_file_1_arg.fd = fd;
    read_file_1_arg.numbytes = numbytes;

    result_2 = read_file_1(&read_file_1_arg, clnt);
    if (result_2 == (read_output *) NULL) {
        clnt_perror(clnt, "call failed");
        return -1;// Indicate error
    }

    // Check if the read operation was successful
    if (result_2->success == 0) {
        // Read operation failed, handle the error
        fprintf(stderr, "Read error: %s\n", result_2->out_msg.out_msg_val);
        return -1;// Indicate error
    }

    // Read operation was successful, copy the data to the buffer
    memcpy(buffer, result_2->buffer.buffer_val, result_2->buffer.buffer_len);

    // Return the number of bytes read
    return result_2->buffer.buffer_len;
}

void List() {
    list_output *result_4;
    list_input list_files_1_arg;

    // Set the user name from the current user's information
    strcpy(list_files_1_arg.user_name, (getpwuid(getuid()))->pw_name);

    result_4 = list_files_1(&list_files_1_arg, clnt);
    if (result_4 == (list_output *) NULL) {
        clnt_perror(clnt, "call failed");
    } else {
        // Print the list of files
        printf("List of files:\n%s", result_4->out_msg.out_msg_val);
    }
}

int main(int argc, char *argv[]) {
    char *host;

    if (argc < 2) {
        printf("usage: %s server_host\n", argv[0]);
        exit(1);
    }
    host = argv[1];
    ssnfsprog_1(host);
    int fd;
    fd = Open("MyFile");
    printf("File descripter retuned is %d \n", fd);

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
