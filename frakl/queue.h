// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

struct queue;
typedef struct queue* queue_t;

queue_t queue_create(uintptr_t max_cap);

void queue_destroy(queue_t q);

unsigned queue_push_n(queue_t q, unsigned n, void* data[]);

static inline bool queue_push(queue_t q, void* x) {
  void* buffer[1] = {x};
  return queue_push_n(q, 1, buffer) == 1 ? true : false;
}

unsigned queue_pop_n(queue_t q, unsigned n, void* results[]);

static inline void* queue_pop(queue_t q) {
  void* buffer[1];
  return queue_pop_n(q, 1, buffer) == 1 ? buffer[0] : NULL;
}

void queue_dump(queue_t q);

bool queue_is_empty(queue_t q);

unsigned queue_get_length(queue_t q);

size_t queue_get_capacity(queue_t q);
