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

static inline void tiff_spec_init_from_frak_args(tiff_spec_t spec,
                                                 struct frak_args* args) {
  spec->width = args->width;
  spec->height = args->height;
  spec->ppi = args->ppi;
  spec->type = args->gray ? tiff_gray : tiff_bilevel;
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
  char* err = parse_args(argc - 1, argv + 1, frak_arg_specs, &args);
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
  arc4random_buf(data, len - (data - buf));

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
  return rc;
}
