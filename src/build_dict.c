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


#define FILE_OUT "data.bin"
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


char write_word_count(FILE* file, unsigned long long unique_words, unsigned long long num_words, long unique_offset, long word_offset) {
    // Replace unique words
    if(!replace_data(file, unique_words, unique_offset))
        return 0;

    // Replace total words
    if(!replace_data(file, num_words, word_offset))
        return 0;

    return 1;
}

Tree** free_dicts(Tree** dicts, int num_dicts) {
    for(int i = 0; i < num_dicts; i++)
        tree_free(dicts[i]); // Free trees

    free(dicts); // Free tree array

    return NULL;
}

char free_data(Tree** dicts, TreeIter** iters, WordQueue* queue, FILE* file, char* word, int num_trees) {
    // Free all trees and iterators
    for(int i = 0; i < num_trees; i++) {
        if(dicts && dicts[i]) // Free tree
            tree_free(dicts[i]);
        if(iters && iters[i]) // Free iterator
            tree_iter_free(iters[i]);
    }

    // Free tree and iter arrays
    if(dicts) // Free tree array
        free(dicts);
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

FILE* init_out_file(const char* input_name, long* unique_offset, long* word_offset) {
    FILE* file = fopen(FILE_OUT, "wb"); // Open file to write

    if(!file) // File failed to open
        return NULL;

    long len = strlen(input_name); // Get length of name
    
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
    *unique_offset = ftell(file); // Assign start of unique words

    if(*unique_offset == -1l) { // ftell failure
        fclose(file);
        return NULL;
    }

    // Write number of unique words
    if(fwrite(&dummy_data, sizeof(dummy_data), 1, file) != 1) {
        // Write failed
        fclose(file);
        return NULL;
    }

    *word_offset = ftell(file); // Assign start of total words

    if(*word_offset == -1l) { // ftell failure
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

char write_dict(Tree** dicts, int num_cores, const char* input_name) {
    // Number of words counter
    unsigned long long num_words;
    unsigned long long unique_words;

    // Offset of word position
    long unique_offset = 0;
    long num_offset = 0;

    // Get file with header containing dummy data
    FILE* file = init_out_file(input_name, &unique_offset, &num_offset);

    if(!file) // File failed to open
        return 0;

    // Create array too hold tree iterators
    TreeIter** next = malloc(num_cores * sizeof(TreeIter*));

    if(!next) // Allocation failed
        return 0; //free_data(dicts, NULL, NULL, file, NULL, num_cores);

    for(int i = 0; i < num_cores; i++) { // Iterate tree dicts
        if(!dicts[i]) // Only write if all tree's non-null
            return 0; //free_data(dicts, next, NULL, file, NULL, num_cores);

        next[i] = tree_iter_create(dicts[i]); // Create tree iterator

        if(!next[i]) // Allocation failed
            return 0; // free_data(dicts, NULL, NULL, file, NULL, num_cores);
    }

    // Create priority queue to hold words
    WordQueue* word_queue = word_queue_create(num_cores);

    if(!word_queue) // Allocation failed
        return 0; //free_data(dicts, NULL, NULL, file, NULL, num_cores);
    
    // Populate queue with word from each tree
    for(int i = 0; i < num_cores; i++) {
        if(tree_iter_has_next(next[i])) {
            // Get word and count
            unsigned long long count;
            char* word = tree_iter_get(next[i], &count);
            word_queue_insert(word_queue, word, count, i);

            #ifdef DBG
            printf("Initial Word: %s x %llu\n", word, count);
            #endif
        }
    }

    // Write words and counts to file
    while(!word_queue_is_empty(word_queue)) {
        // Get next word to write
        unsigned long long count;
        int index;
        char* word = word_queue_get_min(word_queue, &count, &index);

        #ifdef DBG
        printf("\nWord Popped: {%s}(%llu)\n", word, count);
        #endif

        // Add word from tree of next word to queue
        if(tree_iter_has_next(next[index])) {
            unsigned long long new_count;           
            char* new_word = tree_iter_get(next[index], &new_count);
            word_queue_insert(word_queue, new_word, new_count, index);
        }

        #ifdef DBG
        char* test_word = word_queue_peak(word_queue);
        if(!test_word)
            printf("\nWORD POPPED FROM QUEUE IS NULL\n");
        else 
            printf("\nPopped from queue: %s\n", test_word);
        #endif


        
        // Add duplicate words from queue
        while(!word_queue_is_empty(word_queue) && !strcmp(word, word_queue_peak(word_queue))) {
            // Get duplicate word count
            unsigned long long dup_count;
            free(word_queue_get_min(word_queue, &dup_count, &index));

            #ifdef DBG
            printf("Duplicate count: %llu\n", dup_count);
            #endif

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
            return 0; //free_data(dicts, next, word_queue, file, word, num_cores);

        free(word); // Deallocate word;

        // Increment word counts
        num_words += count;
        unique_words++;
    }

    // Set total and unique words
    if(!write_word_count(file, unique_words, num_words, unique_offset, num_offset))
        return 0; //free_data(dicts, next, word_queue, file, NULL, num_cores);

    // free_data(dicts, next, word_queue, file, NULL, num_cores);

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

    #ifdef DBG
    printf("\nThread Launched\n");
    printf("size: %ld, start: %ld, end: %ld\n", file_size, start, end);
    #endif

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
        #ifdef DBG
        printf("Word Reading begun\n");
        #endif

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
            #ifdef DBG
            printf("Data[i] = %c\n", data[i]);
            #endif

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

    // Open file
    int fd = open(filepath, O_RDONLY);
    if (fd < 0) {
        perror("open");
        exit(1);
    }

    // Get total filesize
    struct stat st;
    if(fstat(fd, &st) < 0)
        return 0;
    size_t filesize = st.st_size;

    // Map whole file
    char* mapped = mmap(NULL, filesize, PROT_READ, MAP_PRIVATE, fd, 0);
   
    if(mapped == MAP_FAILED) {
        close(fd);
        return 0;
    }

    close(fd); // Close file

    // Get size of each subsection
    long subsect_size = st.st_size / num_cores;

    // Create array of each thread's ID
    pthread_t* thread_ids = malloc(num_cores * sizeof(pthread_t));

    if(!thread_ids) // Allocation failure
        return 0;

    // Create array to hold thread's return value
    void** thread_results = malloc(num_cores * sizeof(Tree*));

    if(!thread_results) // Allocation failure
        return 0;

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
        args->file_start = mapped;
        args->start_offset = start_offset;
        args->end_offset = end_offset;
        args->file_size = st.st_size;

        if(i == 0) // 1st subsection must read first word
            args->read_first = 1;

        // Create thread
        pthread_create(&thread_ids[i], NULL, read_section, (void*)args);
    }

    for(int i = 0; i < num_cores; i++) // Synchronize threads
        pthread_join(thread_ids[i], &thread_results[i]); // Collect return value
    
    Tree** dicts = (Tree**)thread_results; // Cast output to Tree's

    #ifdef DBG
    printf("\n\nResults\n");
    for(int i = 0; i < num_cores; i++) {
        printf("\n\nThread %d:\n", i);
        tree_print(dicts[i], print_word);
    }
    #endif


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