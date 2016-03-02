cmake_minimum_required(VERSION 2.8)
project(final)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

link_directories(/usr/lib/x86_64-linux-gnu/)
link_directories(/usr/local/lib/)

set(SOURCE_FILES main.cpp http_server.cpp http_server.h http_server.cpp http_server.h)
add_executable(final ${SOURCE_FILES})

target_link_libraries(final ev pthread http_parser)