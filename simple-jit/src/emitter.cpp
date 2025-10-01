#include "emitter.h"

void emit_movz(uint32_t dest, int imm, std::vector<uint32_t> &output) {
    // mov wide with zero
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/MOVZ--Move-wide-with-zero-?lang=en
    uint32_t code = 0xd2800000;
    code |= dest;
    code |= static_cast<uint32_t>(imm << 5);
    output.push_back(code);
}

void emit_add_sr(uint32_t dest, uint32_t src0, uint32_t src1,
                 std::vector<uint32_t> &output) {
    // add shifted register
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/ADD--shifted-register---Add-optionally-shifted-register-?lang=en
    uint32_t code = 0x8b000000;
    code |= src0 << 5;  // Rn
    code |= src1 << 16; // Rm
    code |= dest;       // Rd
    output.push_back(code);
}

void emit_orr_sr(uint32_t dest, uint32_t src, std::vector<uint32_t> &output) {
    // ORR - Bitwise OR
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/ORR--shifted-register---Bitwise-OR--shifted-register--?lang=en
    uint32_t code = 0xaa0003e0;
    code |= src << 16;
    code |= dest;
    output.push_back(code);
}

void emit_ret(std::vector<uint32_t> &output) {
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/RET--Return-from-subroutine-?lang=en
    // assume the return address is in w30 reg!
    uint32_t code = 0xd65f03c0;
    output.push_back(code);
}
