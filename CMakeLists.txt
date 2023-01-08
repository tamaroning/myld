cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")

project(myld VERSION 0.1)

configure_file(config.h.in config.h)

add_executable(myld src/main.cc)
target_include_directories(myld PUBLIC "${PROJECT_BINARY_DIR}")

