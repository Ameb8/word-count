#include <stdio.h>
#include <string.h>
#include "../include/tree.h"


void test_print(const void* key, const void* val, const size_t key_size, const size_t val_size) {
    printf("%llu occurrences of '%s'\n", (const unsigned long long)val, (const char*)key);
}

int compare_str(const void* a, const void* b) {
    return strcmp((const char*)a, (const char*)b);
}

void set_word(void** val, size_t* val_size) {
    if(!*val)
        *val = 1;
    else
        *val += 1;
    
    val_size = size_of(*val);
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
        tree_set(tree, strings[i], size_of(strings[i]), set_word);
    }

    printf("Tree (in-order):\n");
    tree_print(tree, test_print);

    return 0;
}