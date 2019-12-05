// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include <stdbool.h>
#include <stdio.h>
#include <string.h>

typedef void (*test_cb_t)(void);

void register_test(const char* name, test_cb_t test, bool expect_fail,
                   const char* file, int line);
bool run_tests(const char* filter);
void fail(const char* file, int line, const char* fmt, ...);

#define TEST_(name, expect_fail)                                         \
  static void _test_##name(void);                                        \
  static __attribute__((constructor)) void _test_##name##_init(void) {   \
    register_test(#name, _test_##name, expect_fail, __FILE__, __LINE__); \
  }                                                                      \
  static void _test_##name(void)

#define TEST(name) TEST_(name, false)
#define TEST_FAIL(name) TEST_(name, true)

#define EXPECT_(cond, ...)                   \
  do {                                       \
    if (!(cond)) {                           \
      fail(__FILE__, __LINE__, __VA_ARGS__); \
    }                                        \
  } while (0)

#define EXPECT_EQ(x, y) EXPECT_((x) == (y), #x " != " #y)
#define EXPECT_TRUE(x) EXPECT_(!!(x), "!" #x)
#define EXPECT_FALSE(x) EXPECT_(!(x), #x)
#define EXPECT_STREQ(x, y)                                           \
  EXPECT_((x == NULL && y == NULL) || (x && y && strcmp(x, y) == 0), \
          #x " != " #y)
