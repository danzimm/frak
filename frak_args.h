// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include "args.h"

enum frak_palette {
  frak_palette_black_and_white = 0,
  frak_palette_gray = 1,
  frak_palette_color = 2,
  frak_palette_blue = 3,
};

enum frak_design {
  frak_design_noise = 0,
  frak_design_mandlebrot = 1,
  frak_design_default = 2,
};

typedef struct frak_args {
  uint32_t width;
  uint32_t height;
  uint32_t ppi;
  const char* name;
  unsigned palette;
  unsigned design;
  uint32_t max_iteration;
} * frak_args_t;

extern struct arg_spec const* const frak_arg_specs;

void frak_usage(void) __attribute__((noreturn));
void frak_args_init(frak_args_t args);
char* frak_args_validate(frak_args_t args);

static inline char* parse_frak_args(frak_args_t args, int argc,
                                    const char* argv[]) {
  return parse_args(argc - 1, argv + 1, frak_arg_specs, (void*)frak_args_init,
                    (void*)frak_args_validate, (void*)args);
}
