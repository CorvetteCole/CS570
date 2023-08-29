cmake_minimum_required(VERSION 3.26)
project(assignment_1)

set(CMAKE_CXX_STANDARD 20)

add_executable(assignment_1 main.cpp
        Load.cpp
        Load.h
        Print.cpp
        Print.h
        Query.cpp
        Query.h
        Change.cpp
        Change.h
        Clean.cpp
        Clean.h)
