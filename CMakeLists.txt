cmake_minimum_required(VERSION 3.5)
project(IOT)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

set(SOURCE_FILES main.c serial_io.h serial_io_win32.c)
add_executable(IOT ${SOURCE_FILES})