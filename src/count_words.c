#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include "../include/tree.h"
#include "../include/min_queue.h"
#include "../include/count_words.h"

#define OUTPUT_FILE "data.bin"

#define WORD_BUF_SIZE 256


// Holds parameters passed to each reading thread
typedef struct {
    // Path to file being read
    const char* filepath;

    // Start and end location for thread reading
    long start_offset;
    long end_offset;

    // Determine if should read first word
    char read_first;
} ThreadArgs;



typedef struct {
    unsigned long long count;
    int index;
} HeapVal;


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


// Write word and count to file
void write_word(FILE* file, char* word, unsigned long long count) {
    unsigned int len = strlen(word);
    fwrite(&len, sizeof(unsigned int), 1, file);
    fwrite(word, sizeof(char), len, file);
    fwrite(&count, sizeof(unsigned long long), 1, file);
}


// Merge trees and serialize into single binary file
char write_file(Tree** dicts, long num_cores) {
    // Open file and append
    FILE* file = fopen(OUTPUT_FILE, "wb");

    if(!file) // File opening failed
        return 0;

    #ifdef DBG
    printf("data.bin Opened\n");
    #endif

    uint32_t num_words = 0;

    // Allocate array to hold tree iterators
    TreeIter** dict_iters = malloc(num_cores * sizeof(TreeIter*));
    
    // Initialize tree iterators
    for(int i = 0; i < num_cores; i++) {
        if(dicts[i]) // Check tree is non-null
            dict_iters[i] = tree_iter_create(dicts[i]);
    }

    // Initialize priority queue for merging
    MinQueue* next_words = min_queue_create(num_cores + 1, compare_str);

    // Populate queue with min node in each tree
    for(int i = 0; i < num_cores; i++) {
        void* word;
        void* count;

        // Add element word and count to queue
        if(dict_iters[i] && tree_iter_has_next(dict_iters[i])) {
            // Get word and count
            tree_iter_next(dict_iters[i], &word, NULL, &count, NULL);
            
            // Add to queue
            HeapVal val = {*(unsigned long long*)count, i};
            #ifdef DBG
            printf("HeapVal[%d] = {%lld, %d}\n", i, val.count, val.index);
            #endif
            min_queue_insert(next_words, (char*)word, strlen(word) + 1, &val, sizeof(HeapVal));
        }
    }

    #ifdef DBG
    printf("Queue Size: %d\n", min_queue_size(next_words));

    if(!min_queue_is_empty(next_words))
        printf("Queue not empty\n");
    else   
        printf("Queue empty\n");

    int lr = 0;
    #endif

    while(!min_queue_is_empty(next_words)) {
        #ifdef DBG
        printf("Min Queue Not empty, Loop: %d\n", lr++);
        #endif

        void* void_word;
        void* void_val;

        min_queue_get_min(next_words, &void_word, &void_val);

        char* word = (char*)void_word;
        HeapVal* val = (HeapVal*)void_val;

        #ifdef DBG
        printf("Adding to output: {%s, {%lld, %d}}\n", word, val->count, val->index);
        #endif

        // Add next node from word's list to queue
        if(tree_iter_has_next(dict_iters[val->index])) {
            tree_iter_next(dict_iters[val->index], &void_word, NULL, &void_val, NULL);
            HeapVal* hval = malloc(sizeof(HeapVal));
            hval->count = *(unsigned long long*)void_val;
            hval->index = val->index;
            //HeapVal hval = {*(unsigned long long*)void_val, val->index};
            #ifdef DBG
            printf("Creating HeapVal: count=%llu, index=%d\n", hval->count, hval->index);
            printf("HeapVal address: %p, size: %zu\n", (void*)&val, sizeof(HeapVal));
            printf("sizeof(HeapVal): %zu\n", sizeof(*hval));
            #endif
            min_queue_insert(next_words, (char*)void_word, strlen(void_word) + 1, hval, sizeof(HeapVal));
        }

        // Check next word
        min_queue_peak(next_words, &void_word, &void_val);

        // Sum all duplicates
        while(!strcmp((char*)void_word, word)) {
            // Increment word count
            HeapVal* duplicate = (HeapVal*)void_val;
            val->count += duplicate->count;

            // Add next node from duplicates list to queue
            if(tree_iter_has_next(dict_iters[duplicate->index])) {
                tree_iter_next(dict_iters[duplicate->index], &void_word, NULL, &void_val, NULL);
                HeapVal hval = {*(unsigned long long*)void_val, duplicate->index};
                min_queue_insert(next_words, (char*)void_word, strlen(void_word) + 1, &hval, sizeof(HeapVal));
            }

            // Discard top element from queue
            min_queue_get_min(next_words, NULL, NULL);
        }

        // Write element to file
        write_word(file, word, val->count);
    }

    min_queue_free(next_words);

    // Free trees and iterators
    for(int i = 0; i < num_cores; i++) {
        tree_free(dicts[i]);
        tree_iter_free(dict_iters[i]);
    }

    return 1;
}



