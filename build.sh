#!/usr/local/bin/bash

if [[ $OSTYPE == "freebsd"* ]]; then
	clang -g -O0 -static main.c read_elf.c -o main && ./main
elif [[ $OSTYPE == "linux-gnu"* ]]; then
	gcc -g -O0 main.c read_elf.c -o main && ./main
fi
