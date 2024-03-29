// Copywrite (c) 2019 Dan Zimmerman

#include <frakl/time_utils.h>
#include <frakl/wq.h>
#include <unistd.h>

#include "tests.h"

static void computer(uint8_t** cells, unsigned n, uint8_t* base) {
  uint8_t* const* const end = cells + n;
  do {
    uint8_t* cell = *cells;
    unsigned diff = cell - base;
    unsigned data = diff;
    diff *= diff;
    for (unsigned i = 0; i < diff; i++) {
      data += 37 * data / 43 + 100;
    }
    uint8_t x = data & 255;
    *cell = x;
  } while (++cells != end);
}

static void _test_wq_internal(bool use_local_cache) {
  uint8_t buffer[512];
  struct timespec start;
  struct timespec concurrent;
  struct timespec serial;

  wq_t wq = wq_create("test", (void*)computer, 0, 512);
  wq_set_worker_cache_size(wq, use_local_cache ? (uint32_t)-1 : 1);

  void* q_items[sizeof(buffer) / 2];
  for (unsigned i = 0; i < sizeof(buffer) / 2; i++) {
    EXPECT_TRUE(wq_push(wq, buffer + i));
    q_items[i] = buffer + sizeof(buffer) / 2 + i;
  }
  EXPECT_EQ(wq_push_n(wq, sizeof(buffer) / 2, q_items), sizeof(buffer) / 2);

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
  printf("  Serial vs Concurrent: %3ldms vs %3ldms\n", timespec_to_ms(&serial),
         timespec_to_ms(&concurrent));
  wq_destroy(wq);
}

TEST(WorkqueueNoLocalCache) { _test_wq_internal(false); }

TEST(WorkqueueLocalCache) { _test_wq_internal(true); }
