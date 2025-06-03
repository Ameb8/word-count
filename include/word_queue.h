#ifndef WORD_QUEUE_H
#define WORD_QUEUE_H

typedef struct WordQueue WordQueue;

 
WordQueue* word_queue_create(int capacity);
char word_queue_insert(WordQueue* wq, char* word, unsigned long long count, int index);
char* word_queue_get_min(WordQueue* pq, unsigned long long* count, int* index);
char* word_queue_peak(WordQueue* pq);
int word_queue_size(WordQueue* pq);
char word_queue_is_empty(WordQueue* pq);
void word_queue_free(WordQueue* pq);

#ifdef DBG
typedef struct Element Element;
void print_heap(WordQueue* pq);
#endif

#endif