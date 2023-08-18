cmake_minimum_required(VERSION 3.12)
project(catch)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_COLOR_DIAGNOSTICS OFF)

add_library(catch INTERFACE)
target_include_directories(catch INTERFACE include/public)
