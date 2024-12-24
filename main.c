#include <stdint.h>
#include <stdio.h>

#include "read_elf.h"

void read_elf(const char *file_name);
static symbols_t symbols;

uint64_t foo(uint64_t x) {
  int64_t rbp;

  // Inline assembly to read the stack base pointer
  __asm__ volatile("mov %%rbp, %0"
                   : "=r"(rbp) // Output operand
                   :           // No input operands
                   :           // No clobbered registers
  );

  uint64_t addr = *(uint64_t *)(rbp + 8);
  printf("return addr foo: 0x%lx\n", addr);
  rbp = *(uint64_t *)rbp;
  addr = *(uint64_t *)(rbp + 8);
  printf("return addr bar: 0x%lx\n", addr);


  return x;
}

uint64_t bar(void) {
  return foo(42) + 4;
}

int main(int argc, char **argv) {
  printf("Simple stack tracer\n");

  printf("argv[0] %s\n", argv[0]);
  symbols = get_elf_symbols(argv[0]);

  // for (size_t i = 0; i < symbols.len; i++) {
  //   printf("symbol_name[%lu]: %s, 0x%lx\n", i, symbols.symbols[i].symbol_name,
  //          symbols.symbols[i].address);
  // }

  printf("bar ret_val: 0x%lx\n", bar());


  return 0;
}
