// Copywrite (c) 2019 Dan Zimmerman

#include "queue.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <strings.h>

struct q_cell;
typedef struct q_cell* q_cell_t;

struct q_cell {
  _Atomic(void*) data;
};

struct queue {
  _Atomic(uintptr_t) head;
  _Atomic(uintptr_t) tail;
  uintptr_t cap_mask;
  struct q_cell cells[];
};

queue_t queue_create(uint8_t cap_pow) {
  if (cap_pow == 0 || cap_pow > 31) {
    cap_pow = 16;
  }
  size_t cap = 1 << cap_pow;
  queue_t res = malloc(sizeof(struct queue) + cap * sizeof(struct q_cell));
  res->cap_mask = cap - 1;
  atomic_init(&res->head, 0);
  atomic_init(&res->tail, 0);
  bzero(res->cells, sizeof(struct q_cell) * cap);
  return res;
}

void queue_destroy(queue_t q) {
  if (atomic_load(&q->head) != atomic_load(&q->tail)) {
    fprintf(stderr, "Warning: destroying non-empty queue %p\n", q);
  }
  free(q);
}

uintptr_t atomic_fetch_inc_and(_Atomic(uintptr_t) * value, uintptr_t mask) {
  uintptr_t val = atomic_fetch_add(value, 1) + 1;
  if (val != (val & mask)) {
    uintptr_t unused = atomic_fetch_and(value, mask);
    (void)unused;
  }
  return (val - 1) & mask;
}

void queue_push(queue_t q, void* data) {
  atomic_store(&q->cells[atomic_fetch_inc_and(&q->head, q->cap_mask)].data,
               data);
}

void* queue_pop(queue_t q) {
  return atomic_exchange(
      &q->cells[atomic_fetch_inc_and(&q->tail, q->cap_mask)].data, NULL);
}

bool queue_is_empty(queue_t q) {
  return atomic_load(&q->head) == atomic_load(&q->tail);
}

unsigned queue_get_length(queue_t q) {
  uintptr_t head = atomic_load(&q->head);
  uintptr_t tail = atomic_load(&q->tail);
  if (head >= tail) {
    return head - tail;
  } else {
    return (q->cap_mask + 1) + head - tail;
  }
}

size_t queue_get_capacity(queue_t q) { return q->cap_mask; }
