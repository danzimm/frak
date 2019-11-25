cmake_minimum_required(VERSION 3.8.1)

project(frak VERSION 0.1)


set(FRAK_SRC main.c args.c frak_args.c tiff.c)
set(FRAK_CFLAGS -Wall -Werror -Wextra)

add_executable(frak ${FRAK_SRC})
add_executable(frak-dbg ${FRAK_SRC})
target_compile_options(frak PRIVATE ${FRAK_CFLAGS} -O3)
target_compile_options(frak-dbg PRIVATE ${FRAK_CFLAGS} -Og -g)
SET(CMAKE_EXE_LINKER_FLAGS -lz)
