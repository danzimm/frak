// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include <stdbool.h>

typedef void (*test_cb_t)(void);

void register_test(const char* name, test_cb_t test);
bool run_tests(void);
void fail(const char* check, const char* file, int line);

#define TEST(name)                                                     \
  static void _test_##name(void);                                      \
  static __attribute__((constructor)) void _test_##name##_init(void) { \
    register_test(#name, _test_##name);                                \
  }                                                                    \
  static void _test_##name(void)

#define EXPECT_EQ(x, y)                       \
  do {                                        \
    if (x != y) {                             \
      fail(#x " != " #y, __FILE__, __LINE__); \
    }                                         \
  } while (0)
