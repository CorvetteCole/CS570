cmake_minimum_required(VERSION 3.16)
project(assignment_5)

set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 17)

include(FetchContent)

if(EXISTS "/usr/include/tirpc")
    include_directories(/usr/include/tirpc)
    link_libraries(tirpc dl m)
endif()

set(GENERATED_RPC_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated_rpc)
file(MAKE_DIRECTORY ${GENERATED_RPC_DIR})
include_directories(${GENERATED_RPC_DIR})

# Copy the .x file to the generation directory
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/ssnfs.x ${GENERATED_RPC_DIR}/ssnfs.x COPYONLY)

# Define custom command to run rpcgen in the generation directory
add_custom_command(
        OUTPUT
        ${GENERATED_RPC_DIR}/ssnfs_xdr.c
        ${GENERATED_RPC_DIR}/ssnfs.h
        ${GENERATED_RPC_DIR}/ssnfs_clnt.c
        ${GENERATED_RPC_DIR}/ssnfs_svc.c
        WORKING_DIRECTORY ${GENERATED_RPC_DIR}
        COMMAND rpcgen ssnfs.x # Call rpcgen on the copied .x file
        DEPENDS ${GENERATED_RPC_DIR}/ssnfs.x # Depend on the copied .x file
        VERBATIM
)

add_executable(client client.cpp
        ${GENERATED_RPC_DIR}/ssnfs_clnt.c
        ${GENERATED_RPC_DIR}/ssnfs_xdr.c)

add_executable(server server.cpp
        ${GENERATED_RPC_DIR}/ssnfs_svc.c
        ${GENERATED_RPC_DIR}/ssnfs_xdr.c
        VirtualDisk.cpp)

enable_testing()
FetchContent_Declare(googletest URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip)
# For Windows: Prevent overriding the parent project's compiler/linker settings
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(virtual_disk_tests tests/VirtualDiskTests.cpp
        VirtualDisk.cpp
        ${GENERATED_RPC_DIR}/ssnfs.h)

target_link_libraries(virtual_disk_tests gtest_main)

include(GoogleTest)

gtest_discover_tests(virtual_disk_tests)










