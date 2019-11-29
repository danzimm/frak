// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

struct queue;
typedef struct queue* queue_t;

// cap_pow is the capacity as specified as a power of 2, i.e. for a capacity of
// 4 the caller should pass 2.
queue_t queue_create(uint8_t cap_pow);

void queue_destroy(queue_t q);

void queue_push(queue_t q, void* data);

void queue_pop_n(queue_t q, unsigned n, void* results[]);

static inline void* queue_pop(queue_t q) {
  void* buffer[1];
  queue_pop_n(q, 1, buffer);
  return buffer[0];
}

void queue_dump(queue_t q);

bool queue_is_empty(queue_t q);

unsigned queue_get_length(queue_t q);

size_t queue_get_capacity(queue_t q);
