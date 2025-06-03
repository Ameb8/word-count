/*
#include <stdlib.h>
#include <string.h>
#include "../include/tree.h"
#include "../include/min_queue.h"


typedef struct {
    void* key;
    void* val;
    size_t key_size;
    size_t val_size;
} Element;

typedef struct MinQueue {
    Element** heap;
    int (*compare)(const void*, const void*);
    int size;
    int capacity;
} MinQueue;

MinQueue* min_queue_create(int capacity, int (*compare)(const void*, const void*)) {
    // Allocate memory for priority queue
    MinQueue* pq = malloc(sizeof(MinQueue));

    // Allocate and initialize heap
    pq->heap = calloc(capacity, sizeof(Element*));

    // Initialize fields
    pq->size = 0;
    pq->capacity = capacity;
    pq->compare = compare;

    return pq;
}


static inline int min_index(MinQueue* pq, int i, int j) {
    // Check if out of bound
    if(i >= pq->size)
        return j;
    if(j >= pq->size)
        return i;
    
    // Check which index has larger key
    if(pq->compare(pq->heap[i], pq->heap[j]) < 0)
        return i;

    return j;
}

static inline int parent(int index) {
    return (index - 1) / 2;
}

static inline int left_child(MinQueue* pq, int index) {
    return 2 * index + 1;
}

static inline int right_child(MinQueue* pq, int index) {
    return 2 * index + 2;
}


// Gets index of a min value between parent and children
static inline int min_child(MinQueue* pq, int parent) {
    if(left_child(pq, parent < pq->size) && min_index(pq, left_child(pq, parent), right_child(pq, parent)) < 0) {
        // Compare left child to parent
        if(min_index(pq, left_child(pq, parent), parent) < 0)
            return left_child(pq, parent);
        
        return parent; // Parent is min
    } else if(right_child(pq, parent < pq->size)) {
        // Compare left child to parent
        if(min_index(pq, right_child(pq, parent), parent) < 0)
            return right_child(pq, parent);
        
        return parent; // Parent is min
    }
}

static inline void swap(MinQueue* pq, int index_1, int index_2) {
    Element* temp = pq->heap[index_1];
    pq->heap[index_1] = pq->heap[index_2];
    pq->heap[index_2] = temp;
}

static void heapify_up(MinQueue* pq) {
    int pos = pq->size - 1;

    while(pos > 0 && pq->compare(pq->heap[pos], pq->heap[parent(pos)]) < 0) {
        swap(pq, pos, parent(pos));
        pos = parent(pos);
    }
}


Element* element_create(const void* key, size_t key_size, const void* val, size_t val_size) {
    // Allocate memory for element
    Element* elem = malloc(sizeof(Element));

    if(!elem) // Allocation failed
        return NULL;

    #ifdef DBG
    typedef struct {
        unsigned long long count;
        int index;
    } HeapVal;

    printf("Val Size: %zu\n", val_size);
    HeapVal* test = (HeapVal*)val;
    printf("HeapVal: %llu, %d\n", test->count, test->index);
    printf("Key Size: %zu\n", key_size);
    char* key_print = (char*)key;
    printf("Key: %s\n", key_print);
    #endif

    // Allocate memory for key
    elem->key = malloc(key_size);

    if (!elem->key) { // Allocation failed
        free(elem);
        return NULL;
    }

    // Allocate memory for val
    elem->val = malloc(val_size);

    if (!elem->val) { // Alocation failed
        free(elem->key);
        free(elem);
        return NULL;
    }

    // Copy data
    memcpy(elem->key, key, key_size);
    memcpy(elem->val, val, val_size);

    // Set sizes
    elem->key_size = key_size;
    elem->val_size = val_size;

    return elem;
}



static void heapify_down(MinQueue* pq) {
    int pos = 0;
    int min_index = min_child(pq, pos);

    while(min_index != pos) { // Heap down
        swap(pq, pos, min_index);
        pos = min_index;
        min_index = min_child(pq, pos);
    }
}


char min_queue_insert(MinQueue* pq, void* key, size_t key_size, void* val, size_t val_size) {
    if(!pq ||pq->size == pq->capacity)
        return 0; // Check if new value can fit and non-null input

    Element* new = element_create(key, key_size, val, val_size);

    if(!new) {//Element creation failed
        printf("Element not created\n");
        return 0;
    }
    pq->heap[pq->size++] = new;

    heapify_up(pq);

    return 1;
}


char min_queue_get_min(MinQueue* pq, void** key, void** val) {
    if(!pq || pq->size == 0)
        return 0;

    Element* min = pq->heap[0];

    // Remove element from heap
    swap(pq, 0, pq->size--);
    heapify_down(pq);

    // Transfer ownership
    *key = min->key;
    *val = min->val;

    free(min);

    return 1;
}


char min_queue_peak(MinQueue* pq, void** key, void** val) {
    if(pq->size == 0)
        return 0;
    
    *key = pq->heap[0]->key;
    *val = pq->heap[0]->val;

    return 1;
}


int min_queue_size(MinQueue* pq) {
    if(!pq) // return -1 if null input
        return -1;

    return pq->size;
}


char min_queue_is_empty(MinQueue* pq) {
    if(!pq)
        return 1;

    if(pq->size == 0)
        return 1;

    return 0;
}


void element_free(Element* element) {
    free(element->key);
    free(element->val);
    free(element);
}
void min_queue_free(MinQueue* pq) {
    if(!pq) // Ensure input is non-null
        return;

    // Free remaining elements
    for(int i = 0; i < pq->size; i++)
        element_free(pq->heap[i]);

    // Free heap and queue
    free(pq->heap);
    free(pq);
}*/