cmake_minimum_required(VERSION 3.10)

# set the project name
project(Http)

# Enable C++11
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

add_executable(HttpServer server_linux.cpp http_tcpServer_linux.cpp)
target_include_directories(HttpServer PRIVATE .)

add_executable(ChatClient client.cpp)

# specify the C++ standard
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED True)