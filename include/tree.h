#ifndef TREE_H
#define TREE_H

#include <stdint.h>
#include <stdlib.h>


typedef struct Tree Tree;
typedef struct TreeIter TreeIter;

Tree* tree_create(int (*compare)(const void*, const void*));
char tree_set(Tree* tree, const void* key, const size_t key_size, int (*set_val)(void**, size_t*));
uint32_t tree_size(Tree* tree);
void tree_print(Tree* tree, void (*print)(const void*, const void*, const size_t, const size_t));
void tree_free(Tree* tree);
TreeIter* tree_iter_create(Tree* tree);
void tree_free(Tree* tree);
char tree_iter_has_next(TreeIter* tree_iter);
char tree_iter_next(TreeIter* tree_iter, void** key, size_t* key_size, void** val, size_t* val_size);
void tree_iter_free(TreeIter* tree_iter);

#endif