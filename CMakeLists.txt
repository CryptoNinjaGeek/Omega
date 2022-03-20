cmake_minimum_required(VERSION 3.21)
project(OmegaEngine)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${PROJECT_SOURCE_DIR}/cmake")

add_subdirectory(Engine)
add_subdirectory(Demo)
