cmake_minimum_required(VERSION 3.26)
project(assignment_1)

set(CMAKE_CXX_STANDARD 20)

add_executable(assignment_1 main.cpp
        common/SharedDatabase.cpp
        common/SharedDatabase.h
        common/StudentInfo.cpp
        common/StudentInfo.h
)
