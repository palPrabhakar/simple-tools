#!/usr/bin/python3
"""
Find the definition and usage of
instructions source and destination
"""

import tkinter as tk
from tkinter import ttk


block_terminators_riscv = {'jal', 'jalr', 'beq', 'bne', 'blt',
                           'bge', 'bltu', 'bgeu', 'ret', 'call',
                           'j', 'jr', 'ecall', 'ebreak', 'bgt',
                           'ble', 'bgtu', 'bleu', 'beqz', 'bnez',
                           'bltz', 'bgez', 'bgtz', 'blez'}


def get_bb(instrs):
    bblocks = []
    cblock = []

    for ins in instrs:
        if ins.instr in block_terminators_riscv:
            ins.set_block(cblock)
            cblock.append(ins)
            bblocks.append(cblock)
            cblock = []
        elif not ins.dest:
            if len(cblock) == 0:
                ins.set_block(cblock)
                cblock.append(ins)
            else:
                bblocks.append(cblock)
                cblock = [ins]
        else:
            ins.set_block(cblock)
            cblock.append(ins)

    return bblocks


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

    def __str__(self):
        return f"{self.name}"


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
