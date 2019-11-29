// Copywrite (c) 2019 Dan Zimmerman

#include "queue.h"

#include <assert.h>
#include <stdatomic.h>
#include <stdio.h>
#include <strings.h>

struct q_cell;
typedef struct q_cell* q_cell_t;

struct q_cell {
  void* data;
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
  uintptr_t head = atomic_load(&q->head);
  uintptr_t tail = atomic_load(&q->tail);
  if (head != tail) {
    fprintf(stderr, "Warning: destroying non-empty queue %p (%lx -> %lx)\n", q,
            tail, head);
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
  q->cells[atomic_fetch_inc_and(&q->head, q->cap_mask)].data = data;
}

void queue_pop_n(queue_t q, unsigned n, void* results[]) {
  const uintptr_t head = atomic_load(&q->head);
  uintptr_t tail = atomic_load(&q->tail);
  if (tail == head) {
    bzero(results, sizeof(void*) * n);
    return;
  }
  const uintptr_t cap_mask = q->cap_mask;
  const uintptr_t cap = cap_mask + 1;
  uintptr_t len;
  uintptr_t new_tail;
  do {
    len = head > tail ? head - tail : cap + head - tail;
    new_tail = (tail + (n < len ? n : len)) & cap_mask;
  } while (!atomic_compare_exchange_weak(&q->tail, &tail, new_tail));
  void** results_iter = results;
  for (uintptr_t i = tail; i != new_tail; i = ((i + 1) & cap_mask)) {
    struct q_cell* cell = &q->cells[i];
    *results_iter++ = cell->data;
    cell->data = NULL;
  }
}

bool queue_is_empty(queue_t q) {
  return atomic_load(&q->head) == atomic_load(&q->tail);
}

void queue_dump(queue_t q) {
  printf("Dumping queue %p\n", q);
  uintptr_t head = q->head;
  uintptr_t tail = q->tail;
  uintptr_t cap_mask = q->cap_mask;
  for (uintptr_t i = tail; i != head; i = ((i + 1) & cap_mask)) {
    printf("  At %lu (%p): %p\n", i, &q->cells[i], q->cells[i].data);
  }
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
