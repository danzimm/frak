// Copywrite (c) 2019 Dan Zimmerman

#include <fcntl.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>
#include <zlib.h>
#if __LINUX__
#include <sys/random.h>
#endif

#include "frak_args.h"
#include "frakl/tiff.h"

static void noise_generator(struct frak_args const* const args, void* buffer,
                            size_t len) {
  (void)args;
#if __APPLE__
  arc4random_buf(buffer, len);
#elif __LINUX__
  getrandom(buffer, len);
#endif
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
  return (255 * result) / max_iteration;
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

// n-atic:
// ax^n + c = y
// a(0)^n + c = from
// a(len)^n + from = to
// => a = (to - from) / len^n
// y = ((to - from) / len^n)x^n + from
static void fill_color_palette(struct tiff_palette_color* colors, unsigned len,
                               struct tiff_palette_color* from,
                               struct tiff_palette_color* to, uint32_t curve) {
  if (!len) {
    return;
  }

  double r = (double)from->red;
  double g = (double)from->green;
  double b = (double)from->blue;
  double c = (double)curve;

  double rstep = ((double)to->red - r) / pow((double)len, c);
  double gstep = ((double)to->green - g) / pow((double)len, c);
  double bstep = ((double)to->blue - b) / pow((double)len, c);

  unsigned x = 0;
  do {
    struct tiff_palette_color* color = &colors[x];
    color->red = (uint16_t)(r + rstep * pow((double)x, c));
    color->green = (uint16_t)(g + gstep * pow((double)x, c));
    color->blue = (uint16_t)(b + bstep * pow((double)x, c));
    r += rstep;
    g += gstep;
    b += bstep;
  } while ((++x) != len);
}

static struct tiff_palette_color frak_color_to_tiff(struct frak_color* color) {
  return (struct tiff_palette_color){
      .red = 65535 * color->red / 255,
      .green = 65535 * color->green / 255,
      .blue = 65535 * color->blue / 255,
  };
}

static inline void tiff_spec_init_from_frak_args(tiff_spec_t spec,
                                                 struct frak_args* args) {
  spec->width = args->width;
  spec->height = args->height;
  spec->ppi = args->ppi;
  switch (args->palette) {
    case frak_palette_color:
    case frak_palette_custom: {
      spec->type = tiff_palette;
      unsigned colors_byte_len = sizeof(uint16_t) * 3 * 256;
      spec->palette = calloc(1, sizeof(struct tiff_palette) + colors_byte_len);
      spec->palette->len = 3 * 256;
      if (args->palette == frak_palette_color) {
        arc4random_buf((void*)spec->palette->colors, colors_byte_len);
      } else {
        struct frak_color* fcol = args->colors->colors;
        struct tiff_palette_color* pcol = spec->palette->colors;

        struct frak_color* from_fcol = fcol;
        struct frak_color* to_fcol = fcol + 1;
        struct frak_color const* const end_fcol = fcol + args->colors->count;

        uint16_t from_i;
        uint16_t to_i;
        struct tiff_palette_color from = frak_color_to_tiff(from_fcol);
        struct tiff_palette_color to;
        do {
          from_i = from_fcol->i;
          to_i = to_fcol->i;
          to = frak_color_to_tiff(to_fcol);
          fill_color_palette(pcol + from_i, to_i - from_i, &from, &to,
                             args->curve);
          from = to;
          from_fcol = to_fcol;
        } while ((++to_fcol) != end_fcol);
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
  int o_flags = 0;

  frak_args_init(&args);
  char* err = parse_frak_args(&args, argc, argv);
  if (err) {
    fprintf(stderr, "%s\n\n", err);
    free(err);
    frak_usage();
  }
  max_iteration = args.max_iteration;

  tiff_spec_init_from_frak_args(&spec, &args);
  if (args.palette_only) {
    o_flags = O_RDWR;
  } else {
    o_flags = O_RDWR | O_CREAT | O_TRUNC;
  }

  fd = open(args.name, o_flags, 0644);
  if (fd < 0) {
    perror("open");
    rc = 1;
    goto out;
  }

  if (args.palette_only) {
    struct stat st;
    if (fstat(fd, &st) != 0) {
      perror("fstat");
      rc = 1;
      goto out;
    }
    len = st.st_size;
  } else {
    len = tiff_spec_compute_file_size(&spec);
    if ((rc = ftruncate(fd, len)) != 0) {
      perror("truncate");
      goto out;
    }
  }

  buf = mmap(NULL, len, PROT_WRITE | PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
  if (!buf || buf == MAP_FAILED) {
    perror("mmap");
    rc = 1;
    goto out;
  }

  if (args.palette_only) {
    const char* err = tiff_update_color_palette(&spec, buf);
    if (err) {
      fprintf(stderr, "Failed to update color palette: %s\n", err);
      rc = 1;
    }
  } else {
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
  }

out:
  if (buf && buf != MAP_FAILED) {
    munmap(buf, len);
  }
  if (fd >= 0) {
    close(fd);
  }
  if (rc != 0 && (o_flags & O_CREAT) != 0) {
    unlink(args.name);
  }
  if (spec.palette) {
    free(spec.palette);
  }
  return rc;
}
