#pragma once

#define TYPE_LIST                                                              \
    X(bool, 'b', 4)                                                            \
    X(char, 'c', 5)                                                            \
    X(float, 'f', 6)                                                           \
    X(int, 'i', 7)

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
    X(and, 1)                                                                  \
    X(or, 1)                                                                   \
    X(not, 1)                                                                  \
    X(jmp, 1)                                                                  \
    X(br, 1)                                                                   \
    X(call, 1)                                                                 \
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
    X(id, 1)                                                                   \
    X(const, 1)                                                                \
    X(print, 1)                                                                \
    X(label, 1)                                                                \
    X(nop, 1)                                                                  \
    X(set, 0)                                                                  \
    X(get, 0)                                                                  \
    X(alloc, 1)                                                                \
    X(free, 1)                                                                 \
    X(store, 1)                                                                \
    X(load, 1)                                                                 \
    X(ptradd, 1)                                                               \
    X(speculate, 0)                                                            \
    X(commit, 0)                                                               \
    X(guard, 0)                                                                \
    X(ceq, 1)                                                                  \
    X(clt, 1)                                                                  \
    X(cle, 1)                                                                  \
    X(cgt, 1)                                                                  \
    X(cge, 1)                                                                  \
    X(char2int, 0)                                                             \
    X(int2char, 0)                                                             \
    X(float2bits, 0)                                                           \
    X(bits2float, 0)                                                           \
    X(any, 0)
