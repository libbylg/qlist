cmake_minimum_required(VERSION 3.4)
project(qlist)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES qlist_test.c)
add_executable(qlist ${SOURCE_FILES})