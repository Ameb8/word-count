#include <stdio.h>
#include <string.h>
#include <time.h>

#include "../include/tree.h"

// Assumes Node and Tree structs and node_write() are already defined
/*
void dict_save(const Tree* tree, const char* input_filename) {
    FILE* fp = fopen("data.bin", "ab");  // Append mode
    if(!fp) {
        printf("Failed to open data.bin\n");
        return;
    }

    // Record current time
    time_t now = time(NULL);
    if(now == ((time_t)-1)) {
        printf("Failed to get current time\n");
        fclose(fp);
        return;
    }

    // Prepare filename
    size_t name_len = strlen(input_filename);
    fwrite(&name_len, sizeof(size_t), 1, fp);            // Write length of filename
    fwrite(input_filename, sizeof(char), name_len, fp);  // Write filename characters

    // Write time as UNIX timestamp
    fwrite(&now, sizeof(time_t), 1, fp);

    // Write tree itself
    tree_write(fp, tree);

    fclose(fp);
}
*/