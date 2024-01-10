cmake_minimum_required(VERSION 3.23)
project(agentDataManagement)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")

add_executable(agentDataManagement main.cpp thread_pool.cpp thread_pool.h data_management.cpp data_management.h constants.h)

find_package(OpenSSL REQUIRED)
target_link_libraries(agentDataManagement PRIVATE OpenSSL::SSL OpenSSL::Crypto stdc++fs)