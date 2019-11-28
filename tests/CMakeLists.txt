cmake_minimum_required(VERSION 3.8.1)

project(frak_tests VERSION 0.1)

set(FRAK_TESTS_SRC driver.c tests.c tests_tests.c)
add_executable(frak_tests ${FRAK_TESTS_SRC})
add_dependencies(frak_tests frakl)
target_compile_options(frak_tests PRIVATE ${FRAK_CFLAGS})
target_link_libraries(frak_tests z frakl)
