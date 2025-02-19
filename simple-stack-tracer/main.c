#include <stdint.h>
#include <stdio.h>

#include "read_elf.h"

int test(void) {
  PRINT_STACK_TRACE
  return 2;
}

uint64_t foo(uint64_t x) { return x + test(); }

uint64_t bar(void) { return foo(42) + 4; }

int main(int argc, char **argv) {
  (void)argc;
  printf("Simple stack tracer\n\n");

  // printf("argv[0] %s\n", argv[0]);
  init_stack_tracer(argv[0]);

  int ret = bar();

  deinit_stack_tracer();

  printf("\n");

  return ret;
}
