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

#if !__APPLE__
#include <time.h>
#endif

#include "frak_args.h"
#include "frakl/tiff.h"
#include "frakl/time_utils.h"
#include "frakl/wq.h"

static void fill_random(void* buffer, size_t len) {
#if __APPLE__
  arc4random_buf(buffer, len);
#else
  srandom(time(NULL));
  void* iter = buffer;
  void* const end = iter + len;
  if (iter == end) {
    return;
  }
  long int buf;
  while (iter < end) {
    buf = random();
    unsigned len = ((long)sizeof(long int) < (end - iter)) ? sizeof(long int)
                                                           : (end - iter);
    memcpy(iter, &buf, len);
    iter += len;
  }
#endif
}

#if __APPLE__
#define base_random arc4random
#else
#define base_random random
#endif

static uint32_t give_me_random(uint32_t max) {
  return ((uint64_t)base_random() * max) >> 32;
}

static uint32_t max_iteration = 1000;

// c = x + iy
// m_c(z) = z^2 + c
//        = z_x^2 - z_y^2 + x + i(2z_x * z_y + y)
static uint8_t mandlebrot_pixel(uint32_t column, uint32_t row, uint32_t width,
                                uint32_t height) {
  double x = 4.0 * (double)column / (double)width - 2.0;
  double y = 4.0 * (double)row / (double)height - 2.0;

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

struct pixel_worker_ctx {
  struct frak_args const* const args;
  void* buffer;
};

static void mandlebrot_worker(void** pixels, unsigned n,
                              struct pixel_worker_ctx* ctx) {
  struct frak_args const* const args = ctx->args;

  const uint32_t width = args->width;
  const uint32_t height = args->height;
  void** iter = pixels;
  void* const* const end = iter + n;
  do {
    void* pixel = *iter;
    uint32_t i = (uint32_t)(pixel - ctx->buffer);
    uint32_t row = i / width;
    uint32_t column = i % width;
    *(uint8_t*)pixel = mandlebrot_pixel(column, row, width, height);
  } while (++iter != end);
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
        fill_random((void*)spec->palette->colors, colors_byte_len);
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
    default:
      fprintf(stderr, "Unknown palette type %d, defaulting to bilevel\n",
              args->palette);
      /* fallthrough */
    case frak_palette_gray:
      spec->type = tiff_gray;
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

  struct timespec start;
  struct timespec init;
  struct timespec mmap_img;
  struct timespec meta;
  struct timespec init_queue;
  struct timespec compute_data;
  struct timespec baseis;
  struct timespec initis;
  struct timespec randomizeis;
  struct timespec pushis;

  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  frak_args_init(&args);
  char* err = parse_frak_args(&args, argc, argv);
  if (err || args.print_help) {
    if (!args.print_help) {
      fprintf(stderr, "%s\n\n", err);
      free(err);
    }
    frak_usage(args.print_help ? 0 : 1);
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
  if (args.stats) {
    clock_gettime(CLOCK_MONOTONIC_RAW, &init);
  }

  buf = mmap(NULL, len, PROT_WRITE | PROT_READ, MAP_FILE | MAP_SHARED, fd, 0);
  if (!buf || buf == MAP_FAILED) {
    perror("mmap");
    rc = 1;
    goto out;
  }
  if (args.stats) {
    clock_gettime(CLOCK_MONOTONIC_RAW, &mmap_img);
  }

  if (args.palette_only) {
    const char* err = tiff_update_color_palette(&spec, buf);
    if (err) {
      fprintf(stderr, "Failed to update color palette: %s\n", err);
      rc = 1;
    }
    if (args.stats) {
      clock_gettime(CLOCK_MONOTONIC_RAW, &meta);
      clock_gettime(CLOCK_MONOTONIC_RAW, &init_queue);
      clock_gettime(CLOCK_MONOTONIC_RAW, &compute_data);
    }
  } else {
    data = tiff_spec_write_metadata(&spec, buf);
    if (args.stats) {
      clock_gettime(CLOCK_MONOTONIC_RAW, &meta);
    }

    struct pixel_worker_ctx ctx = {
        .args = &args,
        .buffer = data,
    };
    const uint32_t work_count = args.width * args.height;
    wq_t wq = wq_create("frak", (void*)mandlebrot_worker, args.worker_count,
                        32 - __builtin_clz(work_count));
    wq_set_worker_cache_size(wq, args.worker_cache_size);

    if (args.randomize) {
      uint32_t* is = calloc(work_count, sizeof(uint32_t));
      if (args.stats) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &baseis);
      }
      for (uint32_t i = 0; i < work_count; i++) {
        is[i] = i;
      }
      if (args.stats) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &initis);
      }
      for (uint32_t i = work_count; i > 0; i--) {
        uint32_t j = give_me_random(i);
        uint32_t tmp = is[j];
        is[j] = is[i];
        is[i] = tmp;
      }
      if (args.stats) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &randomizeis);
      }
      for (uint32_t i = 0; i < work_count; i++) {
        wq_push(wq, data + is[i]);
      }
      if (args.stats) {
        clock_gettime(CLOCK_MONOTONIC_RAW, &pushis);
      }
      free(is);
    } else {
      for (uint32_t i = 0; i < work_count; i++) {
        wq_push(wq, data + i);
      }
    }
    if (args.stats) {
      clock_gettime(CLOCK_MONOTONIC_RAW, &init_queue);
    }

    wq_start(wq, &ctx);
    wq_wait(wq);
    if (args.stats) {
      clock_gettime(CLOCK_MONOTONIC_RAW, &compute_data);
    }
    wq_destroy(wq);
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
  if (args.stats) {
    timespec_minus(&compute_data, &init_queue);
    timespec_minus(&init_queue, &meta);
    timespec_minus(&meta, &mmap_img);
    timespec_minus(&mmap_img, &init);
    timespec_minus(&init, &start);

    if (args.randomize) {
      timespec_minus(&pushis, &randomizeis);
      timespec_minus(&randomizeis, &initis);
      timespec_minus(&initis, &baseis);
    }

#define max(a, b) ((a) < (b) ? (a) : (b))
#define maxdigi(x)                               \
  do {                                           \
    int _tmp = x != 0 ? (int)ceil(log10(x)) : 1; \
    if (_tmp > ndigits) {                        \
      ndigits = _tmp;                            \
    }                                            \
  } while (0)

    int ndigits = 1;
    maxdigi(timespec_to_ms(&init));
    maxdigi(timespec_to_ms(&mmap_img));
    maxdigi(timespec_to_ms(&meta));
    maxdigi(timespec_to_ms(&init_queue));
    maxdigi(timespec_to_ms(&compute_data));
    if (args.randomize) {
      maxdigi(timespec_to_ms(&initis));
      maxdigi(timespec_to_ms(&randomizeis));
      maxdigi(timespec_to_ms(&pushis));
    }

    printf(
        "Timing:\n  init: %*ld\n  mmap: %*ld\n  meta: %*ld\n  qini: %*ld\n  "
        "comp: %*ld\n",
        ndigits, timespec_to_ms(&init), ndigits, timespec_to_ms(&mmap_img),
        ndigits, timespec_to_ms(&meta), ndigits, timespec_to_ms(&init_queue),
        ndigits, timespec_to_ms(&compute_data));
    if (args.randomize) {
      printf("\n  inii: %*ld\n  shfi: %*ld\n  push: %*ld\n", ndigits,
             timespec_to_ms(&initis), ndigits, timespec_to_ms(&randomizeis),
             ndigits, timespec_to_ms(&pushis));
    }
  }
  return rc;
}
