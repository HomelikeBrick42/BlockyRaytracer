cmake_minimum_required(VERSION 3.20)
project(VoxelRendering C)

set(CMAKE_C_STANDARD 11)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC" OR "x${CMAKE_CXX_SIMULATE_ID}" STREQUAL "xMSVC")
    add_compile_options(-W4) # Wall on MSVC has hundreds of warnings in the std libary, windows header, etc
else ()
    add_compile_options(-Wall -Wextra)
endif ()
add_compile_options(-Werror -Wno-unused-parameter)

add_compile_options(-D_USE_MATH_DEFINES -D_CRT_SECURE_NO_WARNINGS)

if (CMAKE_BUILD_TYPE MATCHES Release)
    add_compile_options(-O3)
endif ()

include_directories(src)

add_link_options(-lopengl32)
add_executable(VoxelRendering
        src/gl.h
        src/Main.c)
