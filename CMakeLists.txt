cmake_minimum_required(VERSION 3.14.4)
project(OS_HW1)

set(CMAKE_CXX_STANDARD 11)

add_executable(OS_HW1 ProcessControlBlock.cpp ProcessControlBlock.h Commands.cpp Commands.h smash.cpp)