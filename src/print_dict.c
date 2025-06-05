#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/print_dict.h"

#define DICT_FILE "data.bin"

char* read_str(FILE* file) {
    if(!file) // Ensure non-null input
        return 0;

    size_t len; // Read the length of the word
    size_t read_len = fread(&len, sizeof(len), 1, file);

    if(read_len != 1) {
        if(feof(file)) // End of file reached
            return "";
        else if(ferror(file)) // Reading error
            return NULL;
    }

    char* word = malloc(len + 1);

    if(!word) // Allocation failure
        return NULL;

    // Read the word
    size_t read_word = fread(word, sizeof(char), len, file);

    // Read failure
    if(read_word != len) {
        free(word);
        return NULL;
    }

    word[len] = '\0'; // Add null terminator

    return word;
}


char read_ull(FILE* file, unsigned long long* num) {
    if(!file) // Ensure non-null input
        return 0;

    size_t read_count = fread(num, sizeof(unsigned long long), 1, file);

    if(read_count != 1)
        return 0;

    return 1;
}


char print_header(FILE* file) {
    if(!file) // Ensure non-null input
        return 0;

    char* title = read_str(file); // Read filename
        
    if(!title || title[0] == '\0') // Title not read
        return 0;

    
    printf("\033[1m%s\n", title); // Print title
    free(title);// Deallocate memory for title
    unsigned long long num_words; // Holds number of words

    if(!read_ull(file, &num_words)) // Read unique words
        return 0; // Read failure

    printf("\nUnique Words: %llu\n", num_words); //print num unique

    if(!read_ull(file, &num_words)) // Read total words
        return 0;

    printf("Total Words: %llu\n\n\n\033[0m", num_words); // print num total

    return 1;
}


char print_dict() {
    FILE* file = fopen(DICT_FILE, "rb"); // Open file to read

    if(!file) // File failed too open
        return 1;

    #ifdef DBG
    printf("data.bin opened for writing\n");
    #endif

    if(!print_header(file)) {
        fclose(file);
        return 0;
    }


    while(1) {
        char* word = read_str(file); // Read word

        #ifdef DBG
        if(!word)
            printf("\nWord is NULL!\n");
        else {
            printf("\nWord 1st char: %c\n", word[0]);
            printf("Word: %s\n", word);
        }
        #endif

        if(!word) // Error while reading
            return 0;

        if(word[0] == '\0') // Finished reading file
            break;

        // Read the count
        unsigned long long count;
        char count_read = read_ull(file, &count);

        if(!count_read) { // Error reading count
            free(word);
            return 0;
        }

        // Print the result
        printf("%s: %llu\n", word, count);
        free(word);
    }

    fclose(file);
    return 1;
}
