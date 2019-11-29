// Copywrite (c) 2019 Dan Zimmerman

#include "wq.h"

#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "queue.h"

struct wq {
  char* name;
  queue_t queue;
  size_t worker_count;
  pthread_t* workers;
  void* ctx;
};

struct wq_item {
  wq_cb_t cb;
  void* work;
};

static size_t get_reasonable_worker_count(void) {
  return 4 * sysconf(_SC_NPROCESSORS_ONLN) / 3;
}

wq_t wq_create(const char* name, size_t worker_count, size_t queue_cap_shift) {
  wq_t res = malloc(sizeof(struct wq));
  res->name = strdup(name);
  res->queue = queue_create(queue_cap_shift);
  if (!worker_count) {
    worker_count = get_reasonable_worker_count();
  }
  res->worker_count = worker_count;
  res->workers = NULL;
  res->ctx = NULL;
  return res;
}

const char* wq_get_name(wq_t wq) { return wq->name; }

void wq_push(wq_t wq, wq_cb_t cb, void* work) {
  struct wq_item* item = malloc(sizeof(struct wq_item));
  item->cb = cb;
  item->work = work;
  queue_push(wq->queue, item);
}

static void* wq_worker(wq_t wq) {
  queue_t q = wq->queue;
  void* ctx = wq->ctx;
  struct wq_item* item;
  while ((item = queue_pop(q)) != NULL) {
    item->cb(item->work, ctx);
    free(item);
  }
  return NULL;
}

void wq_start(wq_t wq, void* ctx) {
  if (wq->workers) {
    return;
  }
  const size_t worker_count = wq->worker_count;
  wq->workers = calloc(worker_count, sizeof(pthread_t));
  wq->ctx = ctx;
  for (size_t i = 0; i < worker_count; i++) {
    pthread_create(&wq->workers[i], NULL, (void*)&wq_worker, wq);
  }
}

void wq_wait(wq_t wq) {
  const size_t worker_count = wq->worker_count;
  for (size_t i = 0; i < worker_count; i++) {
    pthread_join(wq->workers[i], NULL);
  }
  free(wq->workers);
  wq->workers = NULL;
}

void wq_destroy(wq_t wq) {
  free(wq->name);
  queue_destroy(wq->queue);
  if (wq->workers) {
    free(wq->workers);
  }
  free(wq);
}

bool wq_is_running(wq_t wq) { return wq->workers != NULL; }
