const USER_NAME_SIZE=15;
const FILE_NAME_SIZE=20;

struct open_input
{
  char user_name[USER_NAME_SIZE];
  char file_name[FILE_NAME_SIZE];
};

struct open_output
{
  int fd; /* returns -1 if file cannot be opened, otherwise returns file descriptor*/
  char out_msg<>;  /* error message, if any */
};

struct read_input
{
  char user_name[USER_NAME_SIZE];
  int fd; /* file descriptor*/
  int numbytes;/* number of bytes to read */
};

struct read_output
{
  int success; /* set to 1 if read was successful, otherwise it is set to -1*/
  char buffer<>; /* data read is returned through this field*/
  char out_msg<>;  /* error message, if any*/
};

struct write_input
{
  char user_name[USER_NAME_SIZE];
  int fd;      /* file descriptor*/
  int numbytes; /* number of bytes to be written from the buffer*/
  char buffer<>; 
};

struct write_output
{
int success; /* set to 1 if write is successful, otherwise it is set to -1*/
  char out_msg<>; /* error/success message*/
};

struct list_input
{
  char user_name[USER_NAME_SIZE];
};

struct list_output
{
  char out_msg<>; /* list of files in the directory */
};

struct seek_input
{
char user_name[USER_NAME_SIZE];
int fd;
int position;
};

struct seek_output
{
int success;
 char out_msg<>;
};


struct delete_input
{
  char user_name[USER_NAME_SIZE];
  char file_name[FILE_NAME_SIZE];
};

struct delete_output
{
   char out_msg<>; /* file deleted or file not found message*/
}; 

struct close_input
{
char user_name[USER_NAME_SIZE];
int fd;
};

struct close_output
{
   char out_msg<>;
};


program SSNFSPROG{
  version SSNFSVER{
    open_output open_file(open_input) = 1;
    read_output read_file(read_input) = 2;
    write_output write_file(write_input) = 3;
    list_output list_files(list_input) = 4;
    delete_output delete_file(delete_input) = 5;
    close_output  close_file(close_input) = 6;
    seek_output seek_position(seek_input)=7;
  } = 1;
}=0x31110023;
