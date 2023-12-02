cmake_minimum_required(VERSION 3.26)
project(Multi_threaded_Web_Server)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(client
        Client.cpp
        DieWithMessage.cpp
        Practical.h)

add_executable(server
        Server.cpp
        HandleTCPClient.cpp
        DieWithMessage.cpp
        Practical.h)