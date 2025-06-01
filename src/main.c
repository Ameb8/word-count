#include <stdio.h>
#include <string.h>
#include "../include/tree.h"
#include "../include/build_dict.h"
#include "../include/print_dict.h"

#ifdef TEST
#include "../test/test.h"
#endif



int main(int argc, char *argv[]) {
    #ifdef TEST
    test();
    #endif

    if(argc != 2) {
        printf("single path to text file must be include as program argument\n");
        return 1;
    }

    char result = count_words(argv[1]);

    #ifdef DBG
    if(result)
        printf("Words counted successfully\n");
    else
        printf("Word Counting Failed\n");
    #endif

    //print_dict();
}