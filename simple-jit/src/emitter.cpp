#include <ranges>
#include <stdexcept>

#include "emitter.h"

static void emit_mov_helper(uint32_t code, uint32_t dest, uint64_t imm,
                            std::vector<uint32_t> &output) {
    uint16_t value = 0;
    unsigned char shift = 0;

    for (auto i : std::views::iota(0ul, 4ul)) {
        uint16_t bits = (imm & ONES_16);
        if (value && bits) {
            throw std::runtime_error(
                "error: immeditate cannot be encoded in single mov\n");
        }
        shift = bits ? static_cast<unsigned char>(i) : shift;
        value = bits ? bits : value;
        imm >>= 16;
    }

    code |= dest;
    code |= static_cast<uint32_t>(value) << 5;
    code |= static_cast<uint32_t>(shift) << 21;
    output.push_back(code);
}

void emit_movz(uint32_t dest, uint64_t imm, std::vector<uint32_t> &output) {
    // mov wide with zero
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/MOVZ--Move-wide-with-zero-?lang=en
    uint32_t code = 0xd2800000;
    emit_mov_helper(code, dest, imm, output);
}

void emit_movn(uint32_t dest, int64_t imm, std::vector<uint32_t> &output) {
    // move with with not
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/MOVN--Move-wide-with-NOT-?lang=en
    uint32_t code = 0x92800000;
    emit_mov_helper(code, dest, static_cast<uint64_t>(~imm), output);
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
