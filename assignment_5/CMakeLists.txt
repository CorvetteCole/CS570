cmake_minimum_required(VERSION 3.16)
project(assignment_5)

set(CMAKE_CXX_STANDARD 17)

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
        ssnfs_xdr.c)



