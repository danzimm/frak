// Copywrite (c) 2019 Dan Zimmerman

#pragma once

#include <stdbool.h>
#include <stdlib.h>

struct queue;
typedef struct queue* queue_t;

queue_t queue_create(void);

void queue_destroy(queue_t q);

void queue_push(queue_t q, void* data);

void* queue_pop(queue_t q);

bool queue_is_empty(queue_t q);

unsigned queue_get_length(queue_t q);
