#!/usr/bin/bash

gcc -Wall -D_FILE_OFFSET_BITS=64 main.c `pkg-config fuse3 --cflags --libs` -o main

