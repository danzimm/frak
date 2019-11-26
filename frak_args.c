// Copywrite (c) 2019 Dan Zimmerman

#include "frak_args.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void frak_usage() {
  static char const* const cmd = "frak";
  char* usage = create_usage(cmd, "a tiff generator", frak_arg_specs);
  fprintf(stderr, "%s", usage);
  free(usage);
  exit(1);
}

static struct arg_enum_opt palette_enum_opts[] = {
    {.option = "b", .value = frak_palette_black_and_white},
    {.option = "bw", .value = frak_palette_black_and_white},
    {.option = "bilevel", .value = frak_palette_black_and_white},
    {.option = "blackwhite", .value = frak_palette_black_and_white},
    {.option = "g", .value = frak_palette_gray},
    {.option = "gray", .value = frak_palette_gray},
    {.option = "grey", .value = frak_palette_gray},
    {.option = "c", .value = frak_palette_color},
    {.option = "color", .value = frak_palette_color},
    {.option = "colour", .value = frak_palette_color},
    {.option = NULL, .value = 0},
};

static struct arg_enum_opt design_enum_opts[] = {
    {.option = "noise", .value = frak_design_noise},
    {.option = "mandlebrot", .value = frak_design_mandlebrot},
    {.option = "mand", .value = frak_design_mandlebrot},
    {.option = NULL, .value = 0},
};

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
    {.flag = "--palette",
     .takes_arg = true,
     .required = false,
     .parser = enum_parser,
     .parser_ctx = (void*)palette_enum_opts,
     .offset = offsetof(struct frak_args, palette),
     .help = "Specify the color palette to use when generating the image"},
    {.flag = "--design",
     .takes_arg = true,
     .required = false,
     .parser = enum_parser,
     .parser_ctx = (void*)design_enum_opts,
     .offset = offsetof(struct frak_args, design)},
    {.flag = NULL},
};

void frak_args_init(frak_args_t args) {
  args->width = 256;
  args->height = 256;
  args->ppi = 401;
  args->name = NULL;
  args->palette = 0;
  args->design = 0;
}

char* frak_args_validate(frak_args_t args) {
  if (args->palette == frak_palette_black_and_white &&
      args->design == frak_design_mandlebrot) {
    return strdup(
        "Mandlebrots cannot be generated with a black & white palette");
  }
  return NULL;
}
