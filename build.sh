#!/bin/bash

gcc -g -O0 main.c read_elf.c -o main && ./main
