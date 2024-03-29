// Copywrite (c) 2019 Dan Zimmerman

#include "frak_args.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void frak_usage(int code) {
  static char const* const cmd = "frak";
  char* usage = create_usage(cmd, "a tiff generator", frak_arg_specs);
  fprintf(stderr, "%s", usage);
  free(usage);
  exit(code);
}

static struct arg_enum_opt palette_enum_opts[] = {
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
    {.option = "mandlebrot", .value = frak_design_mandlebrot},
    {.option = "mand", .value = frak_design_mandlebrot},
    {.option = NULL, .value = 0},
};

static char* color_parser(const char* arg, void* slot, void* ctx) {
  (void)ctx;

  long irgb[4];
  struct tuple_spec spec = {
      .is_double = false,
      .count = 4,
  };
  char* err = tuple_parser(arg, irgb, &spec);
  if (err) {
    goto out;
  }

  static const char* element_desc[] = {"index", "red", "blue", "green"};
  for (unsigned i = 0; i < 4; i++) {
    if (irgb[i] < 0) {
      asprintf(&err, "the %s component must be non-negative, was %ld",
               element_desc[i], irgb[i]);
      goto out;
    }
    if (irgb[i] > 255) {
      asprintf(&err, "the %s component cannot be bigger than 255, was %ld",
               element_desc[i], irgb[i]);
      goto out;
    }
  }

  struct frak_colors* colors = *(struct frak_colors**)slot;
  if (!colors) {
    colors = malloc(sizeof(struct frak_colors));
    colors->count = 0;
  }

  colors = realloc(colors, sizeof(struct frak_colors) +
                               sizeof(struct frak_color) * (++colors->count));
  struct frak_color* color = &colors->colors[colors->count - 1];
  color->i = irgb[0];
  color->red = irgb[1];
  color->green = irgb[2];
  color->blue = irgb[3];

  *(struct frak_colors**)slot = colors;

out:
  return err;
}

const struct tuple_spec center_tuple_spec = {
    .count = 2,
    .is_double = true,
};

struct arg_spec const* const frak_arg_specs = (struct arg_spec[]){
    {.flag = "--width",
     .takes_arg = true,
     .parser = pu32_parser,
     .offset = offsetof(struct frak_args, width),
     .help = "Width of result in pixels"},
    {.flag = "--height",
     .takes_arg = true,
     .parser = pu32_parser,
     .offset = offsetof(struct frak_args, height),
     .help = "Height of result in pixels"},
    {.flag = "--ppi",
     .takes_arg = true,
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
     .parser = pu32_parser,
     .offset = offsetof(struct frak_args, max_iteration)},
    {.flag = "--color",
     .takes_arg = true,
     .parser = color_parser,
     .offset = offsetof(struct frak_args, colors),
     .help = "Specify a color to use starting at iteration i. Arg format is"
             " i,r,g,b. Implies --palette custom if --palette is left"
             " unspecified, errors out if palette is non-custom"},
    {.flag = "--color-curve",
     .takes_arg = true,
     .parser = pdbl_parser,
     .offset = offsetof(struct frak_args, curve),
     .help = "Specify the quadratic curve to choose colors between different"
             " --color's"},
    {.flag = "--palette-only",
     .takes_arg = false,
     .parser = bool_parser,
     .offset = offsetof(struct frak_args, palette_only),
     .help = "In place change the color palette of an image"},
    {.flag = "-j",
     .takes_arg = true,
     .parser = pu32_parser,
     .offset = offsetof(struct frak_args, worker_count),
     .help = "Configure the number of workers to use when running. Defaults to"
             " 4/3 * number of active processors (specify 0 for this default)"},
    {.flag = "--help",
     .parser = bool_parser,
     .offset = offsetof(struct frak_args, print_help),
     .help = "Show this help page"},
    {.flag = "--worker-cache-size",
     .takes_arg = true,
     .parser = pu32_parser,
     .offset = offsetof(struct frak_args, worker_cache_size),
     .help = "The number of pixels to place into a single work item"},
    {.flag = "--stats",
     .parser = bool_parser,
     .offset = offsetof(struct frak_args, stats),
     .help = "Print stats about each phase of running."},
    {.flag = "--no-compute",
     .parser = bool_parser,
     .offset = offsetof(struct frak_args, no_compute),
     .help = "Don't actually compute the pixels for the output image. Useful"
             " for benchmarking"},
    {.flag = "--center",
     .takes_arg = true,
     .parser = tuple_parser,
     .parser_ctx = (void*)&center_tuple_spec,
     .offset = offsetof(struct frak_args, center),
     .help = "Specify the center of the fractal in x,y. Defaults to 0,0"},
    {.flag = "--fwidth",
     .takes_arg = true,
     .parser = pdbl_parser,
     .offset = offsetof(struct frak_args, fwidth),
     .help = "Specify the width of the fractal in the fractal's coordinate"
             " system. The height will automatically be calculated based on the"
             " aspect ratio of the image. Defaults to 4"},
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
  args->colors = NULL;
  args->curve = 1.0;
  args->palette_only = false;
  args->worker_count = 0;
  args->print_help = false;
  args->worker_cache_size = 0;
  args->stats = false;
  args->no_compute = false;
  args->center[0] = 0;
  args->center[1] = 0;
  args->fwidth = 4;
}

static int color_sort(void const* a, void const* b) {
  struct frak_color const* ca = a;
  struct frak_color const* cb = b;
  return ca->i - cb->i;
}

char* frak_args_validate(frak_args_t args) {
  if (args->width * args->height >= UINT32_MAX) {
    return strdup(
        "Image requested is too large, only images up until 2^32-1 pixels "
        "large are supported right now");
  }
  if (args->design == frak_design_default) {
    args->design = frak_design_mandlebrot;
  }
  if (args->max_iteration != 0) {
    if (args->design == frak_design_default) {
      args->design = frak_design_mandlebrot;
    } else if (args->design != frak_design_mandlebrot) {
      return strdup("Cannot specify --max-iter without --design mandlebrot");
    }
  }
  if (args->max_iteration == 0) {
    args->max_iteration = 1000;
  }

  if (args->colors) {
    if (args->palette == frak_palette_default) {
      args->palette = frak_palette_custom;
    } else if (args->palette != frak_palette_custom) {
      return strdup("Cannot specify --color without --palette custom");
    }
  }
  if (args->palette == frak_palette_default) {
    args->palette = frak_palette_gray;
  } else if (args->palette == frak_palette_custom) {
    if (!args->colors) {
      return strdup("Must specify --color with --palette custom");
    }
    if (args->colors->count < 2) {
      return strdup("Must specify at least two --color options");
    }
    qsort(args->colors->colors, args->colors->count, sizeof(struct frak_color),
          color_sort);
    if (args->colors->colors[args->colors->count - 1].i > 256) {
      return strdup("Colors cannot be at an index greater than 256");
    }
  }
  if (args->palette_only) {
    if (args->palette != frak_palette_custom &&
        args->palette != frak_palette_color) {
      return strdup(
          "Can only in place change palette if color palette is"
          " being used (--palette color/custom)");
    }
  }
  if (!args->worker_cache_size) {
    args->worker_cache_size = (uint32_t)-1;
  }
  return NULL;
}
