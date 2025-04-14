cmake_minimum_required(VERSION 3.10)
project(ChatApp)

set(CMAKE_CXX_STANDARD 17)

include_directories(include)

add_executable(server 
    src/server_main.cpp
    src/server.cpp
    src/client.cpp
)

add_executable(client 
    src/client_main.cpp
    src/client.cpp
)

