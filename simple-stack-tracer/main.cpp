#include <stdint.h>
#include <stdio.h>

#include "read_elf.h"

int test(void) {
  PRINT_STACK_TRACE
  return 2;
}

struct Test {
  int foo(int x) { return x + test(); }
};

int bar(void) {
  Test t;
  return t.foo(42) + 4;
}

int main([[maybe_unused]] int argc, char **argv) {
  printf("Simple stack tracer\n\n");

  init_stack_tracer(argv[0]);

  int ret = bar();

  deinit_stack_tracer();

  printf("\n");

  return ret;
}
