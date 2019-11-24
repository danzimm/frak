// Copywrite (c) 2019 Dan Zimmerman

#include "frak_args.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void usage() {
  static char const* const cmd = "frak";
  fprintf(stderr,
          "%s: a tiff generator. Example usage: %s [options] name.tiff\n", cmd,
          cmd);
  fprintf(stderr, "options:\n");
  fprintf(stderr,
          "--width [width]   width in pixels of final image. default 256\n");
  fprintf(stderr,
          "--height [height] height in pixels of final image. default 256\n");
  fprintf(stderr,
          "--ppi [ppi]       pixels per inch of final image. default 401\n");
  fprintf(stderr, "--gray            specify generate a grayscale image\n");
  exit(1);
}

struct arg_spec const* const frak_arg_specs = (struct arg_spec[]){
    {
        .flag = "--width",
        .takes_arg = true,
        .parser = u32_parser,
        .offset = offsetof(struct frak_args, width),
    },
    {
        .flag = "--height",
        .takes_arg = true,
        .parser = u32_parser,
        .offset = offsetof(struct frak_args, height),
    },
    {
        .flag = "--ppi",
        .takes_arg = true,
        .parser = u32_parser,
        .offset = offsetof(struct frak_args, ppi),
    },
    {
        .flag = "name",
        .takes_arg = true,
        .parser = str_parser,
        .offset = offsetof(struct frak_args, name),
    },
    {
        .flag = "--gray",
        .takes_arg = false,
        .parser = bool_parser,
        .offset = offsetof(struct frak_args, gray),
    },
    {
        .flag = NULL,
        .takes_arg = 0,
        .parser = NULL,
        .offset = 0,
    },
};
