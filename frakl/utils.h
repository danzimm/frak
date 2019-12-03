// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

#if UINTPTR_MAX == 0xFFFFFFFFFFFFFFFFu
#define __round_clz __builtin_clzll
#else
#define __round_clz __builtin_clz
#endif

static inline uintptr_t round_to_next_power_of_two(uintptr_t x) {
  if (!x) {
    return 0;
  }
  const uintptr_t max_bit_set = sizeof(uintptr_t) * 8 - __round_clz(x) - 1;
  assert(max_bit_set < sizeof(uintptr_t) * 8);
  const uintptr_t max_bit_value = (uintptr_t)1 << max_bit_set;
  if (max_bit_value == x) {
    return max_bit_value;
  } else {
    return max_bit_value * 2;
  }
}
