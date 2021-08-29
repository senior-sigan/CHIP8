cmake_minimum_required(VERSION 3.13)
project(CHIP8)

set(CMAKE_CXX_STANDARD 17)

set(BUILD_EXAMPLES OFF CACHE BOOL "Build the examples.")
set(BUILD_GAMES OFF CACHE BOOL "Build the example games.")
add_subdirectory(src/external/raylib-3.7.0)

add_executable(${PROJECT_NAME} src/main.cpp)

target_link_libraries(${PROJECT_NAME} raylib)