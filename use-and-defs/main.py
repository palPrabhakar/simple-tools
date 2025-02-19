"""
Find the definition and usage of
instructions source and destination
"""

import tkinter as tk
from tkinter import ttk


class Instruction:
    def __init_(self, instr, dest, src0, src1):
        self.instr = instr
        self.dest = dest
        self.src0 = src0
        self.src1 = src1

    def __str__(self):
        return f"{self.instr} {self.dest}, {self.src0}, {self.src1}"


class Function:
    def __init__(self, name, instrs):
        self.name = name
        self.instrs = instrs

    def __str__(self):
        return f"{self.name}"


def parse_instr(instr):
    return instr


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
        tk.Label(mainframe, text=inst, font=("JetBrains Mono", 15)).grid(column=0, row=i+1)

    for child in mainframe.winfo_children():
        child.grid_configure(padx=5, pady=5)

    root.mainloop()


if __name__ == '__main__':
    main()
