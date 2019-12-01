// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include <time.h>

void timespec_minus(struct timespec* dst, struct timespec const* src);

void timespec_add(struct timespec* dst, struct timespec const* src);

void timespec_divide(struct timespec* dst, unsigned n);

void timespec_avg(struct timespec* dst, struct timespec const* src,
                  unsigned cnt);

#define timespec_to_ms(ts) ((ts)->tv_sec * 1000 + (ts)->tv_nsec / 1000000)
