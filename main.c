// Copywrite (c) 2019 Dan Zimmerman

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <zlib.h>

#include "arg.h"

#define packed_struct(x) struct __attribute__((packed)) x

packed_struct(tiff) {
  uint16_t byte_order;
  uint16_t magic;
  uint32_t ifd_offset;
};

enum ifd_entry_tag {
  ImageWidth = 0x0100,
  ImageLength = 0x0101,
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

struct frak_args {
  uint32_t width;
  uint32_t height;
  uint32_t ppi;
  const char* name;
};

struct img {
  struct tiff tiff;
  struct frak_args args;
};

static uint32_t compute_image_data_len(struct img* img) {
  uint32_t total_bits = img->args.width * img->args.height;
  uint32_t adder = (total_bits & 0x7) != 0 ? 1 : 0;
  return (total_bits >> 3) + adder;
}

static uint32_t compute_resolution_off(void) { return 8 + 2 + 10 * 12 + 4; }

static uint32_t compute_image_data_off(void) {
  return compute_resolution_off() + 2 * 2 * 4;
}

static uint32_t compute_len(struct img* img) {
  // header, ifd header, 10 ifd entries, ifd footer, x res, y res, image data
  return compute_image_data_off() + compute_image_data_len(img);
}

static void usage(const char* cmd) __attribute__((noreturn));
static void usage(const char* cmd) {
  fprintf(stderr,
          "%s: a tiff generator. Example usage: %s [options] name.tiff\n", cmd,
          cmd);
  fprintf(stderr, "options:\n");
  fprintf(stderr,
          "--width [width]   width in pixels of final image. default 256\n");
  fprintf(stderr,
          "--height [height] height in pixels of final image. default 256\n");
  fprintf(stderr,
          "--ppi [ppi]       pixels per inch of final image. default 401\n");
  exit(1);
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

  buf = write_long(
      write_entry(
          write_entry(
              write_entry(
                  write_entry(
                      write_entry(
                          write_entry(
                              write_entry(
                                  write_entry(
                                      write_entry(
                                          write_entry(
                                              write_short(write_hdr(buf), 10),
                                              ImageWidth, IFD_LONG, 1,
                                              img->args.width),
                                          ImageLength, IFD_LONG, 1,
                                          img->args.height),
                                      Compression, IFD_SHORT, 1, 1),
                                  PhotometricInterpretation, IFD_SHORT, 1, 1),
                              StripOffsets, IFD_LONG, 1,
                              compute_image_data_off()),
                          RowsPerStrip, IFD_LONG, 1, img->args.height),
                      StripByteCounts, IFD_LONG, 1,
                      compute_image_data_len(img)),
                  XResolution, IFD_RATIONAL, 1, compute_resolution_off()),
              YResolution, IFD_RATIONAL, 1, compute_resolution_off() + 8),
          ResolutionUnit, IFD_SHORT, 1, 2),
      0);
  printf("Wrote %#lx header bytes (%#x)\n", buf - base,
         compute_resolution_off());

  buf = write_rational(write_rational(buf, img->args.width, img->args.ppi),
                       img->args.height, img->args.ppi);
  printf("Wrote %#lx header/metadata bytes (%#x)\n", buf - base,
         compute_image_data_off());
  return buf;
}

static void inline frak_args_init(struct frak_args* args) {
  args->width = 256;
  args->height = 256;
  args->ppi = 401;
  args->name = NULL;
}

struct arg_spec frak_arg_specs[] = {
    {
        .flag = "--width",
        .takes_arg = true,
        .parser = u32_parser,
        .offset = offsetof(struct frak_args, width),
    },
    {
        .flag = "--height",
        .takes_arg = true,
        .parser = u32_parser,
        .offset = offsetof(struct frak_args, height),
    },
    {
        .flag = "--ppi",
        .takes_arg = true,
        .parser = u32_parser,
        .offset = offsetof(struct frak_args, ppi),
    },
    {
        .flag = "name",
        .takes_arg = true,
        .parser = str_parser,
        .offset = offsetof(struct frak_args, name),
    },
    {
        .flag = NULL,
        .takes_arg = 0,
        .parser = NULL,
        .offset = 0,
    },
};

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
    fprintf(stderr, "%s", err);
    free(err);
    goto out;
  }

  if (!img.args.width || !img.args.height || !img.args.name) {
    usage(argv[0]);
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
