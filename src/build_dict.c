#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/tree.h"
#include "../include/word_queue.h"
#include "../include/build_dict.h"
#include "../include/save_dict.h"


// Initial size of word reading buffer
#define INIT_WORD_BUF 64


// Holds parameters passed to each reading thread
typedef struct {
    // Pointers to file start and length
    char* file_start;
    long file_size;

    // Start and end location for thread reading
    long start_offset;
    long end_offset;

    // Determine if should read first word
    char read_first;
} ThreadArgs;


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


void lowercase(char* word) {
    char* c = word;

    while(c && *c != '\0') {
        *c = tolower((unsigned char)*c);
        c++;
    }
}


char grow_word_buf(char word[INIT_WORD_BUF], char** word_ptr, int len, int* cap) {
    *cap *= 2; // Double word buffer capacity

    if(*word_ptr == word) { // Move word data to word_ptr
        *word_ptr = malloc(*cap); // Allocate word pointer

        if(!(*word_ptr)) // Allocation failure
            return 0;
        
        memcpy(*word_ptr, word, len); // Copy data
    } else { // Reallocate word_ptr to increase size
        char* temp = realloc(*word_ptr, *cap); // Double allocated memory

        if(!temp) // Allocation failed
            return 0;

        *word_ptr = temp;
    }

    return 1;
}


void* read_section(void* arg) {
    if(!arg) // Ensure non-null input
        return NULL;

    // Get file pointer
    ThreadArgs* args = (ThreadArgs*)arg;
    char* data = args->file_start;
    long file_size = args->file_size;

    // Get section offsets
    long start = args->start_offset;
    long end = args->end_offset;

    while(start < end && !isspace(data[start]))
        start++; // find first delimiter in section

    // Create tree to hold results
    Tree* dict = tree_create(compare_str);
    
    if(!dict) { // Allocation failed
        free(args);
        return NULL;
    }

    long i = start; // Tracks file position
    
    // Read section of file
    while(i <= end) {
        while(isspace(data[i])) 
            i++; // Find next word

        if(i >= end) 
            break;

        // Data to store current word
        int len = 0; // Holds current length
        char word[INIT_WORD_BUF]; // Holds word

        // Data to allow dynamic word buffer resizing
        char* word_ptr = word; // Points to either word or heap memory
        int cap = INIT_WORD_BUF; // Holds current capacity

        // Save non-punctuation letters until space or file end
        while(i < file_size && !isspace(data[i])) {
            if(isalnum(data[i])) { // Add current char
                if(len + 1 >= cap) { // Grow word buffer
                    if(!grow_word_buf(word, &word_ptr, len, &cap)) {
                        // Program error in grow_word_buf
                        free(args); // Free argument memory

                        if(word_ptr && word_ptr != word)
                            free(word_ptr); // Deallocate word if in heap

                        return NULL;
                    }
                }

                // Save char to word buffer
                word_ptr[len++] = data[i];
            }

            i++; // Increment file position
        }

        if(len > 0) { // Add word to tree
            word_ptr[len] = '\0'; //Add null terminator
            lowercase(word_ptr); // Convert to lowercase
            tree_set(dict, (void*)word_ptr, len + 1, set_word_count); // Add to tree
        }

        // Deallocate word if stored in heap
        if(word_ptr && word_ptr != word)
            free(word_ptr); // Deallocate word pointer

    }

    free(args); // Free memory for argument

    return dict;
}


char* map_file(char* filepath, size_t* filesize) {
    // Open file
    int fd = open(filepath, O_RDONLY);
    
    if(fd < 0)
        return NULL;

    // Get total filesize
    struct stat st;

    if(fstat(fd, &st) < 0) // Filesize retrieve failed
        return 0;

    *filesize = st.st_size; // Assign filesize

    // Map whole file
    char* mapped = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
    
    if(mapped == MAP_FAILED)
        return NULL;

    close(fd); // Close file

    return mapped;
}


pthread_t* init_threads(long num_cores, size_t filesize, char* file_start) {
    // Get size of each thread's section
    long subsect_size = filesize / num_cores;

    // Create array of each thread's ID
    pthread_t* thread_ids = malloc(num_cores * sizeof(pthread_t));

    if(!thread_ids) // Allocation failure
        return NULL;

    // Create a thread for each core
    for(int i = 0; i < num_cores; i++) {
        // Define subsection offsets
        long start_offset = i * subsect_size;
        long end_offset;

        // Calculate end offset
        if(i == num_cores - 1) // Assign final thread remainder of file
            end_offset = filesize;
        else // Assign fixed size chunk
            end_offset = (i + 1) * subsect_size;
        
        // Allocate memory for thread arguments
        ThreadArgs* args = malloc(sizeof(ThreadArgs));

        if(!args) { // Allocation failed
            free(thread_ids);
            return NULL;
        }

        // Initialize ThreadArgs fields
        args->file_start = file_start;
        args->start_offset = start_offset;
        args->end_offset = end_offset;
        args->file_size = filesize;

        if(i == 0) // 1st subsection must read first word
            args->read_first = 1;

        // Create thread
        pthread_create(&thread_ids[i], NULL, read_section, (void*)args);
    }

    return thread_ids;
}


char count_words(char* filepath) {
    // Get number of logical cores available 
    long num_cores = sysconf(_SC_NPROCESSORS_ONLN);

    // System error
    if(num_cores < 1)
        // Terminate program
        return 0;

    size_t filesize = 0;
    char* mapped = map_file(filepath, &filesize);

    if(!mapped)
        return 0;

    // Get size of each subsection
    long subsect_size = filesize / num_cores;

    // Launch threads to read file
    pthread_t* thread_ids = init_threads(num_cores, filesize, mapped);

    if(!thread_ids)
        return 0;

    // Create array to hold thread's return value
    void** thread_results = malloc(num_cores * sizeof(Tree*));

    if(!thread_results) // Allocation failure
        return 0;

    for(int i = 0; i < num_cores; i++) // Synchronize threads
        pthread_join(thread_ids[i], &thread_results[i]); // Collect return value
    
    Tree** dicts = (Tree**)thread_results; // Cast output to Tree's

    // Merge and write results to file
    char res = write_dict(dicts, num_cores, filepath);

    if(!res) // Check for write failure
        printf("Dictionary failed to save\n");

    // Free Allocated Memory
    for(int i = 0; i < num_cores; i++) {
        if(dicts && dicts[i])
            tree_free(dicts[i]);
    }

    free(thread_ids);
    free(thread_results);
    

    return res;
} 