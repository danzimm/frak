// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include <stdint.h>

struct fractal_ctx {
  uint32_t width;
  uint32_t height;
  uint32_t max_iteration;
  void* buffer;
};

void fractal_worker(void** pixels, unsigned n, struct fractal_ctx* ctx);
