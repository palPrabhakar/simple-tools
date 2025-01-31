#!/usr/local/bin/bash

CFLAGS="-Wall -Wpedantic -Wextra -Werror -g -O0"

if [[ $OSTYPE == "freebsd"* ]]; then
	clang $CFLAGS -static main.c read_elf.c -o main && ./main
elif [[ $OSTYPE == "linux-gnu"* ]]; then
	gcc $CFLAGS main.c read_elf.c -o main && ./main
fi
