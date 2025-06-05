#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "../include/word_queue.h"
#include "../include/tree.h"
#include "../include/save_dict.h"

#define FILE_OUT "data.bin"

void print_word(const void* key, const void* val, const size_t key_size, const size_t val_size) {
    const char* word = (const char*)key;
    unsigned long long count = *(const unsigned long long*)val;

    printf("%s: %llu\n", word, count);
}


int compare_str(const void* a, const void* b) {
    return strcmp((const char*)a, (const char*)b);
}


char word_write(FILE* file, char* word, unsigned long long count) {
    if(!file || !word)
        return 0; 
    // Write word length to file
    size_t len = strlen(word);
    size_t len_write = fwrite(&len, sizeof(len), 1, file);

    // Write word and count to file
    size_t word_write = fwrite(word, sizeof(char), len, file);
    size_t count_write = fwrite(&count, sizeof(count), 1, file);

    // Ensure correct number of bytes were written
    if(len_write != 1) // Check word size
        return 0; //
    if(word_write != strlen(word)) // Check word
        return 0;
    if(count_write != 1) //Check count
        return 0;

    return 1;
}


char* tree_iter_get(TreeIter* iter, unsigned long long* count) {
    void* word_ptr;
    void* count_ptr;

    // Attempt to get the next item
    if (!tree_iter_next(iter, &word_ptr, NULL, &count_ptr, NULL)) {
        return NULL;  // No more items
    }

    if(count != NULL) // Assign count
        *count = *(unsigned long long*)count_ptr;

    return (char*)word_ptr;
}


char replace_data(FILE* file, unsigned long long data, long offset) {
    // Find start of data
    if(fseek(file, offset, SEEK_SET))
        return 0; // Error in fseek

    // Write data
    if(fwrite(&data, sizeof(data), 1, file) != 1)
        return 0; // Write failed

    return 1;
}


char write_word_count(FILE* file, unsigned long long unique_words, unsigned long long num_words, long offset) {
    // Find start of data
    if(fseek(file, offset, SEEK_SET))
        return 0; // Error in fseek

    // Replace unique word count
    if(fwrite(&unique_words, sizeof(unique_words), 1, file) != 1)
        return 0; // Write failed

    // Replace total word count
    if(fwrite(&num_words, sizeof(num_words), 1, file) != 1)
        return 0; // Write failed

    return 1;
}

Tree** free_dicts(Tree** dicts, int num_dicts) {
    for(int i = 0; i < num_dicts; i++)
        tree_free(dicts[i]); // Free trees

    free(dicts); // Free tree array

    return NULL;
}

char free_data(TreeIter** iters, WordQueue* queue, FILE* file, char* word, int num_trees) {
    // Free all trees and iterators
    for(int i = 0; i < num_trees; i++) {
        if(iters && iters[i]) // Free iterator
            tree_iter_free(iters[i]);
    }

    if(iters) // Free iterator array
        free(iters);

    if(queue) // Free word priority queue
        word_queue_free(queue);

    if(file) // Close file
        fclose(file);

    if(word) // Free word
        free(word);

    return 0;
}


FILE* init_out_file(const char* input_name, long* offset) {
    FILE* file = fopen(FILE_OUT, "wb"); // Open file to write

    if(!file) // File failed to open
        return NULL;

    size_t len = strlen(input_name); // Get length of name
    
    // Write length of input file name
    if(fwrite(&len, sizeof(len), 1, file) != 1) {
        // Write failed
        fclose(file);
        return NULL;
    }

    // Write filename
    if(fwrite(input_name, sizeof(char), strlen(input_name), file) != strlen(input_name)) {
        // Write failed
        fclose(file);
        return NULL;
    }

    unsigned long long dummy_data = 0; // Dummy data for word count
    *offset = ftell(file); // Assign start of unique words

    if(*offset == -1l) { // ftell failure
        fclose(file);
        return NULL;
    }

    // Write number of unique words
    if(fwrite(&dummy_data, sizeof(dummy_data), 1, file) != 1) {
        // Write failed
        fclose(file);
        return NULL;
    }

    // Write total number of words
    if(fwrite(&dummy_data, sizeof(dummy_data), 1, file) != 1) {
        // Write failed
        fclose(file);
        return NULL;
    }

    return file;
}


TreeIter** get_iters(Tree** trees, int num_cores) {
    // Create array too hold tree iterators
    TreeIter** iters = malloc(num_cores * sizeof(TreeIter*));

    if(!iters) // Allocation failure
        return NULL;

    for(int i = 0; i < num_cores; i++) { // Iterate tree dicts
        if(!trees[i]) // Only write if all tree's non-null
            return NULL;
    
        iters[i] = tree_iter_create(trees[i]); // Create tree iterator
    
        if(!iters[i]) // Allocation failed
            return NULL;
    }

    return iters;
}


WordQueue* init_word_queue(TreeIter** iters, int num_cores) {
    // Create priority queue to hold words
    WordQueue* word_queue = word_queue_create(num_cores);

    if(!word_queue) // Allocation failed
        return NULL;
    
    // Populate queue with word from each tree
    for(int i = 0; i < num_cores; i++) {
        if(tree_iter_has_next(iters[i])) {
            // Get word and count
            unsigned long long count;
            char* word = tree_iter_get(iters[i], &count);
            word_queue_insert(word_queue, word, count, i);
        }
    }

    return word_queue;
}


char write_dict(Tree** dicts, int num_cores, const char* input_name) {
    // Number of words counter
    unsigned long long num_words = 0;
    unsigned long long unique_words = 0;

    // Offset of word position
    long count_offset = 0;

    // Get file with header containing dummy data
    FILE* file = init_out_file(input_name, &count_offset);

    if(!file) // File failed to open
        return 0;

    // Create array too hold tree iterators
    TreeIter** next = get_iters(dicts, num_cores);

    if(!next) // Tree Iterator not created
        return free_data(NULL, NULL, file, NULL, num_cores);

    WordQueue* word_queue = init_word_queue(next, num_cores);    

    if(!word_queue) // Allocation failed
        return free_data(NULL, NULL, file, NULL, num_cores);

    // Write words and counts to file
    while(!word_queue_is_empty(word_queue)) {
        // Get next word to write
        unsigned long long count;
        int index;
        char* word = word_queue_get_min(word_queue, &count, &index);

        // Add word from tree of next word to queue
        if(tree_iter_has_next(next[index])) {
            unsigned long long new_count;           
            char* new_word = tree_iter_get(next[index], &new_count);
            word_queue_insert(word_queue, new_word, new_count, index);
        }
        
        // Add duplicate words from queue
        while(!word_queue_is_empty(word_queue) && !strcmp(word, word_queue_peak(word_queue))) {
            // Get duplicate word count
            unsigned long long dup_count;
            free(word_queue_get_min(word_queue, &dup_count, &index));

            //append duplicate word's count
            count += dup_count;

            // Add word from tree duplicate words tree to queue
            if(tree_iter_has_next(next[index])) {
                char* new_word = tree_iter_get(next[index], &dup_count);
                word_queue_insert(word_queue, new_word, dup_count, index);
            }
        }
        

        // Write word to file
        if(!word_write(file, word, count))
            return free_data(next, word_queue, file, word, num_cores);

        free(word); // Deallocate word;

        // Increment word counts
        num_words += count;
        unique_words++;
    }

    // Set total and unique words
    if(!write_word_count(file, unique_words, num_words, count_offset))
        return free_data(next, word_queue, file, NULL, num_cores);

    free_data(next, word_queue, file, NULL, num_cores);

    return 1;
}