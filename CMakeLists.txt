cmake_minimum_required(VERSION 3.8.1)

project(frak VERSION 0.1)

add_executable(frak main.c args.c)
target_compile_options(frak PRIVATE -Wall -Werror -Wextra -O3)
SET(CMAKE_EXE_LINKER_FLAGS -lz)
