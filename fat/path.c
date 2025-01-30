#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const size_t increment_factor = 10;

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
