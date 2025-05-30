#include <stdio.h>
#include <string.h>
#include "../include/tree.h"
#include "../include/count_words.h"


int main(int argc, char *argv[]) {
    if(argc != 2) {
        printf("single path to text file must be include as program argument\n");
        return 1;
    }

    char result = count_words(argv[1]);

    if(result)
        printf("Words counted successfully\n");
    else
        printf("Word Counting Failed\n");
}