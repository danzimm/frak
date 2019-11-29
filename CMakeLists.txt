cmake_minimum_required(VERSION 3.8.1)

project(frak VERSION 0.1)

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()
if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
  set(OPT_FLAGS -O0 -g)
else()
  set(OPT_FLAGS -O3)
endif()

if (FRAK_SAN_TYPE)
  set(SAN_FLAGS "-fsanitize=${FRAK_SAN_TYPE}")
  set(CMAKE_EXE_LINKER_FLAGS ${SAN_FLAGS})
  set(CMAKE_SHARED_LINKER_FLAGS ${SAN_FLAGS})
  set(OPT_FLAGS ${OPT_FLAGS} -g -fno-omit-frame-pointer)
endif()

set(FRAK_CFLAGS -Wall -Werror -Wextra -D_GNU_SOURCE=1 ${OPT_FLAGS} ${SAN_FLAGS})

add_subdirectory(frakl)

set(FRAK_SRC main.c frak_args.c)
add_executable(frak ${FRAK_SRC})
add_dependencies(frak frakl)
target_compile_options(frak PRIVATE ${FRAK_CFLAGS})
target_link_libraries(frak z m pthread frakl)

add_subdirectory(tests)
