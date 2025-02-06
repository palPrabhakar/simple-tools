#include "fat.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

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

static int search_dirs(dir_t *dir, int len, const char *dname, dir_t *rval) {
    for (size_t i = 0;
         i < len && (uint8_t)dir[i].DIR_Name[0] != 0x00; ++i) {
        if ((uint8_t)dir[i].DIR_Name[0] != 0xE5) {
            char name[12] = {'\0'};
            size_t j;
            for (j = 0; j < 8 && dir[i].DIR_Name[j] != ' '; ++j)
                name[j] = dir[i].DIR_Name[j];
            if (dir[i].DIR_Attr != 0x10) {
                name[j++] = '.';
                for (int k = 8; k < 11 && dir[i].DIR_Name[k] != ' '; ++k)
                    name[j++] = dir[i].DIR_Name[k];
            }

            if (strcmp(name, dname) == 0) {
                memcpy(rval, &dir[i], sizeof(dir_t));
                return 0;
            }
        }
    }
    return 1;
}

int get_dir(char **plist, size_t idx, size_t plen, fat_fuse *ff, dir_t *dir, size_t dlen, dir_t *rval) {
    assert(idx < plen && "error: get_dir index > length\n");
    if(search_dirs(dir, dlen, plist[idx], rval)) {
        return 1;
    }
    if(idx == 0 && idx == plen - 1) {
        return 0;
    }
    return 0;
}
