#ifndef _READ_ELF_H
#define _READ_ELF_H

#include <stddef.h>

typedef struct symbol {
    size_t address;
    const char *symbol_name;
} symbol_t;

typedef struct symbols {
  size_t len;
  symbol_t *symbols;
} symbols_t;

symbols_t get_elf_symbols(const char *);

#endif
