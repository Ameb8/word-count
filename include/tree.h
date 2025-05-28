#ifndef TREE_H
#define TREE_H

#include <stdlib.h>

typedef struct Tree Tree;

Tree* tree_create(int (*compare)(const void*, const void*));
char tree_set(Tree* tree, const void* key, const size_t key_size, int (*set_val)(void**, size_t*));
void tree_print(Tree* tree, void (*print)(const void*, const void*, const size_t, const size_t));
//unsigned long long tree_size(Tree* tree);
//unsigned long long tree_unique(Tree* tree);
//void tree_write(const Tree* tree, const char* filename);

#endif