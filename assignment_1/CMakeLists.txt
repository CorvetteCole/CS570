cmake_minimum_required(VERSION 3.16)
project(assignment_1)

set(CMAKE_CXX_STANDARD 20)

# fetch latest argparse
include(FetchContent)
FetchContent_Declare(
        argparse
        GIT_REPOSITORY https://github.com/p-ranav/argparse.git
)
FetchContent_MakeAvailable(argparse)

link_libraries(pthread)
add_executable(assignment_1 main.cpp
        SharedDatabase.hpp
        Utilities.cpp
        Utilities.h
)
target_link_libraries(assignment_1 PRIVATE argparse)

enable_testing()

FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(assignment_1_tests
        SharedDatabase.hpp
        SharedDatabaseTests.cpp
        Utilities.cpp
        Utilities.h
)
target_link_libraries(assignment_1_tests PRIVATE gtest_main argparse)
include(GoogleTest)
gtest_discover_tests(assignment_1_tests)




