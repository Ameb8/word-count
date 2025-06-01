#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/stat.h>
#include "../include/tree.h"
#include "../include/min_queue.h"
#include "../include/build_dict.h"


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


#define WORD_BUF_SIZE 256


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



// Read subsection of file and return Tree containing word count
void* thread_read(void* arg) {
    ThreadArgs* args = (ThreadArgs*)arg;

    FILE* file = fopen(args->filepath, "r");

    if(!file)
        return NULL;
    
    // Move position indicator to start of thread's subsection
    if(fseek(file, args->start_offset, SEEK_SET)) {
        fclose(file); // close file
        return NULL; // fseek failed
    }

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

    if(!args->read_first) { // Dont skip for first thread
        // Advance file pointer to first delimiter
        while((c = fgetc(file)) != EOF) {
            if(isspace(c))
                break; // Delimiter found

            long pos = ftell(file); // Get file pointer position

            if(pos == -1) { // Error getting file position
                fclose(file);
                return NULL;
            }
                
            if(pos > args->end_offset)
                break; // No full words in section
        }
    }

    // Create tree to hold words
    Tree* dict = tree_create(compare_str);

    // Add words until end of file or section
    while(1) {
        // Get file pointer position
        long word_start = ftell(file); 

        if(word_start >= args->end_offset)
            break; // End of section reached

        if(word_start == -1) {
            fclose(file);
            return NULL; // Error detected
        }

        // Read next word
        int ret = fscanf(file, "%255s", word);

        // End of file reached
        if(ret == EOF)
            break;

        // Record word in tree
        tree_set(dict, word, strlen(word) + 1, set_word_count);
    }


    // Free thread argument and file memory
    free(arg);
    fclose(file);

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

    #ifdef DBG
    for(int i = 0; i < num_cores; i++) {
        printf("\n\nThread %d:\n", i);
        tree_print(dicts[i], print_word);
    }
    #endif


    // Merge and write results to file
    //res = write_file(dicts, num_cores);

    // Free Allocated Memory
    for(int i = 0; i < num_cores; i++) {
        if(dicts[i])
            tree_free(dicts[i]);
    }

    free(thread_ids);
    free(thread_results);
    

    return 0;
} 