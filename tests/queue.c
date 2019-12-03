// Copywrite (c) 2019 Dan Zimmerman

#include <frakl/queue.h>
#include <pthread.h>
#include <stdint.h>

#include "tests.h"

TEST(Queue) {
  queue_t q = queue_create(3);
  EXPECT_EQ(queue_get_capacity(q), 3);
  EXPECT_EQ(queue_get_length(q), 0);
  EXPECT_TRUE(queue_is_empty(q));

  EXPECT_TRUE(queue_push(q, (void*)0x1));
  EXPECT_EQ(queue_get_length(q), 1);
  EXPECT_FALSE(queue_is_empty(q));

  queue_push(q, (void*)0x2);
  EXPECT_EQ(queue_get_length(q), 2);
  EXPECT_FALSE(queue_is_empty(q));

  queue_push(q, (void*)0x3);
  EXPECT_EQ(queue_get_length(q), 3);
  EXPECT_FALSE(queue_is_empty(q));

  EXPECT_EQ(queue_pop(q), (void*)0x1);
  EXPECT_EQ(queue_get_length(q), 2);
  EXPECT_FALSE(queue_is_empty(q));

  EXPECT_EQ(queue_pop(q), (void*)0x2);
  EXPECT_EQ(queue_get_length(q), 1);
  EXPECT_FALSE(queue_is_empty(q));

  EXPECT_EQ(queue_pop(q), (void*)0x3);
  EXPECT_EQ(queue_get_length(q), 0);
  EXPECT_TRUE(queue_is_empty(q));

  queue_push(q, (void*)0x1);
  EXPECT_EQ(queue_get_length(q), 1);
  EXPECT_FALSE(queue_is_empty(q));

  queue_push(q, (void*)0x2);
  EXPECT_EQ(queue_get_length(q), 2);
  EXPECT_FALSE(queue_is_empty(q));

  queue_push(q, (void*)0x3);
  EXPECT_EQ(queue_get_length(q), 3);
  EXPECT_FALSE(queue_is_empty(q));

  EXPECT_EQ(queue_pop(q), (void*)0x1);
  EXPECT_EQ(queue_get_length(q), 2);
  EXPECT_FALSE(queue_is_empty(q));

  EXPECT_EQ(queue_pop(q), (void*)0x2);
  EXPECT_EQ(queue_get_length(q), 1);
  EXPECT_FALSE(queue_is_empty(q));

  queue_push(q, (void*)0x5);
  EXPECT_EQ(queue_get_length(q), 2);
  EXPECT_FALSE(queue_is_empty(q));

  queue_push(q, (void*)0x6);
  EXPECT_EQ(queue_get_length(q), 3);
  EXPECT_FALSE(queue_is_empty(q));

  EXPECT_EQ(queue_pop(q), (void*)0x3);
  EXPECT_EQ(queue_get_length(q), 2);
  EXPECT_FALSE(queue_is_empty(q));

  queue_push(q, (void*)0x7);
  EXPECT_EQ(queue_get_length(q), 3);
  EXPECT_FALSE(queue_is_empty(q));

  EXPECT_EQ(queue_pop(q), (void*)0x5);
  EXPECT_EQ(queue_get_length(q), 2);
  EXPECT_FALSE(queue_is_empty(q));

  EXPECT_EQ(queue_pop(q), (void*)0x6);
  EXPECT_EQ(queue_get_length(q), 1);
  EXPECT_FALSE(queue_is_empty(q));

  queue_push(q, (void*)0x8);
  EXPECT_EQ(queue_get_length(q), 2);
  EXPECT_FALSE(queue_is_empty(q));

  EXPECT_EQ(queue_pop(q), (void*)0x7);
  EXPECT_EQ(queue_get_length(q), 1);
  EXPECT_FALSE(queue_is_empty(q));

  EXPECT_EQ(queue_pop(q), (void*)0x8);
  EXPECT_EQ(queue_get_length(q), 0);
  EXPECT_TRUE(queue_is_empty(q));

  queue_destroy(q);
}

