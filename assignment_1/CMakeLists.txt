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

link_libraries(pthread)
add_executable(assignment_1 main.cpp
        SharedDatabase.h
)
target_link_libraries(assignment_1 PRIVATE argparse nlohmann_json::nlohmann_json)

enable_testing()

FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(assignment_1_tests
        SharedDatabase.h
        SharedDatabaseTests.cpp
)
target_link_libraries(assignment_1_tests PRIVATE gtest_main argparse nlohmann_json::nlohmann_json)
include(GoogleTest)
gtest_discover_tests(assignment_1_tests)




