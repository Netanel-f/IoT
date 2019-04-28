cmake_minimum_required(VERSION 3.5)
project(IOT)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c11")

# EX2
#set(EX2_SOURCE_FILES Ex2/main.c Ex2/serial_io.h Ex2/serial_io_win32.c Ex2/gps.h Ex2/gps.c)

#add_executable(IOT_EX2 ${EX2_SOURCE_FILES})

# EX 3
set(EX3_SOURCE_FILES Ex3/main.c Ex3/serial_io.h Ex3/serial_io_win32.c)
#set(EX3_SOURCE_FILES Ex2/main.c Ex2/serial_io.h Ex2/serial_io_win32.c Ex2/gps.h Ex2/gps.c)
add_executable(IOT_EX3 ${EX3_SOURCE_FILES})