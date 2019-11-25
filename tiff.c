// Copywrite (c) 2019 Dan Zimmerman

#include "tiff.h"

#define packed_struct(x) struct __attribute__((packed)) x

packed_struct(tiff) {
  uint16_t byte_order;
  uint16_t magic;
  uint32_t ifd_offset;
};

enum ifd_entry_tag {
  ImageWidth = 0x0100,
  ImageLength = 0x0101,
  BitsPerSample = 0x0102,
  Compression = 0x0103,
  PhotometricInterpretation = 0x0106,
  StripOffsets = 0x0111,
  RowsPerStrip = 0x0116,
  StripByteCounts = 0x0117,
  XResolution = 0x011A,
  YResolution = 0x011B,
  ResolutionUnit = 0x0128,
};

enum ifd_entry_type {
  IFD_BYTE = 1,
  IFD_ASCII = 2,
  IFD_SHORT = 3,
  IFD_LONG = 4,
  IFD_RATIONAL = 5,
};

packed_struct(ifd_entry) {
  uint16_t tag;
  uint16_t type;
  uint32_t len;
  uint32_t value_or_offset;
};

packed_struct(ifd) {
  uint16_t len;
  struct ifd_entry entries[];
  /* uint32_t next_ifd_offset; */
};

static uint32_t compute_image_data_len(tiff_spec_t spec) {
  if (spec->type == tiff_bilevel) {
    uint32_t wadder = ((spec->width & 0x7) != 0) ? 1 : 0;
    return ((spec->width >> 3) + wadder) * spec->height;
  } else {
    return spec->width * spec->height;
  }
}

static uint32_t compute_resolution_off(tiff_spec_t spec) {
  return 8 + 2 + 10 * 12 + 4 + (spec->type != tiff_bilevel ? 12 : 0);
}

static uint32_t compute_image_data_off(tiff_spec_t spec) {
  return compute_resolution_off(spec) + 2 * 2 * 4;
}

uint32_t tiff_spec_compute_file_size(tiff_spec_t spec) {
  return compute_image_data_off(spec) + compute_image_data_len(spec);
}

static void* write_hdr(void* buf) {
  struct tiff* tiff = buf;
  tiff->byte_order = 0x4949;
  tiff->magic = 42;
  tiff->ifd_offset = 8;
  return (void*)(tiff + 1);
}

static void* write_entry(void* buf, enum ifd_entry_tag tag,
                         enum ifd_entry_type type, uint32_t len, uint32_t voo) {
  struct ifd_entry* entry = buf;
  entry->tag = tag;
  entry->type = type;
  entry->len = len;
  entry->value_or_offset = voo;
  return (void*)(entry + 1);
}

static void* write_rational(void* buf, uint32_t numer, uint32_t denom) {
  uint32_t* comp = buf;
  *comp++ = numer;
  *comp++ = denom;
  return (void*)comp;
}

static void* write_long(void* buf, uint32_t x) {
  uint32_t* xx = buf;
  *xx++ = x;
  return (void*)xx;
}

static void* write_short(void* buf, uint16_t x) {
  uint16_t* xx = buf;
  *xx++ = x;
  return xx;
}

void* tiff_spec_write_metadata(tiff_spec_t spec, void* buf) {
  // Header, ifd header
  buf = write_hdr(buf);
  buf = write_short(buf, 10 + (spec->type != tiff_bilevel ? 1 : 0));

  buf = write_entry(write_entry(buf, ImageWidth, IFD_LONG, 1, spec->width),
                    ImageLength, IFD_LONG, 1, spec->height);

  if (spec->type != tiff_bilevel) {
    buf = write_entry(buf, BitsPerSample, IFD_LONG, 1, 8);
  }

  buf = write_long(
      write_entry(
          write_entry(
              write_entry(
                  write_entry(
                      write_entry(
                          write_entry(write_entry(write_entry(buf, Compression,
                                                              IFD_SHORT, 1, 1),
                                                  PhotometricInterpretation,
                                                  IFD_SHORT, 1, 1),
                                      StripOffsets, IFD_LONG, 1,
                                      compute_image_data_off(spec)),
                          RowsPerStrip, IFD_LONG, 1, spec->height),
                      StripByteCounts, IFD_LONG, 1,
                      compute_image_data_len(spec)),
                  XResolution, IFD_RATIONAL, 1, compute_resolution_off(spec)),
              YResolution, IFD_RATIONAL, 1, compute_resolution_off(spec) + 8),
          ResolutionUnit, IFD_SHORT, 1, 2),
      0);
  buf = write_rational(write_rational(buf, spec->ppi, 1), spec->ppi, 1);
  return buf;
}
