cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

project(pandora-c-sdk)

set(CMAKE_MACOSX_RPATH 0)
add_subdirectory(src)
add_subdirectory(sample)