// Read subsection of file and return Tree containing word count
void* thread_read(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;

    FILE* file = fopen(args->filepath, "r");

    if(!file)
        return NULL;
    
    // Move position indicator to start of thread's subsection
    if(fseek(file, args->start_offset, SEEK_SET))
        return NULL; // fseek failed


    #ifdef DBG
    printf("Thread scanning section from offset %ld to %ld:\n", args->start_offset, args->end_offset);
    long cur_pos = ftell(file);
    if (cur_pos == -1) return NULL;

    fseek(file, args->start_offset, SEEK_SET);

    char* buffer = malloc(args->end_offset - args->start_offset + 1);
    if (!buffer) return NULL;

    fread(buffer, 1, args->end_offset - args->start_offset, file);
    buffer[args->end_offset - args->start_offset] = '\0';  // Null-terminate

    printf("Thread section contents:\n%s\n", buffer);
    free(buffer);

    // Return to where we were before printing section
    fseek(file, cur_pos, SEEK_SET);

    // Move position indicator to start of thread's subsection
    if(fseek(file, args->start_offset, SEEK_SET))
    return NULL; // fseek failed
    #endif
    

    char word[WORD_BUF_SIZE]; // Buffer for word
    long pos = args->start_offset;  // Get starting position
    char c; // hold chars

    // Advance file pointer to first delimiter
    while((c = fgetc(file)) != EOF) {
        if(args->read_first)
            continue; // First thread must read first word

        if(isspace(c))
            break; // Delimiter found

        long pos = ftell(file); // Get file pointer position

        if(pos == -1) // Error getting file position
            return NULL;
            
        if(pos > args->end_offset)
            break; // No full words in section
    }

    // Create tree to hold words
    Tree* dict = tree_create(compare_str);

    #ifdef DBG
    char* words_read[1000] = {NULL};
    int read_pos = 0;
    #endif

    // Add words until end of file or section
    while(1) {
        // Get file pointer position
        long word_start = ftell(file); 

        if(word_start >= args->end_offset)
            break; // End of section reached

        if(word_start == -1)
            return NULL; // Error detected

        // Read next word
        int ret = fscanf(file, "%255s", word);

        #ifdef DBG
        // Add read word to words_read
        words_read[read_pos++] = strdup(word);
        #endif

        // End of file reached
        if(ret == EOF)
            break;

        // Record word in tree
        tree_set(dict, word, sizeof(word), set_word_count);
    }

    #ifdef DBG
    printf("Words read (%d):\n", read_pos);

    for(int i = 0; i < 100 && words_read[i]; i++) {
        printf("%s\n", words_read[i]);
        free(words_read[i]);
    }
    #endif

    // Free thread argument memory
    free(arg);

    return dict;
}


char count_words(char* filepath) {
    // Get number of logical cores available 
    long num_cores = 2; //= sysconf(_SC_NPROCESSORS_ONLN);

    #ifdef DBG
    printf("Cores Available: %ld\n", num_cores);
    #endif

    // System error
    if(num_cores < 1) {
        // Terminate program
        perror("sysconf");
        exit(1);
    }

    // Get total filesize
    struct stat st;
    stat(filepath, &st);

    // Get size of each subsection
    long subsect_size = st.st_size / num_cores;

    // Create array of each thread's ID
    pthread_t* thread_ids = malloc(num_cores * sizeof(pthread_t));

    // Create array to hold thread's return value
    void** thread_results = malloc(num_cores * sizeof(Tree*));

    // Create a thread for each core
    for(int i = 0; i < num_cores; i++) {
        // Define subsection offsets
        long start_offset = i * subsect_size;
        long end_offset;

        // Calculate end offset
        if(i == num_cores - 1) // Assign final thread remainder of file
            end_offset = st.st_size;
        else // Assign fixed size chunk
            end_offset = (i + 1) * subsect_size;
        
        // Allocate memory for thread arguments
        ThreadArgs* args = malloc(sizeof(ThreadArgs));

        if(!args) // Allocation failed
            exit(1);

        // Initialize ThreadArgs fields
        args->filepath = filepath;
        args->start_offset = start_offset;
        args->end_offset = end_offset;
        args->read_first = 0;

        if(i == 0) // 1st subsection must read first word
            args->read_first = 1;

        // Create thread
        pthread_create(&thread_ids[i], NULL, thread_read, (void*)args);
    }

    for(int i = 0; i < num_cores; i++) // Synchronize threads
        // Collect return value
        pthread_join(thread_ids[i], &thread_results[i]);
    
    // Convert output to Tree's
    Tree** dicts = (Tree**)thread_results;

    #ifdef DBG
    for(int i = 0; i < num_cores; i++) {
        printf("\n\nThread %d:\n", i);
        tree_print(dicts[i], print_word);
    }
    #endif

    // Merge and write results to file
    return write_file(dicts, num_cores);
}
