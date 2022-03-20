cmake_minimum_required(VERSION 3.21)
project(Basic)

set(CMAKE_CXX_STANDARD 14)

add_executable(Basic main.cpp)
include_directories(Basic PUBLIC ../../Engine/Header )
target_link_libraries(Basic oEngine)
