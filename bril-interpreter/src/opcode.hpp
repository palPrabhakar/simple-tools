#pragma once

#define TYPE_LIST                                                              \
    X(bool, 'b')                                                                       \
    X(float, 'f')                                                                       \
    X(int, 'i')

// X(opcode, supported)
#define OPCODE_LIST                                                            \
    X(add, 1)                                                                  \
    X(mul, 1)                                                                  \
    X(sub, 1)                                                                  \
    X(div, 1)                                                                  \
    X(eq, 1)                                                                   \
    X(lt, 1)                                                                   \
    X(gt, 1)                                                                   \
    X(le, 1)                                                                   \
    X(ge, 1)                                                                   \
    X(and, 0)                                                                  \
    X(or, 0)                                                                   \
    X(not, 0)                                                                  \
    X(jmp, 1)                                                                  \
    X(br, 0)                                                                   \
    X(call, 0)                                                                 \
    X(ret, 1)                                                                  \
    X(fadd, 1)                                                                 \
    X(fmul, 1)                                                                 \
    X(fsub, 1)                                                                 \
    X(fdiv, 1)                                                                 \
    X(feq, 1)                                                                  \
    X(flt, 1)                                                                  \
    X(fgt, 1)                                                                  \
    X(fle, 1)                                                                  \
    X(fge, 1)                                                                  \
    X(id, 0)                                                                   \
    X(const, 1)                                                                \
    X(print, 1)                                                                \
    X(label, 1)                                                                \
    X(nop, 0)
