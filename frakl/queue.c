// Copywrite (c) 2019 Dan Zimmerman

#include "queue.h"

#include <stdatomic.h>
#include <stdio.h>

struct q_cell;
typedef struct q_cell* q_cell_t;
typedef _Atomic(q_cell_t) q_cell_t_a;

struct q_cell {
  q_cell_t next;
  void* data;
};

struct queue {
  q_cell_t_a head;
  q_cell_t_a tail;
};

queue_t queue_create(void) { return calloc(1, sizeof(struct queue)); }

void queue_destroy(queue_t q) {
  q_cell_t cell = atomic_load(&q->tail);
  if (cell != NULL) {
    fprintf(stderr, "Warning: destroying non-empty queue %p\n", q);
    q_cell_t next;
    do {
      next = cell->next;
      free(cell);
      cell = next;
    } while (cell != NULL);
  }
  free(q);
}

void queue_push(queue_t q, void* data) {
  // Create the new cell
  q_cell_t cell = malloc(sizeof(struct q_cell));
  cell->data = data;
  cell->next = NULL;

  q_cell_t tail = NULL;
  if (!atomic_compare_exchange_strong(&q->tail, &tail, cell)) {
    q_cell_t prev_head = atomic_load(&q->head);
    // If somehow we got ahead of the thread that set tail then we need to wait
    // for it to set head as well.
    if (!prev_head) {
      while ((prev_head = atomic_load(&q->head)) == NULL)
        ;
    }
    while (!atomic_compare_exchange_weak(&q->head, &prev_head, cell))
      ;
    // Since q->head is set atomically we have a "lock" on prev_head here.
    prev_head->next = cell;
  } else {
    atomic_store(&q->head, cell);
  }
}

void* queue_pop(queue_t q) {
  q_cell_t pop;
  q_cell_t old_head;
  do {
    pop = atomic_load(&q->tail);
    if (!pop->next) {
      old_head = atomic_load(&q->head);
    }
  } while (pop && !atomic_compare_exchange_weak(&q->tail, &pop, pop->next));
  if (!pop) {
    return NULL;
  }
  if (!pop->next) {
    atomic_compare_exchange_strong(&q->head, &old_head, NULL);
  }
  void* data = pop->data;
  free(pop);
  return data;
}

bool queue_is_empty(queue_t q) {
  return atomic_load(&q->tail) == NULL && atomic_load(&q->head) == NULL;
}

unsigned queue_get_length(queue_t q) {
  unsigned i = 0;
  q_cell_t cell = atomic_load(&q->tail);
  while (cell) {
    i += 1;
    cell = cell->next;
  }
  return i;
}
