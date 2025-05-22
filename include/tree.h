#ifndef TREE_H
#define TREE_H

#include <stdlib.h>

typedef struct Tree Tree;

Tree* tree_create(int (*compare)(const void*, const void*));
void tree_insert(Tree* tree, void* val, size_t size);
void tree_print(Tree* tree, void (*print)(const void*));
unsigned long long tree_size(Tree* tree);
unsigned long long tree_unique(Tree* tree);

#endif