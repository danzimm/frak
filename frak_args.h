// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include "args.h"

typedef struct frak_args {
  uint32_t width;
  uint32_t height;
  uint32_t ppi;
  const char* name;
  bool gray;
  bool color;
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
