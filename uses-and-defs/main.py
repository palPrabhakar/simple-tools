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
        self.op0 = operands[0]
        self.op1 = operands[1]
        self.op2 = operands[2]
        self.defs = []
        self.uses = []
        self.idx = 0

    def set_block(self, block):
        self.block = block

    def __str__(self):
        if not self.op0:
            return f"{self.instr}"
        elif not self.op1:
            return f"{self.instr} {self.op0}"
        elif not self.op2:
            return f"{self.instr} {self.op0}, {self.op1}"
        else:
            return f"{self.instr} {self.op0}, {self.op1}, {self.op2}"


class Function:
    def __init__(self, name, instrs):
        self.name = name
        self.instrs = instrs
        self.bb = get_bb(self.instrs)
        build_cfg(self.bb)
        build_uses_and_defs(self.bb)

    def print_cfg(self):
        for b in self.bb:
            print(b)

    def __str__(self):
        instrs = '\n\t'.join(map(str, self.instrs))
        return f"{self.name}\n\t{instrs}"


def find_uses(ins, ii, block, done):
    if block.name in done:
        return

    done[block.name] = True
    instr = block.instrs
    for idx in range(ii, len(instr)):
        if instr[idx].instr in {"sd", "mv", "sw", "lw", "sext.w", "ld"}:
            if ins.op0 in instr[idx].op1:
                ins.uses.append(instr[idx])
                instr[idx].defs.append(ins)

        if instr[idx].instr in {"addi", "mulw", "andi"}:
            if ins.op0 in instr[idx].op1 or ins.op0 in instr[idx].op2:
                ins.uses.append(instr[idx])
                instr[idx].defs.append(ins)

        if instr[idx].op0 == ins.op0:
            return

    for succ in block.succ:
        find_uses(ins, 0, succ, done)


def build_uses_and_defs(bb):
    for b in bb:
        for i, ins in enumerate(b.instrs):
            if ins.instr in {"addi", "mv", "sw", "lw", "andi", "mulw", "sext.w", "ld", "slliw"}:
                find_uses(ins, i+1, b, {})


def build_cfg(bb):
    if len(bb) == 0:
        exit(1)

    def insert_nodes(src, dst):
        src.succ.append(dst)
        dst.pred.append(src)

    imap = {x.name: i for i, x in enumerate(bb)}

    for i, block in enumerate(bb):
        if block.instrs[-1].instr in conditional_jumps_riscv:
            insert_nodes(block, bb[imap[block.instrs[-1].op2]
                                   ])
            assert not i == (len(bb) - 1), "build cfg: conditional_jumps_riscv"
            insert_nodes(block, bb[i+1])
        elif block.instrs[-1].instr in unconditional_jumps_riscv:
            if block.instrs[-1].instr == 'call':
                assert not i == (len(bb) - 1), "build cfg: call instr"
                insert_nodes(block, bb[i+1])
            elif block.instrs[-1].instr == 'j':
                insert_nodes(block, bb[imap[block.instrs[-1].op0]
                                       ])
        else:
            assert not i == (len(bb) - 1), "build cfg: instr"
            insert_nodes(block, bb[i+1])


def get_bb(instrs):
    bblocks = []
    cblock = Block("entry")

    i = 0
    idx = 0
    for ins in instrs:
        ins.idx = idx
        idx = idx + 1
        if ins.instr in block_terminators_riscv:
            cblock.append(ins)
            bblocks.append(cblock)
            cblock = Block(f"block{i}")
            i = i + 1
        elif not ins.op0:
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


def handle_click(event, tbox, func):
    tbox.tag_remove("h0", "1.0", "end")
    tbox.tag_remove("h1", "1.0", "end")
    tbox.tag_remove("h2", "1.0", "end")

    off = 2
    ln = int(tbox.index(f"@{event.x},{event.y}").split(".")[0]) - off
    if not ln < len(func.instrs):
        return

    tbox.tag_add("h1", f"{ln + off}.0", f"{ln + off}.end")
    tbox.tag_config("h1", background="lightblue")

    for ins in func.instrs[ln].defs:
        tbox.tag_add("h0", f"{ins.idx + off}.0", f"{ins.idx + off}.end")
    tbox.tag_config("h0", background="lightgreen")

    for ins in func.instrs[ln].uses:
        tbox.tag_add("h2", f"{ins.idx + off}.0", f"{ins.idx + off}.end")
    tbox.tag_config("h2", background="lightyellow")


def show(function):
    def format_operands(inst):
        def format_operand(op):
            try:
                _ = int(op)
                text.insert("end", f"{op}", "num")
            except Exception:
                if '(' in op and ')' in op:
                    num, rest = op.split('(')
                    reg, *_ = rest.split(')')
                    text.insert("end", f"{num}", "num")
                    text.insert("end", "(")
                    text.insert("end", f"{reg}", "str")
                    text.insert("end", ")")
                else:
                    text.insert("end", f"{op}", "str")

        text.insert("end", f"\t{inst.instr}\t", "instr")
        text.insert("end", f"{inst.op0}", "str")

        if not inst.op1:
            text.insert("end", "\n")
            return

        text.insert("end", ",")
        format_operand(inst.op1)

        if not inst.op2:
            text.insert("end", "\n")
            return

        text.insert("end", ",")
        format_operand(inst.op2)
        text.insert("end", "\n")

    root = tk.Tk()
    root.title("Uses & Defs")

    frame = ttk.Frame(root)
    frame.pack(fill="both", expand=True)

    text = tk.Text(frame, font=("JetBrains Mono", 14),  padx=10, pady=10)
    text.pack(fill="both", expand=True)

    text.tag_config("label", foreground="blue")
    text.tag_config("instr", foreground="red")
    text.tag_config("str", foreground="green")
    text.tag_config("num", foreground="magenta")
    text.insert("end", f"{function.name}:\n", "label")
    for i, inst in enumerate(function.instrs):
        if not inst.op0:
            text.insert("end", f"{inst}\n", "label")
        else:
            format_operands(inst)
    text.bind("<Button-1>", lambda event: handle_click(event, text, function))

    root.mainloop()


def main():
    print("Def and Use")
    function = read_assembly("risc-v2.asm")
    show(function)


if __name__ == '__main__':
    main()
