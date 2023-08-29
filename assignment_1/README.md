# CS570 Assignment 1
## Specification Differences
Here are a list of places where I deviated from the given specification to build a more robust program.
- example 3
- example 2

## Usage
### Build
```shell
mkdir build
cd build
cmake ..
make
```

### Run
In your `build` directory:
```shell
./assignment_1
```

### Arguments

[//]: # (Table contained print query change load and clean commands, with some extra cells unfilled for what they do. Arguments will include long and short versions e.g. -v and --verbose)

| Argument | Long          | Type | Description            |
|----------|---------------|------|------------------------|
| -h       | --help        | flag | Print help message     |
| -v       | --verbose     | flag | Enable verbose output  |
| -i       | --interactive | flag | Enter interactive mode |



## Libraries Used
- [nlohmann/json](https://github.com/nlohmann/json)
- [p-ranav/argparse](https://github.com/p-ranav/argparse)