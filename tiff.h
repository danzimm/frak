// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include <stdint.h>

enum tiff_spec_type {
  tiff_bilevel,
  tiff_gray,
  tiff_palette,
};

struct tiff_palette_color {
  uint16_t red;
  uint16_t green;
  uint16_t blue;
};

typedef struct tiff_palette {
  unsigned len;
  struct tiff_palette_color colors[];
} * tiff_palette_t;

typedef struct tiff_spec {
  enum tiff_spec_type type;
  uint32_t width;
  uint32_t height;
  uint32_t ppi;
  // Only needed if type == tiff_palette
  tiff_palette_t palette;
} * tiff_spec_t;

uint32_t tiff_spec_compute_file_size(tiff_spec_t spec);
void* tiff_spec_write_metadata(tiff_spec_t spec, void* buffer);
