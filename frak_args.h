// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include "args.h"

struct frak_args {
  uint32_t width;
  uint32_t height;
  uint32_t ppi;
  const char* name;
  bool gray;
  bool color;
};

static void inline frak_args_init(struct frak_args* args) {
  args->width = 256;
  args->height = 256;
  args->ppi = 401;
  args->name = NULL;
  args->gray = false;
  args->color = false;
}
extern struct arg_spec const* const frak_arg_specs;

void frak_usage(void) __attribute__((noreturn));
char* frak_args_validate(struct frak_args* args);
