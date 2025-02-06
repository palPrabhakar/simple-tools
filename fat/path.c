#include "fat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const size_t increment_factor = 10;

int read_root_dir(fat_fuse **ff) {
    fseek((*ff)->fp, (*ff)->root_dir_off, SEEK_SET);
    dir_t *dir = malloc(sizeof(dir_t) * (*ff)->root_dir_ent);
    size_t read = fread(dir, sizeof(dir_t), (*ff)->root_dir_ent, (*ff)->fp);
    if (read != (*ff)->root_dir_ent) {
        printf("read: %lu", read);
        free(dir);
        return 0;
    }
    (*ff)->root_dir = dir;
    return read;
}

char **parse_path(char *path, size_t *count) {
    if (path == NULL || path[0] != '/')
        return NULL;

    // initial capacity 10
    size_t limit = increment_factor;
    char **paths = malloc(limit * sizeof(char *));
    char **npaths = NULL;

    *count = 0;
    char *token = strtok(path, "/"); // Use "/" as delimiter
    while (token != NULL) {
        if (*count == limit) {
            limit += increment_factor;
            npaths = realloc(paths, limit * sizeof(char *));
            if (npaths) {
                paths = npaths;
            } else {
                free(paths);
                exit(1);
            }
        }

        paths[(*count)++] = token;
        token = strtok(NULL, "/");
    }
    ++(*count);
    return paths;
}

// void get_fat_obj(char **plist, size_t idx, size_t len, fat_fuse *ff,
//                  dir_t *dir) {
//     if (idx == len - 1) {
//         // last one
//     }
// }
