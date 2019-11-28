cmake_minimum_required(VERSION 3.8.1)

project(frak VERSION 0.1)

set(FRAK_TESTS_SRC driver.c tests.c tests_tests.c)

add_executable(frak_tests ${FRAK_TESTS_SRC})
target_compile_options(frak_tests PRIVATE ${FRAK_CFLAGS})
SET(CMAKE_EXE_LINKER_FLAGS -lz)
