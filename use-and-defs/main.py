"""
Find the definition and usage of
instructions source and destination
"""

import tkinter as tk
from tkinter import ttk


class Instruction:
    def __init__(self, instr, operands):
        self.instr = instr
        self.dest = operands[0]
        self.src0 = operands[1]
        self.src1 = operands[2]

    def __str__(self):
        if not self.src1:
            return f"{self.instr} {self.dest}, {self.src0}\n"
        elif not self.src0:
            return f"{self.instr} {self.dest}\n"
        elif not self.dest:
            return f"{self.instr}\n"
        else:
            return f"{self.instr} {self.dest}, {self.src0}, {self.src1}\n"


class Function:
    def __init__(self, name, instrs):
        self.name = name
        self.instrs = instrs

    def __str__(self):
        return f"{self.name}"


def parse_instr(instr):
    name, operands = instr.split()
    operands = [x.strip()
                for x in operands.split(',')]
    return Instruction(name, [*operands, None, None, None])


def read_assembly(file_name):
    with open(file_name) as f:
        name = f.readline().strip()[:-1]
        instrs = [parse_instr(line) for line in f.readlines() if line.strip()]
        function = Function(name, instrs)
    return function


def main():
    print("Def and Use")
    function = read_assembly("risc-v.asm")

    root = tk.Tk()
    root.title(function.name)

    mainframe = ttk.Frame(root, padding="3 3 12 12")
    mainframe.grid(column=0, row=0)
    root.columnconfigure(0, weight=1)
    root.rowconfigure(0, weight=1)

    for i, inst in enumerate(function.instrs):
        tk.Label(mainframe, text=inst, font=(
            "JetBrains Mono", 15)).grid(column=0, row=i+1)

    root.mainloop()


if __name__ == '__main__':
    main()
