cmake_minimum_required(VERSION 3.8.1)

project(frakl VERSION 0.1)

set(FRAKL_SRC args.c tiff.c queue.c time_utils.c wq.c fractal.c)
add_library(frakl EXCLUDE_FROM_ALL ${FRAKL_SRC})
target_compile_options(frakl PRIVATE ${FRAK_CFLAGS})