TEST(QueuePopN) {
  void* buffer[3];

  queue_t q = queue_create(3);
  queue_push(q, (void*)0x1);
  queue_push(q, (void*)0x2);

  EXPECT_EQ(queue_pop_n(q, 2, buffer), 2);
  EXPECT_EQ(buffer[0], (void*)0x1);
  EXPECT_EQ(buffer[1], (void*)0x2);

  queue_push(q, (void*)0x3);
  queue_push(q, (void*)0x4);

  EXPECT_EQ(queue_pop_n(q, 3, buffer), 2);
  EXPECT_EQ(buffer[0], (void*)0x3);
  EXPECT_EQ(buffer[1], (void*)0x4);

  queue_push(q, (void*)0x5);

  EXPECT_EQ(queue_pop_n(q, 3, buffer), 1);
  EXPECT_EQ(buffer[0], (void*)0x5);

  EXPECT_EQ(queue_pop_n(q, 3, buffer), 0);
  EXPECT_EQ(queue_pop_n(q, 0, buffer), 0);

  queue_push(q, (void*)0x6);
  queue_push(q, (void*)0x7);
  queue_push(q, (void*)0x8);

  EXPECT_EQ(queue_pop_n(q, 4, buffer), 3);
  EXPECT_EQ(buffer[0], (void*)0x6);
  EXPECT_EQ(buffer[1], (void*)0x7);
  EXPECT_EQ(buffer[2], (void*)0x8);

  queue_push(q, (void*)0x9);

  EXPECT_EQ(queue_pop_n(q, 10, buffer), 1);
  EXPECT_EQ(buffer[0], (void*)0x9);

  queue_destroy(q);
}

TEST(QueuePushN) {
  void* pusher[3];
  void* buffer[3];

  queue_t q = queue_create(3);
  pusher[0] = (void*)0x1;
  pusher[1] = (void*)0x2;
  EXPECT_EQ(queue_push_n(q, 2, pusher), 2);

  EXPECT_EQ(queue_pop_n(q, 2, buffer), 2);
  EXPECT_EQ(buffer[0], (void*)0x1);
  EXPECT_EQ(buffer[1], (void*)0x2);

  pusher[0] = (void*)0x3;
  pusher[1] = (void*)0x4;
  EXPECT_EQ(queue_push_n(q, 2, pusher), 2);

  EXPECT_EQ(queue_pop_n(q, 3, buffer), 2);
  EXPECT_EQ(buffer[0], (void*)0x3);
  EXPECT_EQ(buffer[1], (void*)0x4);

  pusher[0] = (void*)0x5;
  EXPECT_EQ(queue_push_n(q, 1, pusher), 1);

  EXPECT_EQ(queue_pop_n(q, 1, buffer), 1);
  EXPECT_EQ(buffer[0], (void*)0x5);

  pusher[0] = (void*)0x6;
  pusher[1] = (void*)0x7;
  pusher[2] = (void*)0x8;
  EXPECT_EQ(queue_push_n(q, 4, pusher), 3);

  EXPECT_EQ(queue_pop_n(q, 3, buffer), 3);
  EXPECT_EQ(buffer[0], (void*)0x6);
  EXPECT_EQ(buffer[1], (void*)0x7);
  EXPECT_EQ(buffer[2], (void*)0x8);

  pusher[0] = (void*)0x9;
  pusher[1] = (void*)0xa;
  EXPECT_EQ(queue_push_n(q, 2, pusher), 2);
  pusher[0] = (void*)0xb;
  pusher[1] = (void*)0xc;
  EXPECT_EQ(queue_push_n(q, 2, pusher), 1);

  EXPECT_EQ(queue_pop_n(q, 3, buffer), 3);
  EXPECT_EQ(buffer[0], (void*)0x9);
  EXPECT_EQ(buffer[1], (void*)0xa);
  EXPECT_EQ(buffer[2], (void*)0xb);

  EXPECT_EQ(queue_push_n(q, 10, pusher), 3);
  EXPECT_EQ(queue_pop_n(q, 10, buffer), 3);
  EXPECT_EQ(memcmp(pusher, buffer, sizeof(pusher)), 0);

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
  queue_t q = queue_create(70000);
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
  queue_destroy(q);
}
