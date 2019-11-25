// Copywrite (c) 2019 Dan Zimmerman

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <zlib.h>

#include "frak_args.h"

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

struct img {
  struct tiff tiff;
  struct frak_args args;
};

static uint32_t compute_image_data_len(struct img* img) {
  if (!img->args.gray) {
    uint32_t wadder = ((img->args.width & 0x7) != 0) ? 1 : 0;
    return ((img->args.width >> 3) + wadder) * img->args.height;
  } else {
    return img->args.width * img->args.height;
  }
}

static uint32_t compute_resolution_off(struct img* img) {
  return 8 + 2 + 10 * 12 + 4 + (img->args.gray ? 12 : 0);
}

static uint32_t compute_image_data_off(struct img* img) {
  return compute_resolution_off(img) + 2 * 2 * 4;
}

static uint32_t compute_len(struct img* img) {
  // header, ifd header, 10 ifd entries, ifd footer, x res, y res, image data
  return compute_image_data_off(img) + compute_image_data_len(img);
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

static void* write_header_and_metadata(void* buf, struct img* img) {
  void* const base = buf;

  // Header, ifd header
  buf = write_hdr(buf);
  buf = write_short(buf, 10 + (img->args.gray ? 1 : 0));

  buf = write_entry(write_entry(buf, ImageWidth, IFD_LONG, 1, img->args.width),
                    ImageLength, IFD_LONG, 1, img->args.height);

  if (img->args.gray) {
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
                                      compute_image_data_off(img)),
                          RowsPerStrip, IFD_LONG, 1, img->args.height),
                      StripByteCounts, IFD_LONG, 1,
                      compute_image_data_len(img)),
                  XResolution, IFD_RATIONAL, 1, compute_resolution_off(img)),
              YResolution, IFD_RATIONAL, 1, compute_resolution_off(img) + 8),
          ResolutionUnit, IFD_SHORT, 1, 2),
      0);
  printf("Wrote %#lx header bytes (%#x)\n", buf - base,
         compute_resolution_off(img));

  buf = write_rational(write_rational(buf, img->args.ppi, 1), img->args.ppi, 1);
  printf("Wrote %#lx header/metadata bytes (%#x)\n", buf - base,
         compute_image_data_off(img));
  return buf;
}

int main(int argc, const char* argv[]) {
  int rc = 0;
  int fd = -1;
  void* buf = NULL;
  void* data;
  struct img img;
  size_t len;

  frak_args_init(&img.args);
  char* err = parse_args(argc - 1, argv + 1, frak_arg_specs, &img.args);
  if (err) {
    fprintf(stderr, "%s\n\n", err);
    free(err);
    usage();
  }

  if (!img.args.width || !img.args.height || !img.args.name) {
    usage();
  }
  len = compute_len(&img);

  fd = open(img.args.name, O_RDWR | O_CREAT | O_TRUNC, 0644);
  if (fd < 0) {
    perror("open");
    rc = 1;
    goto out;
  }

  if ((rc = ftruncate(fd, len)) != 0) {
    perror("truncate");
    goto out;
  }

  buf = mmap(NULL, len, PROT_WRITE | PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
  if (!buf || buf == MAP_FAILED) {
    perror("mmap");
    rc = 1;
    goto out;
  }

  data = write_header_and_metadata(buf, &img);
  arc4random_buf(data, compute_image_data_len(&img));

out:
  if (buf && buf != MAP_FAILED) {
    munmap(buf, len);
  }
  if (fd >= 0) {
    close(fd);
  }
  if (rc != 0) {
    unlink(img.args.name);
  }
  return rc;
}
