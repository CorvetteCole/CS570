cmake_minimum_required(VERSION 3.26)
project(assignment_1)

set(CMAKE_CXX_STANDARD 20)

# fetch latest argparse
include(FetchContent)
FetchContent_Declare(
        argparse
        GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

# fetch latest nlohmann/json
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
FetchContent_MakeAvailable(json)

add_executable(assignment_1 main.cpp
        common/SharedDatabase.cpp
        common/SharedDatabase.h
        common/StudentInfo.cpp
        common/StudentInfo.h
)
target_link_libraries(assignment_1 PRIVATE argparse nlohmann_json::nlohmann_json)

