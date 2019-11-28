cmake_minimum_required(VERSION 3.8.1)

project(frak VERSION 0.1)

if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(OPT_FLAGS -Og -g)
else()
  set(OPT_FLAGS -O3)
endif()

set(FRAK_CFLAGS -Wall -Werror -Wextra ${OPT_FLAGS})

add_subdirectory(frakl)

set(FRAK_SRC main.c frak_args.c)
add_executable(frak ${FRAK_SRC})
add_dependencies(frak frakl)
target_compile_options(frak PRIVATE ${FRAK_CFLAGS})
target_link_libraries(frak z frakl)

add_subdirectory(tests)
