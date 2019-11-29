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
  uint32_t local_cache_size;
  bool computed_cache_size;
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
  res->local_cache_size = (uint32_t)-1;
  return res;
}

void wq_set_worker_cache_size(wq_t wq, uint32_t size) {
  wq->local_cache_size = size;
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

  const uint32_t cache_size = wq->local_cache_size ?: 10;
  struct wq_item** cache = calloc(cache_size, sizeof(struct wq_item*));

  struct wq_item* item;
  struct wq_item* const* end = cache + cache_size;
  struct wq_item** iter;
  do {
    unsigned n = queue_pop_n(q, cache_size, (void**)cache);
    if (!n) {
      break;
    }
    iter = cache;
    end = iter + n;
    do {
      item = *iter;
      item->cb(item->work, ctx);
    } while (++iter != end);
  } while (true);
  free(cache);
  return NULL;
}

void wq_start(wq_t wq, void* ctx) {
  if (wq->workers) {
    return;
  }
  const size_t worker_count = wq->worker_count;
  wq->workers = calloc(worker_count, sizeof(pthread_t));
  wq->ctx = ctx;
  if (wq->local_cache_size == (uint32_t)-1) {
    wq->local_cache_size = queue_get_length(wq->queue) / (8 * worker_count);
    wq->computed_cache_size = true;
  } else {
    wq->computed_cache_size = false;
  }
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
  if (wq->computed_cache_size) {
    wq->computed_cache_size = false;
    wq->local_cache_size = (uint32_t)-1;
  }
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
