cmake_minimum_required(VERSION 3.16)
project(assignment_5)

set(CMAKE_CXX_STANDARD 17)
include(FetchContent)
include_directories(/usr/include/tirpc)
link_libraries(tirpc dl m)

#add_executable(assignment_5 main.cpp
#        ssnfs.h
#        ssnfs_clnt.c
#        ssnfs_svc.c
#        ssnfs_xdr.c)

add_executable(client client.cpp
        ssnfs_clnt.c
        ssnfs_xdr.c)

add_executable(server server.cpp
        ssnfs_svc.c
        ssnfs_xdr.c
        VirtualDisk.h
        VirtualDisk.cpp)


enable_testing()
FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(virtual_disk_tests
        tests/VirtualDiskTests.cpp
        VirtualDisk.h
        VirtualDisk.cpp
        IVirtualDisk.h)

add_executable(server_tests
        tests/ServerTests.cpp
        ssnfs_svc.c
        ssnfs_xdr.c
        IVirtualDisk.h)

target_link_libraries(virtual_disk_tests gtest_main)

target_link_libraries(server_tests gtest_main)

include(GoogleTest)

gtest_discover_tests(virtual_disk_tests)
gtest_discover_tests(server_tests)










