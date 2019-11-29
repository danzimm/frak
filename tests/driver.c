// Copywrite (c) 2019 Dan Zimmerman

#include <stdio.h>
#include <unistd.h>

#include "tests.h"

int main(int argc, const char* argv[]) {
  (void)argc;
  (void)argv;
  printf("Running frak tests... %d\n", getpid());
  return run_tests(argc < 2 ? NULL : argv[1]) ? 0 : 1;
}
