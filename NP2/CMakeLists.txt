cmake_minimum_required(VERSION 3.20)
project(NP2 C)

set(CMAKE_C_STANDARD 99)

add_executable(NP2 server.c server.h client.h client.c)
