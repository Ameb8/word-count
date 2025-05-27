#ifndef SAVE_DICTS_H
#define SAVE_DICTS_H

#include "tree.h"

void dict_save(const Tree* tree, const char* input_filename);
Tree* dict_load(unsigned long long id);
void dict_list();



#endif