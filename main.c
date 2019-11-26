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

static uint32_t max_iteration = 1000;

// c = x + iy
// m_c(z) = z^2 + c
//        = z_x^2 - z_y^2 + x + i(2z_x * z_y + y)
static uint8_t compute_mandlebrot_pixel(double x, double y) {
  double zx = x;
  double zy = y;
  double tmp;
  double magsq = zx * zx + zy * zy;
  uint32_t result = 0;
  const uint32_t max = max_iteration;
  while (magsq <= 4.0 && result != max) {
    tmp = zx * zx - zy * zy + x;
    zy = 2 * zx * zy + y;
    zx = tmp;
    magsq = zx * zx + zy * zy;
    result += 1;
  }
  return (256 * result) / max_iteration;
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

static void fill_color_palette(struct tiff_palette_color* colors, unsigned len,
                               struct tiff_palette_color* from,
                               struct tiff_palette_color* to) {
  if (!len) {
    return;
  }

  double r = (double)from->red;
  double g = (double)from->green;
  double b = (double)from->blue;

  double rstep = ((double)to->red - r) / (double)len;
  double gstep = ((double)to->green - g) / (double)len;
  double bstep = ((double)to->blue - b) / (double)len;

  struct tiff_palette_color* iter = colors;
  struct tiff_palette_color* const end = colors + len;
  do {
    iter->red = (uint16_t)r;
    iter->green = (uint16_t)g;
    iter->blue = (uint16_t)b;
    r += rstep;
    g += gstep;
    b += bstep;
  } while ((++iter) != end);
}

static inline void tiff_spec_init_from_frak_args(tiff_spec_t spec,
                                                 struct frak_args* args) {
  spec->width = args->width;
  spec->height = args->height;
  spec->ppi = args->ppi;
  switch (args->palette) {
    case frak_palette_color:
    case frak_palette_blue: {
      spec->type = tiff_palette;
      unsigned colors_byte_len = sizeof(uint16_t) * 3 * 256;
      spec->palette = malloc(sizeof(struct tiff_palette) + colors_byte_len);
      spec->palette->len = 3 * 256;
      if (args->palette == frak_palette_color) {
        arc4random_buf((void*)spec->palette->colors, colors_byte_len);
      } else {
        static struct tiff_palette_color from = {
            .red = 256 * 0,
            .green = 256 * 0,
            .blue = 256 * 0,
        };
        static struct tiff_palette_color to = {
            .red = 256 * 0,
            .green = 256 * 97,
            .blue = 256 * 255,
        };
        fill_color_palette(spec->palette->colors, 256, &from, &to);
      }
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
  max_iteration = args.max_iteration;

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
