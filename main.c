#include <stdint.h>
#include <stdio.h>

#include "read_elf.h"

void read_elf(const char *file_name);

uint64_t foo(uint64_t x) {
  int64_t rbp;

  // Inline assembly to read the stack base pointer
  __asm__ volatile("mov %%rbp, %0"
                   : "=r"(rbp) // Output operand
                   :           // No input operands
                   :           // No clobbered registers
  );

  // printf("0x%lx\n", rbp);
  // printf("0x%lx\n", rbp + 8);
  // printf("0x%lx\n", rbp + 16);
  // printf("\nrbp: 0x%lx\n", (rbp));
  // printf("\n===========\n\n");
  // printf("*(rbp-24): 0x%lx\n", *(uint64_t *)(rbp - 24));
  // printf("*(rbp-16): 0x%lx\n", *(uint64_t *)(rbp - 16));
  // printf("*(rbp-8): 0x%lx\n", *(uint64_t *)(rbp - 8));
  // printf("*rbp: 0x%lx\n", *(uint64_t *)(rbp));
  // printf("*(rbp+8): 0x%lx\n", *(uint64_t *)(rbp + 8));
  // printf("*(rbp+16): 0x%lx\n", *(uint64_t *)(rbp + 16));
  // printf("*(rbp+24): 0x%lx\n", *(uint64_t *)(rbp + 24));

  printf("return addr foo: 0x%lx\n", *(uint64_t *)(rbp + 8));
  rbp = *(uint64_t *)rbp;
  printf("return addr bar: 0x%lx\n", *(uint64_t *)(rbp + 8));


  return x;
}

uint64_t bar(void) {
  return foo(42) + 4;
}

int main(int argc, char **argv) {
  printf("Simple stack tracer\n");
  printf("bar ret_val: 0x%lx\n", bar());

  printf("argv[0] %s\n", argv[0]);
  get_elf_symbols(argv[0]);

  return 0;
}
