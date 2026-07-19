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
    X(eq, 0)                                                                   \
    X(lt, 0)                                                                   \
    X(gt, 0)                                                                   \
    X(le, 0)                                                                   \
    X(ge, 0)                                                                   \
    X(and, 0)                                                                  \
    X(or, 0)                                                                   \
    X(not, 0)                                                                  \
    X(jmp, 0)                                                                  \
    X(br, 0)                                                                   \
    X(call, 0)                                                                 \
    X(ret, 1)                                                                  \
    X(fadd, 1)                                                                 \
    X(fmul, 1)                                                                 \
    X(fsub, 1)                                                                 \
    X(fdiv, 1)                                                                 \
    X(feq, 0)                                                                  \
    X(flt, 0)                                                                  \
    X(fgt, 0)                                                                  \
    X(fle, 0)                                                                  \
    X(fge, 0)                                                                  \
    X(id, 0)                                                                   \
    X(const, 1)                                                                \
    X(print, 1)                                                                \
    X(label, 0)                                                                \
    X(nop, 0)
