cmake_minimum_required(VERSION 3.23)
project(directoryCopy)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(directoryCopy main.cpp thread_pool.cpp thread_pool.h directory_copy.cpp directory_copy.h constants.h)

find_package(OpenSSL REQUIRED)
target_link_libraries(directoryCopy PRIVATE OpenSSL::SSL OpenSSL::Crypto stdc++fs)