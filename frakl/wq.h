// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include <stdbool.h>
#include <stdlib.h>

struct wq;
typedef struct wq* wq_t;
typedef void (*wq_cb_t)(void** work, unsigned n, void* ctx);

// Pass worker_count = 0 for default
wq_t wq_create(const char* name, wq_cb_t cb, size_t worker_count,
               size_t queue_cap_shift);

void wq_set_worker_cache_size(wq_t wq, uint32_t size);

const char* wq_get_name(wq_t wq);

void wq_push(wq_t wq, void* work);

void wq_start(wq_t wq, void* ctx);

void wq_wait(wq_t wq);

void wq_destroy(wq_t wq);

bool wq_is_running(wq_t wq);
