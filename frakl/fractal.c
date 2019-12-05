// Copywrite (c) 2019 Dan Zimmerman

#include "fractal.h"

// c = x + iy
// m_c(z) = z^2 + c
//        = z_x^2 - z_y^2 + x + i(2z_x * z_y + y)
static uint8_t mandlebrot_pixel(uint32_t column, uint32_t row, uint32_t width,
                                uint32_t height, uint32_t max, double ftop,
                                double fleft, double fwidth, double fheight) {
  long double x = fwidth * (long double)column / (long double)width + fleft;
  long double y = fheight * (long double)row / (long double)height + ftop;

  long double zx = x;
  long double zy = y;
  long double tmp;
  long double magsq = zx * zx + zy * zy;
  uint32_t result = 0;

  while (magsq <= 4.0 && result != max) {
    tmp = zx * zx - zy * zy + x;
    zy = 2 * zx * zy + y;
    zx = tmp;
    magsq = zx * zx + zy * zy;
    result += 1;
  }
  return (255 * result) / max;
}

void fractal_worker(void** pixels, unsigned n, struct fractal_ctx* ctx) {
  const uint32_t width = ctx->width;
  const uint32_t height = ctx->height;
  const uint32_t max = ctx->max_iteration;
  const double ftop = ctx->ftop;
  const double fleft = ctx->fleft;
  const double fwidth = ctx->fwidth;
  const double fheight = ctx->fheight;

  void** iter = pixels;
  void* const* const end = iter + n;
  do {
    uint32_t i = (uint32_t)*iter;
    void* pixel = ctx->buffer + i;
    uint32_t row = i / width;
    uint32_t column = i % width;
    *(uint8_t*)pixel = mandlebrot_pixel(column, row, width, height, max, ftop,
                                        fleft, fwidth, fheight);
  } while (++iter != end);
}
