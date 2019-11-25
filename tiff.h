// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include <stdint.h>

enum tiff_spec_type {
  tiff_bilevel,
  tiff_gray,
};

typedef struct tiff_spec {
  enum tiff_spec_type type;
  uint32_t width;
  uint32_t height;
  uint32_t ppi;
} * tiff_spec_t;

uint32_t tiff_spec_compute_file_size(tiff_spec_t spec);
void* tiff_spec_write_metadata(tiff_spec_t spec, void* buffer);
