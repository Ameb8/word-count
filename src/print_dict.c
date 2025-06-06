#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/print_dict.h"

#define DICT_FILE "data.bin"

char print_dict() {
    FILE* file = fopen(DICT_FILE, "rb"); // Open file to read

    if(!file) // File failed too open
        return 1;

    #ifdef DBG
    printf("data.bin opened for writing\n");
    #endif

    while(1) {
        size_t len;

        // Read the length of the word
        size_t read_len = fread(&len, sizeof(len), 1, file);

        #ifdef DBG
        printf("Read Length: %zu\n", read_len);
        #endif

        if (read_len != 1) {
            if(feof(file)) // End of file reached
                break;
            else if(ferror(file)) // Reading error
                return 1;
        }

        // Allocate memory for next word
        char* word = malloc(len + 1);

        if(!word) // Allocation failure
            return 0;

        // Read the word
        size_t read_word = fread(word, sizeof(char), len, file);

        if(read_word != len) {
            free(word);
            return 0;;
        }

        word[len] = '\0'; // Null-terminate the string

        // Read the count
        unsigned long long count;
        size_t read_count = fread(&count, sizeof(unsigned long long), 1, file);

        if(read_count != 1) { // Count read failed
            free(word);
            return 0;;
        }

        // Print the result
        printf("%s: %llu\n", word, count);
        free(word);
    }

    fclose(file);
    return 1;
}
