// Copywrite (c) 2019 Dan Zimmerman

#include "tests.h"

#include <assert.h>
#include <frakl/time_utils.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct test {
  const char* name;
  test_cb_t cb;
  bool expect_fail;
  const char* file;
  int line;
} * test_t;

struct test_list {
  unsigned length;
  unsigned capacity;
  test_t tests;
};

typedef struct test_failure {
  const char* check;
  const char* file;
  int line;
} * test_failure_t;

static struct test_list g_test_list = {0, 0, NULL};
static jmp_buf g_jmp_env;
static struct test_failure g_failure = {NULL, NULL, 0};

void register_test(const char* name, test_cb_t cb, bool expect_fail,
                   const char* file, int line) {
  unsigned new_len = g_test_list.length + 1;
  if (g_test_list.capacity < new_len) {
    unsigned new_cap = g_test_list.capacity;
    if (new_cap == 0) {
      new_cap = 32;
    } else {
      new_cap *= 2;
    }
    g_test_list.tests =
        realloc(g_test_list.tests, sizeof(struct test) * new_cap);
    g_test_list.capacity = new_cap;
  }
  g_test_list.length = new_len;
  test_t new_test = &g_test_list.tests[new_len - 1];
  new_test->name = name;
  new_test->cb = cb;
  new_test->expect_fail = expect_fail;
  new_test->file = file;
  new_test->line = line;
}

bool run_tests(void) {
  volatile bool any_fail = false;
  struct test const* volatile iter = g_test_list.tests;
  struct test const* const volatile end = iter + g_test_list.length;
  if (iter == end) {
    printf("Warning: No tests ran\n");
    return true;
  }
  volatile struct timespec tstart;
  volatile struct timespec tend;
  do {
    g_failure.check = iter->name;
    g_failure.file = iter->file;
    g_failure.line = iter->line;
    volatile bool success;

    clock_gettime(CLOCK_MONOTONIC_RAW, (void*)&tstart);
    if (setjmp(g_jmp_env) == 0) {
      iter->cb();
      success = !iter->expect_fail;
    } else {
      success = iter->expect_fail;
    }
    clock_gettime(CLOCK_MONOTONIC_RAW, (void*)&tend);
    timespec_minus((void*)&tend, (void*)&tstart);
    printf("[%s][%3ld] %s", success ? "SUCC" : "FAIL",
           tend.tv_sec * 1000 + tend.tv_nsec / 1000000, iter->name);
    if (!success || iter->expect_fail) {
      printf(": %s at %s:%d\n", g_failure.check, g_failure.file,
             g_failure.line);
    } else {
      puts("");
    }
  } while (++iter != end);
  return !any_fail;
}

void fail(const char* check, const char* file, int line) {
  g_failure.check = check;
  g_failure.file = file;
  g_failure.line = line;
  longjmp(g_jmp_env, 1);
  assert(false && "Unreachable fail()");
}
