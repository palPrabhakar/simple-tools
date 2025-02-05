#!/usr/bin/bash

CFLAGS="-Wall -Wpedantic -Wextra -Werror -g -O0"
if [ $# -eq 0 ]; then
    if [[ $OSTYPE == "freebsd"* ]]; then
        clang $CFLAGS -static main.c read_elf.c -o main && ./main
    elif [[ $OSTYPE == "linux-gnu"* ]]; then
        gcc $CFLAGS main.c read_elf.c -o main && ./main
    fi
else
    if [[ $OSTYPE == "freebsd"* ]]; then
        clang++ $CFLAGS -static main.cpp read_elf.c -o main && ./main
    elif [[ $OSTYPE == "linux-gnu"* ]]; then
        g++ $CFLAGS main.cpp read_elf.c -o main && ./main
    fi
fi

