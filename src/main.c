#include <stdio.h>
#include <string.h>
#include "../include/tree.h"


void test_print(const void* key, const void* val, const size_t key_size, const size_t val_size) {
    printf("%llu occurrences of '%s'\n", *(unsigned long long*)val, (const char*)key);
}

int compare_str(const void* a, const void* b) {
    return strcmp((const char*)a, (const char*)b);
}


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

void test_file() {
    
}

int main() {
    Tree* tree = tree_create(compare_str);

    const char* strings[] = {
        "banana",
        "apple",
        "cherry",
        "banana",
        "date",
        "apple",
        "cat",
        "dog"
    };

    int n = sizeof(strings) / sizeof(strings[0]);
    for (int i = 0; i < n; ++i) {
        tree_set(tree, strings[i], sizeof(strings[i]), set_word_count);
    }

    printf("Tree (in-order):\n");
    tree_print(tree, test_print);

    return 0;
}