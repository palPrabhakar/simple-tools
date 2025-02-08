#include "fat.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const size_t increment_factor = 10;

int read_dir(FILE *fp, size_t offset, size_t n_entries, dir_t **dir) {
    fseek(fp, offset, SEEK_SET);
    *dir = malloc(sizeof(dir_t) * n_entries);
    size_t read = fread(*dir, sizeof(dir_t), n_entries, fp);
    if (read != n_entries) {
        free(*dir);
        return 1;
    }
    return 0;
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
    for (size_t i = 0; i < len && (uint8_t)dir[i].DIR_Name[0] != 0x00; ++i) {
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

int get_dir_entries(fat_fuse *ff, size_t cluster, size_t *n_entries) {
    assert(cluster > 0x0001 && "error: invalid cluster number");
    size_t n_clusters = 1;
    size_t cur = cluster;
    while (ff->fat_dir[cur] < 0xFFF7 && ff->fat_dir[cur] != 0x0000) {
        ++n_clusters;
        cur = ff->fat_dir[cur];
    }
    if (ff->fat_dir[cur] == 0xFFF7 || ff->fat_dir[cur] == 0x0000) {
        return 1;
    }
    *n_entries =
        (n_clusters * ff->sec_per_clus * ff->bytes_per_sec) / sizeof(dir_t);
    return 0;
}

int get_dir(char **plist, size_t idx, size_t plen, fat_fuse *ff, dir_t *dir,
            size_t dlen, dir_t *rval) {
    assert(idx < plen && "error: get_dir index > length\n");
    if (search_dirs(dir, dlen, plist[idx], rval)) {
        return 1;
    }

    if (idx == 0 && idx == plen - 1) {
        return 0;
    }

    if (idx == plen - 1) {
        free(dir);
        return 0;
    }

    if (idx != 0) {
        free(dir);
    }

    size_t n_entries;
    if (get_dir_entries(ff, (*rval).DIR_FstClusLO, &n_entries) ||
        read_dir(ff->fp, GET_SECTOR_OFFSET((*rval), ff), 512, &dir)) {
        return 1;
    }

    return get_dir(plist, idx + 1, plen, ff, dir, 512, rval);
}
