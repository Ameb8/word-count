#ifndef MIN_QUEUE_H
#define MIN_QUEUE_H

#include <stdio.h>

typedef struct MinQueue MinQueue;

MinQueue* min_queue_create(int capacity, int (*compare)(const void*, const void*));
char min_queue_insert(MinQueue* pq, void* key, size_t key_size, void* val, size_t val_size);
char min_queue_get_min(MinQueue* pq, void** key, void** val);
char min_queue_peak(MinQueue* pq, void** key, void** val);
int min_queue_size(MinQueue* pq);
char min_queue_is_empty(MinQueue* pq);
void min_queue_free(MinQueue* pq);

#endif