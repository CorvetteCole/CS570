# CS570 Assignment 1

If I had a little more time than this would be a bit better done. The basic gist is:

- `main.cpp`: contains the argparsing and general interaction with the user
- `SharedDatabase.hpp`: contains the database class and all the functions that interact with it
- `SharedDatabaseTests.cpp`: contains the unit tests for the database class

## Specification Differences

TL;DR: Database is loaded or created automatically when ran, querying, loading, printing, changing, etc can be done
interactively via one program.

## Usage

### Build

```shell
mkdir build
cd build
cmake ..
make
```

### Arguments

| Argument | Long       | Type   | Default | Description                                                                      |
|----------|------------|--------|---------|----------------------------------------------------------------------------------|
| -h       | --help     | flag   | N/A     | Print help message                                                               ||
| -p       | --password | string | N/A     | Password for the database, enables write-mode when matching existing             |
| -c       | --clean    | flag   | N/A     | Cleanup shared memory on exit                                                    |
| -l       | --load     | string | N/A     | Load the database from the specified file                                        |
| -o       | --output   | string | N/A     | Save the database to the specified file, or print to the console if not provided |
| -q       | --query    | string | N/A     | Query the database for the specified student ID                                  |
| -s       | --sleep    | int    | 0       | Sleep for the specified number of seconds after acquiring a semaphore            |

### Assignment Examples

Here are some examples on how to perform the actions specified in the assignment document. Note that `-s` can be used
to simulate a delay when semaphores are acquired.

#### Load

```shell
./assignment_1 -l sample_input.txt -p password
```

#### Print

Outputs all students to the console.

```shell
./assignment_1
```

Then select "4" in the interactive prompt

#### Change

Opens an interactive prompt to view and change the database. Will be read-only if no password is provided.

```shell
./assignment_1 -p password
```

#### Query

```shell
./assignment_1 -q 123456789
```

#### Clean

Outputs all students to the provided file and cleans up the shared memory on exit.

```shell
./assignment_1 -co output_students.txt
```

## Libraries Used

- [p-ranav/argparse](https://github.com/p-ranav/argparse)