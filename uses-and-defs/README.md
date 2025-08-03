# uses-and-defs

A RISC-V assembly analyzer and visualizer that displays use-definition chains and control flow graphs with an interactive GUI.

## Overview

This tool analyzes RISC-V assembly code to:
- Build control flow graphs (CFG) from basic blocks
- Track register and variable definitions and uses
- Provide an interactive visualization showing relationships between instructions

## Features

- **Interactive Assembly View**: Click on any instruction to highlight its definitions (green) and uses (yellow)
- **Control Flow Graph**: Visual representation of basic blocks and their connections
- **Use-Definition Analysis**: Tracks how registers and variables flow through the program
- **Syntax Highlighting**: Color-coded assembly with distinct colors for instructions, registers, and immediates
- **Basic Block Construction**: Automatically identifies basic blocks based on control flow

## Requirements

- Python 3.x
- tkinter (usually included with Python)

## Usage

```bash
# Run the visualizer
python3 main.py <assembly_file>

# Or use the convenience script
./run_linux.sh <assembly_file>
```

## Assembly File Format

The tool expects RISC-V assembly files with the following format:
- First line: function name followed by colon (e.g., `square:`)
- Subsequent lines: RISC-V instructions with proper formatting

Example:
```assembly
square:
        addi    sp,sp,-32
        sd      ra,24(sp)
        sd      s0,16(sp)
        addi    s0,sp,32
        mv      a5,a0
        sw      a5,-20(s0)
        lw      a5,-20(s0)
        mulw    a5,a5,a5
        sext.w  a5,a5
        mv      a0,a5
        ld      ra,24(sp)
        ld      s0,16(sp)
        addi    sp,sp,32
        jr      ra
```

## Supported Instructions

The analyzer recognizes various RISC-V instruction categories:

### Conditional Jumps
`beq`, `bne`, `blt`, `bge`, `bltu`, `bgeu`, `bgt`, `ble`, `bgtu`, `bleu`, `beqz`, `bnez`, `bltz`, `bgez`, `bgtz`, `blez`

### Unconditional Jumps  
`jal`, `jalr`, `ret`, `call`, `ecall`, `ebreak`, `j`, `jr`

### Data Movement & Arithmetic
`addi`, `mv`, `sw`, `lw`, `andi`, `mulw`, `sext.w`, `ld`, `sd`, `slliw`

## GUI Features

### Left Panel - Assembly Code
- Syntax-highlighted RISC-V assembly
- Click any instruction to see its relationships
- Selected instruction highlighted in blue
- Definitions highlighted in green
- Uses highlighted in yellow

### Right Panel - Control Flow Graph
- Visual representation of basic blocks
- Arrows show control flow between blocks
- Automatic layout based on program structure

## Files

- `main.py` - Main application with GUI and analysis logic
- `run_linux.sh` - Convenience script to run the application
- `risc-v.asm` - Example RISC-V assembly file (square function)
- `risc-v2.asm` - Additional example assembly file

## Implementation Details

The tool implements several key algorithms:

1. **Basic Block Construction**: Identifies block boundaries using control flow instructions
2. **Control Flow Graph Building**: Links basic blocks based on jumps and fall-through
3. **Use-Definition Analysis**: Tracks register and memory location dependencies
4. **Interactive Visualization**: Maps clicks to instruction relationships

## Example Analysis

When you click on an instruction like `mv a5,a0`:
- **Green highlights**: Instructions that define the value being moved (`a0`)
- **Yellow highlights**: Instructions that use the moved value (`a5`)
- **Blue highlight**: The selected instruction itself

This helps understand data flow and dependencies in the assembly code.