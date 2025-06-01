#ifdef TEST

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../include/tree.h"

void print_word(const void* key, const void* val, const size_t key_size, const size_t val_size) {
    const char* word = (const char*)key;
    unsigned long long count = *(const unsigned long long*)val;

    printf("%s: %llu\n", word, count);
}


int compare_str(const void* a, const void* b) {
    return strcmp((const char*)a, (const char*)b);
}


// Pass to tree set to increment val (word count) or set to 1 if null
int set_word_count(void** val, size_t* val_size) {
    if(*val == NULL) { // New word added to tree
        // Allocate memory for word count
        unsigned long long* count = malloc(sizeof(unsigned long long));
        
        if(!count) // Allocation failed
            return 0;

        *count = 1;
        *val = count;
        *val_size = sizeof(unsigned long long);
    } else { // Word already exists, increment count
        unsigned long long* count = (unsigned long long*)(*val);
        (*count)++;
    }
    return 1;
}



void test_tree() {
    Tree* tree = tree_create(compare_str);

    if (!tree) {
        fprintf(stderr, "Failed to create tree.\n");
        return;
    }

    const char* keys[] = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j", "k", "l", "m", "n", "o", "p"};
    size_t num_keys = sizeof(keys) / sizeof(keys[0]);

    for (size_t i = 0; i < num_keys; ++i) {
        tree_set(tree, keys[i], strlen(keys[i]) + 1, set_word_count);
    }

    printf("Tree size: %u\n", tree_size(tree));
    tree_print_level(tree, print_word);

    TreeIter* tree_it = tree_iter_create(tree);
    printf("\nIterator:\n");

    while(tree_iter_has_next(tree_it)) {
        void* key_ptr;
        void* val_ptr;
        size_t key_size, val_size;
    
        if(tree_iter_next(tree_it, &key_ptr, &key_size, &val_ptr, &val_size)) {
            char* key = (char*)key_ptr;
            unsigned long long value = *(unsigned long long*)val_ptr;
    
            printf("Key: %s, Value: %llu\n", key, value);
        }
    }
    
    tree_iter_free(tree_it);

    tree_free(tree);
}

void test() {
    test_tree();
}

#endif