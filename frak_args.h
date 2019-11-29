// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include <stdint.h>

#include "frakl/args.h"

enum frak_palette {
  frak_palette_default = 0,
  frak_palette_black_and_white = 1,
  frak_palette_gray = 2,
  frak_palette_color = 3,
  frak_palette_custom = 4,
};

enum frak_design {
  frak_design_mandlebrot = 1,
  frak_design_default = 2,
};

struct frak_color {
  uint16_t i;
  uint8_t red;
  uint8_t green;
  uint8_t blue;
};

struct frak_colors {
  unsigned count;
  struct frak_color colors[];
};

typedef struct frak_args {
  uint32_t width;
  uint32_t height;
  uint32_t ppi;
  const char* name;
  unsigned palette;
  unsigned design;
  uint32_t max_iteration;
  struct frak_colors* colors;
  double curve;
  bool palette_only;
  uint32_t worker_count;
  bool print_help;
  uint32_t worker_cache_size;
  bool randomize;
  bool stats;
} * frak_args_t;

extern struct arg_spec const* const frak_arg_specs;

void frak_usage(int code) __attribute__((noreturn));
void frak_args_init(frak_args_t args);
char* frak_args_validate(frak_args_t args);

static inline char* parse_frak_args(frak_args_t args, int argc,
                                    const char* argv[]) {
  return parse_args(argc - 1, argv + 1, frak_arg_specs, (void*)frak_args_init,
                    (void*)frak_args_validate, (void*)args);
}
