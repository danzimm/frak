// Copywrite (c) 2019 Dan Zimmerman

#include "tests.h"

#include <assert.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct test {
  const char* name;
  test_cb_t cb;
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

void register_test(const char* name, test_cb_t cb) {
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
}

bool run_tests(void) {
  volatile bool any_fail = false;
  struct test const* volatile iter = g_test_list.tests;
  struct test const* const volatile end = iter + g_test_list.length;
  if (iter == end) {
    printf("Warning: No tests ran\n");
    return true;
  }
  do {
    if (setjmp(g_jmp_env) == 0) {
      printf("  %s", iter->name);
      iter->cb();
      printf("\n");
    } else {
      printf(" Failure: %s at %s:%d\n", g_failure.check, g_failure.file,
             g_failure.line);
      any_fail = true;
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
