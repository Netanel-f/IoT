cmake_minimum_required(VERSION 3.5)
project(IOT)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

set(EX2_SOURCE_FILES Ex2/main.c Ex2/serial_io.h Ex2/serial_io_win32.c Ex2/gps.h Ex2/gps.c)

add_executable(IOT_EX2 ${EX2_SOURCE_FILES})