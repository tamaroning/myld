cmake_minimum_required(VERSION 3.16)
set(CMAKE_C_COMPILER "/usr/bin/clang")
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")

project(myld VERSION 0.1)

# エントリーポイント
add_executable(myld src/main.cc src/gen-elf.cc)

# config.hがbuild/に生成されるのでリンクする
configure_file(config.h.in config.h)
target_include_directories(myld PUBLIC "${PROJECT_BINARY_DIR}")

# src/以下の.hをインクルードする
target_include_directories(myld PUBLIC ${PROJECT_SOURCE_DIR}/src)

# use libfmt
find_package(fmt REQUIRED)
include_directories(${fmt_INCLUDE_DIRS})
target_link_libraries(myld fmt::fmt)
