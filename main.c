#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "read_elf.h"

void read_elf(const char *file_name);
static symbols_t symbols;

const char *find_name(uint64_t addr) {
  for (size_t i = 0; i < symbols.len - 1; ++i) {
    if(symbols.symbols[i].address == 0x0)
      continue;

    if(symbols.symbols[i].address == symbols.symbols[i+1].address)
      continue;

    int d0 = abs((int)addr - (int)symbols.symbols[i].address);
    int d1 = abs((int)addr - (int)symbols.symbols[i + 1].address);

    // if(d0 == d1)
    //   printf("%s\n", symbols.symbols[i].symbol_name);
    if(symbols.symbols[i+1].address > addr)
      return symbols.symbols[i].symbol_name;

    if (d0 < d1)
      return symbols.symbols[i].symbol_name;
  }
  return symbols.symbols[symbols.len - 1].symbol_name;
}

uint64_t foo(uint64_t x) {
  int64_t rbp;

  // Inline assembly to read the stack base pointer
  __asm__ volatile("mov %%rbp, %0"
                   : "=r"(rbp) // Output operand
                   :           // No input operands
                   :           // No clobbered registers
  );

  uint64_t addr = *(uint64_t *)(rbp + 8);
  printf("return addr foo: 0x%lx, name: %s\n", addr, find_name(addr));
  rbp = *(uint64_t *)rbp;
  addr = *(uint64_t *)(rbp + 8);
  printf("return addr bar: 0x%lx, name: %s\n", addr, find_name(addr));

  return x;
}

uint64_t bar(void) { return foo(42) + 4; }

int main(int argc, char **argv) {
  printf("Simple stack tracer\n");

  printf("argv[0] %s\n", argv[0]);
  symbols = get_elf_symbols(argv[0]);

  // for (size_t i = 0; i < symbols.len; i++) {
  //   printf("symbol_name[%lu]: %s, 0x%lx\n", i,
  //   symbols.symbols[i].symbol_name,
  //          symbols.symbols[i].address);
  // }

  printf("bar ret_val: 0x%lx\n", bar());

  return 0;
}
