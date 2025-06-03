#include <stdio.h>
#include <stdlib.h>
#include <string.h>


typedef struct Element {
    char* word;
    unsigned long long count;
    int index;
} Element;

typedef struct WordQueue {
    Element** heap;
    int size;
    int capacity;
} WordQueue;

 
WordQueue* word_queue_create(int capacity) {
    // Allocate memory for priority queue
    WordQueue* wq = malloc(sizeof(WordQueue));

    // Allocate and initialize heap
    wq->heap = calloc(capacity, sizeof(Element*));

    // Initialize fields
    wq->size = 0;
    wq->capacity = capacity;

    return wq;
}


static inline int min_index(WordQueue* wq, int i, int j) {
    // Check if out of bound
    if(i >= wq->size)
        return j;
    if(j >= wq->size)
        return i;
    
    // Check which index has larger key
    if(strcmp(wq->heap[i]->word, wq->heap[j]->word) < 0)
        return i;

    return j;
}

static inline int parent(int index) {
    return (index - 1) / 2;
}

static inline int left_child(int index) {
    return 2 * index + 1;
}

static inline int right_child(int index) {
    return 2 * index + 2;
}

/*
// Gets index of a min value between parent and children
static inline int min_child(WordQueue* wq, int parent) {
    if(left_child(parent) < wq->size && min_index(wq, left_child(parent), right_child(parent)) < 0) {
        // Compare left child to parent
        if(min_index(wq, left_child(parent), parent) < 0)
            return left_child(parent);
        
        return parent; // Parent is min
    } else if(right_child(parent) < wq->size) {
        // Compare left child to parent
        if(min_index(wq, right_child(parent), parent) < 0)
            return right_child(parent);
        
        return parent; // Parent is min
    }
}*/



static inline int min_child(WordQueue* wq, int parent) {
    int left = left_child(parent);
    int right = right_child(parent);
    int smallest = parent;

    if (left < wq->size &&
        strcmp(wq->heap[left]->word, wq->heap[smallest]->word) < 0) {
        smallest = left;
    }

    if (right < wq->size &&
        strcmp(wq->heap[right]->word, wq->heap[smallest]->word) < 0) {
        smallest = right;
    }

    return smallest;
}


static inline void swap(WordQueue* wq, int index_1, int index_2) {
    Element* temp = wq->heap[index_1];
    wq->heap[index_1] = wq->heap[index_2];
    wq->heap[index_2] = temp;
}

static void heapify_up(WordQueue* wq) {
    int pos = wq->size - 1;

    while(pos > 0 && strcmp(wq->heap[pos]->word, wq->heap[parent(pos)]->word) < 0) {
        swap(wq, pos, parent(pos));
        pos = parent(pos);
    }
}

Element* element_create(char* word, unsigned long long count, int index) {
    // Allocate memory for element
    Element* element = malloc(sizeof(Element));

    if(!element) // Allocation failed
        return NULL;

    // Allocate memory for word
    int len = strlen(word) + 1;
    element->word = malloc(len);

    if(!element->word) // Allocation failed
        return NULL;
    
    // Copy word to element
    memcpy(element->word, word, len);

    // Set the other fields
    element->count = count;
    element->index = index;

    return element;
}


static void heapify_down(WordQueue* wq) {
    int pos = 0;
    int min_index = min_child(wq, pos);

    while(min_index != pos) { // Heapify down
        swap(wq, pos, min_index);
        pos = min_index;
        min_index = min_child(wq, pos);
    }
}


char word_queue_insert(WordQueue* wq, char* word, unsigned long long count, int index) {
    if(!wq || !word || wq->size == wq->capacity)
        return 0; // Check if new value can fit and non-null input

    // Create new element
    Element* new = element_create(word, count ,index);

    if(!new) //Element creation failed
        return 0;

    // Add element to heap
    wq->heap[wq->size++] = new;
    heapify_up(wq);

    return 1;
}


char* word_queue_get_min(WordQueue* pq, unsigned long long* count, int* index) {
    if(!pq || pq->size == 0)
        return NULL;

    Element* min = pq->heap[0];

    // Remove element from heap
    swap(pq, 0, --pq->size);
    heapify_down(pq);

    // Assign fields
    *count = min->count;
    *index = min->index;
    char* word = min->word;

    free(min);

    return word;
}


#ifdef DBG
void print_element(Element* element) {
    printf("(%s)(%llu) [%d]\n", element->word, element->count, element->index);
}

void print_heap(WordQueue* pq) {
    for(int i = 0; i < pq->size; i++) {
        printf("\nHeap Index %d\n", i);
        if(!pq->heap[i])
            printf("Element is NULL\n");
        else
            print_element(pq->heap[i]);
    }
}
#endif


char* word_queue_peak(WordQueue* pq) {
    if(!pq || !pq->heap || pq->size == 0)
        return NULL;

    #ifdef DBG
    printf("Word Queue Size: %d\n", pq->size);
    print_heap(pq);
    #endif

    return pq->heap[0]->word;
}


int word_queue_size(WordQueue* pq) {
    if(!pq) // return -1 if null input
        return -1;

    return pq->size;
}


char word_queue_is_empty(WordQueue* pq) {
    if(!pq)
        return 1;

    if(pq->size == 0)
        return 1;

    return 0;
}


void element_free(Element* element) {
    free(element->word);
    free(element);
}


void word_queue_free(WordQueue* pq) {
    if(!pq) // Ensure input is non-null
        return;

    // Free remaining elements
    for(int i = 0; i < pq->size; i++)
        element_free(pq->heap[i]);

    // Free heap and queue
    free(pq->heap);
    free(pq);
}