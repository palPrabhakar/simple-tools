#ifndef _READ_ELF_H
#define _READ_ELF_H

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct symbol {
  size_t address;
  const char *symbol_name;
} symbol_t;

typedef struct symbols {
  size_t len;
  symbol_t *symbols;
} symbols_t;

void init_stack_tracer(const char *);
const char *find_name(uint64_t);

__attribute__((__always_inline__)) static inline int64_t get_rbp() {
  int64_t rbp;
  __asm__ volatile("mov %%rbp, %0"
                   : "=r"(rbp) // Output operand
                   :           // No input operands
                   :           // No clobbered registers
  );
  return rbp;
}

#define PRINT_STACK_TRACE                                                      \
  int64_t rbp = get_rbp();                                                     \
  uint64_t addr = *(uint64_t *)(rbp + 8);                                      \
  while (strcmp(find_name(addr), "main") != 0) {                               \
    printf("return addr foo: 0x%lx, name: %s\n", addr, find_name(addr));       \
    rbp = *(uint64_t *)rbp;                                                    \
    addr = *(uint64_t *)(rbp + 8);                                             \
  }                                                                            \
  printf("return addr bar: 0x%lx, name: %s\n", addr, find_name(addr));


#endif
