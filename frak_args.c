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
    {.option = "custom", .value = frak_palette_custom},
    {.option = NULL, .value = 0},
};

static struct arg_enum_opt design_enum_opts[] = {
    {.option = "noise", .value = frak_design_noise},
    {.option = "mandlebrot", .value = frak_design_mandlebrot},
    {.option = "mand", .value = frak_design_mandlebrot},
    {.option = NULL, .value = 0},
};

static bool cp_num_bnd(const char** iter, char* buf, unsigned cnt, char c) {
  const char* const base = *iter;
  const char* liter = base;
  if (*liter == '\0') {
    return false;
  }
  while ((liter - base) <= cnt && *liter != '\0' && *liter != c) {
    char x = *liter;
    if (x < '0' || x > '9') {
      return false;
    }
    *buf = x;
    liter++;
    buf++;
  }
  if ((liter - base) > cnt) {
    return false;
  }
  liter++;
  buf[1] = '\0';
  *iter = liter;
  return true;
}

static char* color_parser(const char* arg, void* slot, void* ctx) {
  (void)ctx;
  const char* iter = arg;
  char red[4];
  char green[4];
  char blue[4];
  bool success = cp_num_bnd(&iter, red, 3, ',') &&
                 cp_num_bnd(&iter, green, 3, ',') &&
                 cp_num_bnd(&iter, blue, 3, ',');
  if (!success) {
    return strdup("bad color, expected r,g,b");
  }
  struct frak_color* color = malloc(sizeof(struct frak_color));
  color->red = atoi(red);
  color->green = atoi(green);
  color->blue = atoi(blue);
  *(struct frak_color**)slot = color;
  return NULL;
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
    {.flag = "--max-iter",
     .takes_arg = true,
     .required = false,
     .parser = pu32_parser,
     .offset = offsetof(struct frak_args, max_iteration)},
    {.flag = "--from",
     .takes_arg = true,
     .required = false,
     .parser = color_parser,
     .offset = offsetof(struct frak_args, from),
     .help = "Specify the color to start the gradient at when generating a"
             " custom color palette. Arg format is r,g,b. Implies --palette"
             " custom if --palette is left unspecified, errors out if palette"
             " is specified as non-custom"},
    {.flag = "--to",
     .takes_arg = true,
     .required = false,
     .parser = color_parser,
     .offset = offsetof(struct frak_args, to),
     .help = "Specify the color to end the gradient at when generating a"
             " custom color palette. Arg format is r,g,b. Implies --palette"
             " custom if --palette is left unspecified, errors out if palette"
             " is specified as non-custom"},
    {.flag = "--color-curve",
     .takes_arg = true,
     .required = false,
     .parser = pdbl_parser,
     .offset = offsetof(struct frak_args, curve),
     .help = "Specify the quadratic curve to choose colors between --from &"
             " --to"},
    {.flag = NULL},
};

void frak_args_init(frak_args_t args) {
  args->width = 256;
  args->height = 256;
  args->ppi = 401;
  args->name = NULL;
  args->palette = 0;
  args->design = frak_design_default;
  args->max_iteration = 0;
  args->from = NULL;
  args->to = NULL;
  args->curve = 1.0;
}

char* frak_args_validate(frak_args_t args) {
  if (args->palette == frak_palette_black_and_white &&
      args->design == frak_design_mandlebrot) {
    return strdup(
        "Mandlebrots cannot be generated with a black & white palette");
  }
  if (args->max_iteration != 0) {
    if (args->design == frak_design_default) {
      args->design = frak_design_mandlebrot;
    } else if (args->design != frak_design_mandlebrot) {
      return strdup("Cannot specify --max-iter without --design mandlebrot");
    }
  }
  if (args->design == frak_design_default) {
    args->design = frak_design_noise;
  }
  if (args->max_iteration == 0) {
    args->max_iteration = 1000;
  }

  if (args->from && !args->to) {
    return strdup("Cannot specify --from and not --to");
  } else if (!args->from && args->to) {
    return strdup("Cannot specify --to and not --from");
  } else if (args->from && args->to) {
    if (args->palette == frak_palette_default) {
      args->palette = frak_palette_custom;
    } else if (args->palette != frak_palette_custom) {
      return strdup("Cannot specify --from/--to without --palette custom");
    }
  }
  if (args->palette == frak_palette_default) {
    args->palette = frak_palette_black_and_white;
  } else if (args->palette == frak_palette_custom) {
    if (!(args->from && args->to)) {
      return strdup("Must specify --from/--to with --palette custom");
    }
  }
  return NULL;
}
