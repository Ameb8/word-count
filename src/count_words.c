#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include "../include/tree.h"

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
    char read_first
} ThreadArgs;




// Merge trees and serialize into single binary file
char write_file(Tree** dicts, long num_cores) {
    // Open file and append
    FILE* file = fopen(OUTPUT_FILE, "ab");

    if(!file) // File opening failed
        return 0;

    // Allocate array to hold tree iterators
    TreeIter** dict_iters = malloc(num_cores * sizeof(TreeIter*));
    
    // Initialize tree iterators
    for(int i = 0; i < num_cores; i++)
        dict_iters[i] = tree_iter_create(dicts[i]);

    while(1) { // Write all words and count to file
        // Initialize first word and its count
        char* first_word = NULL;
        int first_count = -1;

        for(int i = 0; i < num_cores; i++) {
            if(tree_iter_has_next(dict_iters[i])) {
                // Get word and count
                char* word;
                unsigned long long count;
                get_word(dict_iters[i], &word, &count);

                // Initialize first word if empty
                if(!first_word) {
                    first_word = word;
                    first_count = count;
                    continue;
                }

                // Compare word to current first word
                int cmp = strcmp(first_word, word);

                if(cmp == 0) // If same, sum counts
                    first_count += count;

                // Update first_word if first alphabetically
                if(cmp > 0) {
                    first_word = word;
                    first_count = count;
                }
            }
        }

        if(first_word && first_count > 0) {
            //  Write to file

        } else { // No words left
            break;
        } 

    }
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

// Read subsection of file and return Tree containing word count
void* thread_read(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;

    FILE* file = fopen(args->filepath, "r");
    
    // Move position indicator to start of thread's subsetion
    if(fseek(file, args->start_offset, SEEK_SET))
        return NULL; // fseek failed

    char word[WORD_BUF_SIZE]; // Buffer for word
    long pos = args->start_offset;  // Get starting position
    char c; // hold chars

    // Advance file pointer to first delimiter
    while((c = fgetc(file)) != EOF) {
        if(args->read_first)
            continue; // First thread must read first word

        if(isspace(c))
            break; // Delimiter  found

        long pos = ftell(file); // Get file pointer position

        if(pos == -1) // Error getting file position
            return NULL;
            
        if(pos > args->end_offset)
            break; // No full words in section
    }

    // Create tree to hold words
    Tree* dict = tree_create(compare_str);

    // Add words until end of file or section
    while(pos < args->end_offset) {
        // Get file pointer position
        long word_start = ftell(file); 

        if(word_start == -1)
            return NULL; // Error detected

        // Read next word
        int ret = fscanf(file, "%255s", word);

        // End of file reached
        if(ret == EOF)
            break;

        // Record word in tree
        tree_set(dict, word, size_of(word), set_word_count);
    }

    return dict;
}


char count_words(char* filepath) {
    // Get number of logical cores available 
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);

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

    // Merge and write results to file
    return serialize_dict(dicts, num_cores);
}
