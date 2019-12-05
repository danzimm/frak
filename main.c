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
#include "frakl/fractal.h"
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
  struct fractal_ctx ctx;

  struct timespec start;
  struct timespec init;
  struct timespec mmap_img;
  struct timespec meta;
  struct timespec init_queue;
  struct timespec compute_data;

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
    ctx.width = args.width;
    ctx.height = args.height;
    ctx.fwidth = args.fwidth;
    ctx.fheight = args.fwidth * (double)args.height / (double)args.width;
    ctx.fleft = args.center[0] - ctx.fwidth / 2.0;
    ctx.ftop = args.center[1] - ctx.fheight / 2.0;
    ctx.buffer = data;
    ctx.max_iteration = args.max_iteration;

    const uintptr_t work_count = args.width * args.height;
    wq_t wq =
        wq_create("frak", (void*)fractal_worker, args.worker_count, work_count);
    wq_set_worker_cache_size(wq, args.worker_cache_size);
    wq_push_n(wq, work_count, NULL);
    if (args.stats) {
      clock_gettime(CLOCK_MONOTONIC_RAW, &init_queue);
    }

    if (!args.no_compute) {
      wq_start(wq, &ctx);
      wq_wait(wq);
    }

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

    printf(
        "Timing:\n  init: %*ld\n  mmap: %*ld\n  meta: %*ld\n  qini: %*ld\n  "
        "comp: %*ld\n",
        ndigits, timespec_to_ms(&init), ndigits, timespec_to_ms(&mmap_img),
        ndigits, timespec_to_ms(&meta), ndigits, timespec_to_ms(&init_queue),
        ndigits, timespec_to_ms(&compute_data));
  }
  return rc;
}
