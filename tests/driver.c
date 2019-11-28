// Copywrite (c) 2019 Dan Zimmerman

#include <stdio.h>

#include "tests.h"

int main(int argc, const char* argv[]) {
  (void)argc;
  (void)argv;
  printf("Running frak tests...\n");
  return run_tests() ? 0 : 1;
}
