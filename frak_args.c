// Copywrite (c) 2019 Dan Zimmerman

#include "frak_args.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

void frak_usage() {
  static char const* const cmd = "frak";
  char* usage = create_usage(cmd, "a tiff generator", frak_arg_specs);
  fprintf(stderr, "%s", usage);
  free(usage);
  exit(1);
}

struct arg_spec const* const frak_arg_specs = (struct arg_spec[]){
    {.flag = "--width",
     .takes_arg = true,
     .required = false,
     .parser = pu32_parser,
     .offset = offsetof(struct frak_args, width),
     .help = "Width of result in pixels"},
    {.flag = "--height",
     .takes_arg = true,
     .required = false,
     .parser = pu32_parser,
     .offset = offsetof(struct frak_args, height),
     .help = "Height of result in pixels"},
    {.flag = "--ppi",
     .takes_arg = true,
     .required = false,
     .parser = pu32_parser,
     .offset = offsetof(struct frak_args, ppi),
     .help = "Pixels per inch of result"},
    {.flag = "name",
     .takes_arg = true,
     .required = true,
     .parser = str_parser,
     .offset = offsetof(struct frak_args, name),
     .help = "Path to the file the result should be written to. Note .tiff"
             " will not automatically be appended, but tiff data will be"
             " written"},
    {.flag = "--gray",
     .takes_arg = false,
     .required = false,
     .parser = bool_parser,
     .offset = offsetof(struct frak_args, gray),
     .help = "Generate gray noise instead of black & white"},
    {.flag = NULL, .takes_arg = 0, .parser = NULL, .offset = 0, .help = NULL},
};
