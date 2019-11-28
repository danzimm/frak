// Copywrite (c) 2019 Dan Zimmerman

#include <frakl/queue.h>
#include <pthread.h>
#include <stdint.h>

#include "tests.h"

TEST(Queue) {
  queue_t q = queue_create();
  queue_push(q, (void*)0x1);
  queue_push(q, (void*)0x2);
  queue_push(q, (void*)0x3);
  queue_push(q, (void*)0x4);
  EXPECT_EQ(queue_get_length(q), 4);
  EXPECT_FALSE(queue_is_empty(q));
  EXPECT_EQ(queue_pop(q), (void*)0x1);
  EXPECT_FALSE(queue_is_empty(q));
  EXPECT_EQ(queue_pop(q), (void*)0x2);
  EXPECT_FALSE(queue_is_empty(q));
  EXPECT_EQ(queue_pop(q), (void*)0x3);
  EXPECT_FALSE(queue_is_empty(q));
  EXPECT_EQ(queue_pop(q), (void*)0x4);
  EXPECT_TRUE(queue_is_empty(q));
  EXPECT_EQ(queue_get_length(q), 0);
  queue_destroy(q);
}

static void* queue_pusher(queue_t q) {
  for (unsigned i = 0; i < 10000; i++) {
    queue_push(q, (void*)(uintptr_t)(i + 1));
  }
  return NULL;
}

static void* queue_popper(queue_t q) {
  for (unsigned i = 0; i < 10000; i++) {
    queue_pop(q);
  }
  return NULL;
}

TEST(QueueMultiThread) {
  queue_t q = queue_create();
  pthread_t t[5];
  for (unsigned i = 0; i < 5; i++) {
    EXPECT_EQ(pthread_create(&t[i], NULL, (void*)queue_pusher, q), 0);
  }
  for (unsigned i = 5; i > 0; --i) {
    EXPECT_EQ(pthread_join(t[i - 1], NULL), 0);
  }
  EXPECT_EQ(queue_get_length(q), 50000);

  for (unsigned i = 0; i < 5; i++) {
    EXPECT_EQ(
        pthread_create(
            &t[i], NULL,
            ((i & 1) != 0) ? (void*)queue_pusher : (void*)queue_popper, q),
        0);
  }
  for (unsigned i = 5; i > 0; --i) {
    pthread_join(t[i - 1], NULL);
  }
  EXPECT_EQ(queue_get_length(q), 40000);

  for (unsigned i = 0; i < 4; i++) {
    pthread_create(&t[i], NULL, (void*)queue_popper, q);
  }
  for (unsigned i = 4; i != 0; i--) {
    pthread_join(t[i - 1], NULL);
  }
  EXPECT_TRUE(queue_is_empty(q));
}
