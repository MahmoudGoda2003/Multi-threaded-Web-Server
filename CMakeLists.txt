cmake_minimum_required(VERSION 3.26)
project(Multi_threaded_Web_Server)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(client
        Client.cpp
        DieWithMessage.cpp
        CreateServerAddress.cpp
        HttpParser.cpp
        Utilities.h
        FileOperations.cpp)

add_executable(server
        Server.cpp
        DieWithMessage.cpp
        CreateServerAddress.cpp
        HttpParser.cpp
        Utilities.h
        FileOperations.cpp)

target_link_libraries(server stdc++fs)
target_link_libraries(client stdc++fs)
target_link_libraries(server pthread)
