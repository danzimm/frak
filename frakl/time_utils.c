// Copywrite (c) 2019 Dan Zimmerman

#include "time_utils.h"

#include <assert.h>

void timespec_minus(struct timespec* a, struct timespec const* b) {
  assert(a && b);
  time_t sec = a->tv_sec - b->tv_sec;
  long nsec = a->tv_nsec - b->tv_nsec;
  while (nsec < 0) {
    nsec += 1000000000;
    sec -= 1;
  }
  a->tv_sec = sec;
  a->tv_nsec = nsec;
}

void timespec_add(struct timespec* a, struct timespec const* b) {
  assert(a && b);
  a->tv_sec += b->tv_sec;
  long long nsec = a->tv_nsec + b->tv_nsec;
  a->tv_sec += nsec / 1000000000;
  a->tv_nsec = nsec % 1000000000;
}

void timespec_divide(struct timespec* a, unsigned n) {
  long long total_nsec = a->tv_sec * 1000000000 + a->tv_nsec;
  total_nsec /= n;
  a->tv_sec = total_nsec / 1000000000;
  a->tv_nsec = total_nsec % 1000000000;
}

void timespec_avg(struct timespec* dst, struct timespec const* ts,
                  unsigned cnt) {
  dst->tv_sec = 0;
  dst->tv_nsec = 0;
  struct timespec const* iter = ts;
  struct timespec const* const end = ts + cnt;
  if (iter == end) {
    return;
  }
  do {
    timespec_add(dst, iter);
  } while (++iter != end);
  timespec_divide(dst, cnt);
}
