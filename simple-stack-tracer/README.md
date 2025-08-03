# simple-stack-tracer

A lightweight C/C++ stack tracer that prints call stack information by reading ELF symbol tables and walking stack frames.

## Overview

This tool demonstrates how to implement a basic stack tracer for C/C++ programs on Linux and FreeBSD systems. It works by:
- Reading ELF symbol tables from the executable
- Walking the stack using frame pointer (RBP) register
- Resolving addresses to function names

## Features

- Cross-platform support (Linux and FreeBSD)
- C and C++ compatibility
- Inline assembly for stack frame access
- ELF symbol table parsing
- Macro-based stack trace printing

## Building and Running

Use the provided build script:

```bash
# Build and run C version
./build.sh

# Build and run C++ version  
./build.sh cpp
```

The build script automatically detects your platform and uses the appropriate compiler (GCC on Linux, Clang on FreeBSD).

## Files

- `main.c` - C demo program with test functions
- `main.cpp` - C++ demo program  
- `read_elf.c` - ELF symbol table reader implementation
- `read_elf.h` - Header with stack tracer interface and macros
- `build.sh` - Cross-platform build script

## Usage

Include `read_elf.h` and call:
1. `init_stack_tracer(argv[0])` - Initialize with executable path
2. `PRINT_STACK_TRACE` - Print current call stack (macro)
3. `deinit_stack_tracer()` - Cleanup resources

## Example Output

```
Simple stack tracer

return addr foo: 0x401234, name: test
return addr foo: 0x401567, name: foo  
return addr foo: 0x401890, name: bar
return addr bar: 0x401abc, name: main
```
