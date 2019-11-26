// Copywrite (c) 2019 Dan Zimmerman

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <zlib.h>

#include "frak_args.h"
#include "tiff.h"

static void noise_generator(struct frak_args const* const args, void* buffer,
                            size_t len) {
  (void)args;
  arc4random_buf(buffer, len);
}

// c = x + iy
// m_c(z) = z^2 + c
//        = z_x^2 - z_y^2 + x + i(2z_x * z_y + y)
static uint8_t compute_mandlebrot_pixel(double x, double y) {
  double zx = x;
  double zy = y;
  double tmp;
  double magsq = zx * zx + zy * zy;
  uint8_t result = 0;
  while (magsq <= 4.0 && result != 255) {
    tmp = zx * zx - zy * zy + x;
    zy = 2 * zx * zy + y;
    zx = tmp;
    magsq = zx * zx + zy * zy;
    result += 1;
  }
  return result;
}

static void mandlebrot_generator(struct frak_args const* const args,
                                 void* buffer, size_t len) {
  if (args->palette == frak_palette_black_and_white) {
    fprintf(stderr, "Warning: mandlebrot doesn't support bilevel images\n");
    return;
  }
  for (size_t i = 0; i < len; i++) {
    uint32_t row = i / args->width;
    uint32_t column = i % args->width;
    double x = 4.0 * (double)column / (double)args->width - 2.0;
    double y = 4.0 * (double)row / (double)args->height - 2.0;
    *(uint8_t*)(buffer + i) = compute_mandlebrot_pixel(x, y);
  }
}

static inline void tiff_spec_init_from_frak_args(tiff_spec_t spec,
                                                 struct frak_args* args) {
  spec->width = args->width;
  spec->height = args->height;
  spec->ppi = args->ppi;
  switch (args->palette) {
    case frak_palette_color: {
      spec->type = tiff_palette;
      unsigned colors_byte_len = sizeof(uint16_t) * 3 * 256;
      spec->palette = malloc(sizeof(struct tiff_palette) + colors_byte_len);
      spec->palette->len = 3 * 256;
      arc4random_buf((void*)spec->palette->colors, colors_byte_len);
    } break;
    case frak_palette_gray:
      spec->type = tiff_gray;
      spec->palette = NULL;
      break;
    default:
      fprintf(stderr, "Unknown palette type %d, defaulting to bilevel\n",
              args->palette);
      /* fallthrough */
    case frak_palette_black_and_white:
      spec->type = tiff_bilevel;
      spec->palette = NULL;
      break;
  }
}

int main(int argc, const char* argv[]) {
  int rc = 0;
  int fd = -1;
  void* buf = NULL;
  void* data;
  struct frak_args args;
  struct tiff_spec spec;
  size_t len;

  frak_args_init(&args);
  char* err = parse_frak_args(&args, argc, argv);
  if (err) {
    fprintf(stderr, "%s\n\n", err);
    free(err);
    frak_usage();
  }

  tiff_spec_init_from_frak_args(&spec, &args);
  len = tiff_spec_compute_file_size(&spec);

  fd = open(args.name, O_RDWR | O_CREAT | O_TRUNC, 0644);
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

  data = tiff_spec_write_metadata(&spec, buf);

  void (*generator)(struct frak_args const* const, void*, size_t);
  switch (args.design) {
    case frak_design_mandlebrot:
      generator = mandlebrot_generator;
      break;
    default:
      fprintf(stderr, "Unexpected design %u, defaulting to noise\n",
              args.design);
      /* fallthrough */
    case frak_design_noise:
      generator = noise_generator;
      break;
  }
  generator(&args, data, len - (data - buf));

out:
  if (buf && buf != MAP_FAILED) {
    munmap(buf, len);
  }
  if (fd >= 0) {
    close(fd);
  }
  if (rc != 0) {
    unlink(args.name);
  }
  if (spec.palette) {
    free(spec.palette);
  }
  return rc;
}
