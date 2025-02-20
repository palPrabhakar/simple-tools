#!/usr/bin/python3
"""
Find the definition and usage of
instructions source and destination
"""

import tkinter as tk
from tkinter import ttk


conditional_jumps_riscv = {'beq', 'bne', 'blt', 'bge', 'bltu', 'bgeu',
                           'bgt', 'ble', 'bgtu', 'bleu', 'beqz', 'bnez',
                           'bltz', 'bgez', 'bgtz', 'blez'}

unconditional_jumps_riscv = {'jal', 'jalr',
                             'ret' 'call', 'ecall', 'ebreak', 'j', 'jr'}

block_terminators_riscv = conditional_jumps_riscv | unconditional_jumps_riscv


class Block:
    def __init__(self, name):
        self.name = name
        self.pred = []
        self.succ = []
        self.instrs = []

    def append(self, instr):
        self.instrs.append(instr)
        instr.set_block(self)

    def __str__(self):
        pred = ', '.join(map(lambda x: x.name, self.pred))
        succ = ', '.join(map(lambda x: x.name, self.succ))
        return f"{self.name}\n\tpred: {pred}\n\tsucc: {succ}"


class Instruction:
    def __init__(self, instr, operands):
        self.instr = instr
        self.dest = operands[0]
        self.src0 = operands[1]
        self.src1 = operands[2]

    def set_block(self, block):
        self.block = block

    def __str__(self):
        if not self.dest:
            return f"{self.instr}"
        elif not self.src0:
            return f"{self.instr} {self.dest}"
        elif not self.src1:
            return f"{self.instr} {self.dest}, {self.src0}"
        else:
            return f"{self.instr} {self.dest}, {self.src0}, {self.src1}"


class Function:
    def __init__(self, name, instrs):
        self.name = name
        self.instrs = instrs
        self.bb = get_bb(self.instrs)
        self.cfg = build_cfg(self.bb)

    def print_cfg(self):
        for b in self.bb:
            print(b)

    def __str__(self):
        instrs = '\n\t'.join(map(str, self.instrs))
        return f"{self.name}\n\t{instrs}"


def build_cfg(bb):
    if len(bb) == 0:
        exit(1)

    def insert_nodes(src, dst):
        src.succ.append(dst)
        dst.pred.append(src)

    imap = {x.name: i for i, x in enumerate(bb)}

    for i, block in enumerate(bb):
        if block.instrs[-1].instr in conditional_jumps_riscv:
            insert_nodes(block, bb[imap[block.instrs[-1].src1]
                                   ])
            assert not i == (len(bb) - 1), "build cfg: conditional_jumps_riscv"
            insert_nodes(block, bb[i+1])
        elif block.instrs[-1].instr in unconditional_jumps_riscv:
            if block.instrs[-1].instr == 'call':
                assert not i == (len(bb) - 1), "build cfg: call instr"
                insert_nodes(block, bb[i+1])
            elif block.instrs[-1].instr == 'j':
                insert_nodes(block, bb[imap[block.instrs[-1].dest]
                                       ])
        else:
            assert not i == (len(bb) - 1), "build cfg: instr"
            insert_nodes(block, bb[i+1])


def get_bb(instrs):
    bblocks = []
    cblock = Block("entry")

    i = 0
    for ins in instrs:
        if ins.instr in block_terminators_riscv:
            cblock.append(ins)
            bblocks.append(cblock)
            cblock = Block(f"block{i}")
            i = i + 1
        elif not ins.dest:
            if len(cblock.instrs) == 0:
                cblock.name = ins.instr[:-1]
            else:
                bblocks.append(cblock)
                cblock = Block(ins.instr[:-1])
        else:
            cblock.append(ins)

    return bblocks


def parse_instr(instr):
    name, *operands = instr.split()
    if len(operands) > 0:
        operands = [x.strip()
                    for x in operands[0].split(',')]
    return Instruction(name, [*operands, None, None, None])


def read_assembly(file_name):
    with open(file_name) as f:
        name = f.readline().strip()[:-1]
        instrs = [parse_instr(line) for line in f.readlines() if line.strip()]
        function = Function(name, instrs)
    return function


def handle_click(event, tbox):
    tbox.tag_remove("highlight", "1.0", "end")
    index = tbox.index(f"@{event.x},{event.y}")
    line_number = index.split(".")[0]
    tbox.tag_add("highlight", f"{line_number}.0", f"{line_number}.end")
    tbox.tag_config("highlight", background="yellow")


def show(function):
    root = tk.Tk()
    root.title(function.name)

    frame = ttk.Frame(root)
    frame.pack(fill="both", expand=True)

    text = tk.Text(frame, font=("JetBrains Mono", 14),  padx=10, pady=10)
    text.pack(fill="both", expand=True)

    for i, inst in enumerate(function.instrs):
        text.insert("end", f"{inst}\n")

    text.bind("<Button-1>", lambda event: handle_click(event, text))

    root.mainloop()


def main():
    print("Def and Use")
    function = read_assembly("risc-v2.asm")
    show(function)


if __name__ == '__main__':
    main()
