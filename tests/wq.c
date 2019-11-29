// Copywrite (c) 2019 Dan Zimmerman

#include <frakl/time_utils.h>
#include <frakl/wq.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#include "tests.h"

static void computer(uint8_t* cell, uint8_t* base) {
  unsigned diff = cell - base;
  unsigned data = diff;
  diff *= diff;
  for (unsigned i = 0; i < diff; i++) {
    data += 37 * data / 43 + 100;
  }
  uint8_t x = data & 255;
  *cell = x;
}

TEST(Workqueue) {
  uint8_t buffer[512];
  struct timespec start;
  struct timespec concurrent;
  struct timespec serial;

  wq_t wq = wq_create("test", 0, 0);
  for (unsigned i = 0; i < sizeof(buffer); i++) {
    wq_push(wq, (void*)computer, buffer + i);
  }

  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  wq_start(wq, buffer);
  bool was_running = wq_is_running(wq);
  wq_wait(wq);
  clock_gettime(CLOCK_MONOTONIC_RAW, &concurrent);
  timespec_minus(&concurrent, &start);

  // We put this down here so we don't inflate the time taken to run
  // concurrently.
  EXPECT_TRUE(was_running);
  EXPECT_FALSE(wq_is_running(wq));

  clock_gettime(CLOCK_MONOTONIC_RAW, &start);
  for (unsigned i = 0; i < sizeof(buffer); i++) {
    unsigned diff = i;
    unsigned data = diff;
    diff *= diff;
    for (unsigned j = 0; j < diff; j++) {
      data += 37 * data / 43 + 100;
    }
    EXPECT_EQ(buffer[i], data & 255);
  }
  clock_gettime(CLOCK_MONOTONIC_RAW, &serial);
  timespec_minus(&serial, &start);

  struct timespec delta = serial;
  timespec_minus(&delta, &concurrent);

  // Note that serial took longer than concurrent, i.e. wq works!
#if !defined(__has_feature) || !__has_feature(thread_sanitizer)
  EXPECT_TRUE(sysconf(_SC_NPROCESSORS_ONLN) <= 1 ||
              (delta.tv_sec >= 0 && delta.tv_nsec >= 0));
#endif
  printf("  Serial vs Concurrent: %3ldms vs %3ldms\n",
         serial.tv_sec * 1000 + serial.tv_nsec / 1000000,
         concurrent.tv_sec * 1000 + concurrent.tv_nsec / 1000000);
  wq_destroy(wq);
}
