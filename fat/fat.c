#include "fat.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const size_t increment_factor = 10;

static int get_num_clusters(fat_fuse *ff, size_t cluster) {
    assert(cluster > 0x0001 && "error: invalid cluster number");
    size_t n_clusters = 1;
    size_t cur = cluster;
    while (ff->fat_dir[cur] < 0xFFF7 && ff->fat_dir[cur] != 0x0000) {
        ++n_clusters;
        cur = ff->fat_dir[cur];
    }
    if (ff->fat_dir[cur] == 0xFFF7 || ff->fat_dir[cur] == 0x0000) {
        return 0;
    }
    return n_clusters;
}

size_t get_next_cluster(fat_fuse *ff, size_t cluster) {
    assert(cluster > 0x0001 && "error: invalid cluster number");
    if (ff->fat_dir[cluster] == 0xFFF7 || ff->fat_dir[cluster] == 0x0000 ||
        ff->fat_dir[cluster] == 0x0001) {
        return 0xFFFF;
    }
    return ff->fat_dir[cluster];
}

int read_dir(FILE *fp, size_t offset, size_t n_entries, dir_t *dir) {
    fseek(fp, offset, SEEK_SET);
    size_t read = fread(dir, sizeof(dir_t), n_entries, fp);
    if (read != n_entries) {
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

int read_dir_entries(fat_fuse *ff, dir_t dir, dir_t **dirs,
                     size_t *total_entries) {
    size_t n_cluster = get_num_clusters(ff, dir.DIR_FstClusLO);
    if (!n_cluster) {
        return 1;
    }

    size_t n_entries = (ff->sec_per_clus * ff->bytes_per_sec) / sizeof(dir_t);
    *dirs = malloc(n_cluster * ff->sec_per_clus * ff->bytes_per_sec);
    size_t cluster = dir.DIR_FstClusLO;
    for (size_t i = 0; i < n_cluster; ++i) {
        if (read_dir(ff->fp, GET_SECTOR_OFFSET(cluster, ff), n_entries,
                     *dirs + i)) {
            free(dirs);
            return 1;
        }
        cluster = get_next_cluster(ff, cluster);
    }
    *total_entries = n_entries * n_cluster;
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
    if (read_dir_entries(ff, *rval, &dir, &n_entries)) {
        return 1;
    }

    return get_dir(plist, idx + 1, plen, ff, dir, n_entries, rval);
}
