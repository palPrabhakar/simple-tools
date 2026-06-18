#include <cassert>
#include <cstdint>

#include "emitter.h"

static void emit_mov_helper(uint32_t code, reg_t dest, uint16_t imm,
                            unsigned char shift,
                            std::vector<uint32_t> &output) {
    code |= dest;
    code |= static_cast<uint32_t>(imm) << 5;
    code |= static_cast<uint32_t>(shift) << 21;
    output.push_back(code);
}

static inline void emit_binop_helper(uint32_t code, reg_t dest, reg_t src0,
                                     reg_t src1,
                                     std::vector<uint32_t> &output) {
    code |= dest;       // Rd
    code |= src0 << 5;  // Rn
    code |= src1 << 16; // Rm
    output.push_back(code);
}

static void emit_stp_ldp(uint32_t code, reg_t rn, reg_t rt1, reg_t rt2,
                         int32_t offset, ls_mode mode,
                         std::vector<uint32_t> &output) {
    assert((offset & 7) == 0 && "err: offset must be a multiple of 8\n");

    code |= static_cast<uint32_t>(mode) << 23;
    offset = (offset / 8) & 0x7f;
    code |= static_cast<uint32_t>(offset) << 15;
    code |= rn << 5;
    code |= rt2 << 10;
    code |= rt1;
    output.push_back(code);
}

static void emit_ldr_str(uint32_t code, reg_t rn, reg_t rt, int32_t imm,
                         ls_mode mode, std::vector<uint32_t> &output) {
    if (mode == ls_mode::offset) {
        // assert(!(imm >> 12) && "err: imm not 12 bit\n");
        code |= _cast_uint32(mode) << 23;
        code |= _cast_uint32((imm & 0xFFF)/ 8) << 10;
    } else {
        // assert(!(imm >> 9) && "err: imm not 9 bit\n");
        code |= _cast_uint32(mode) << 10;
        code |= _cast_uint32((imm & 0x1FF)) << 12;
    }
    code |= rt;
    code |= rn << 5;
    output.push_back(code);
}

void emit_movz(reg_t dest, uint16_t imm, unsigned char shift,
               std::vector<uint32_t> &output) {
    // mov wide with zero
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/MOVZ--Move-wide-with-zero-?lang=en
    uint32_t code = 0xd2800000;
    emit_mov_helper(code, dest, imm, shift, output);
}

void emit_movn(reg_t dest, uint16_t imm, unsigned char shift,
               std::vector<uint32_t> &output) {
    // move with with not
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/MOVN--Move-wide-with-NOT-?lang=en
    uint32_t code = 0x92800000;
    emit_mov_helper(code, dest, static_cast<uint16_t>(~imm), shift, output);
}

void emit_movk(reg_t dest, uint16_t imm, unsigned char shift,
               std::vector<uint32_t> &output) {
    // move with with keep
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/MOVK--Move-wide-with-keep-
    uint32_t code = 0xf2800000;
    emit_mov_helper(code, dest, imm, shift, output);
}

void emit_mov_sp(reg_t dest, reg_t src, std::vector<uint32_t> &output) {
    // move to/from SP
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/MOV--to-from-SP---Move-register-value-to-or-from-SP--an-alias-of-ADD--immediate--
    assert((dest == SP || src == SP) && "err: either src or dest must be SP\n");

    uint32_t code = 0x91000000;
    code |= dest;
    code |= src << 5;
    output.push_back(code);
}

void emit_add_sr(reg_t dest, reg_t src0, reg_t src1,
                 std::vector<uint32_t> &output) {
    // add shifted register
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/ADD--shifted-register---Add-optionally-shifted-register-?lang=en
    uint32_t code = 0x8b000000;
    emit_binop_helper(code, dest, src0, src1, output);
}

void emit_sub_sr(reg_t dest, reg_t src0, reg_t src1,
                 std::vector<uint32_t> &output) {
    // sub shifted register
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/SUB--shifted-register---Subtract-optionally-shifted-register-?lang=en
    uint32_t code = 0xcb000000;
    emit_binop_helper(code, dest, src0, src1, output);
}

void emit_mul(reg_t dest, reg_t src0, reg_t src1,
              std::vector<uint32_t> &output) {
    // mul
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/MUL--Multiply--an-alias-of-MADD-?lang=en
    uint32_t code = 0x9b007c00;
    emit_binop_helper(code, dest, src0, src1, output);
}

void emit_sdiv(reg_t dest, reg_t src0, reg_t src1,
               std::vector<uint32_t> &output) {
    // sdiv
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/SDIV--Signed-divide-?lang=en
    uint32_t code = 0x9ac00c00;
    emit_binop_helper(code, dest, src0, src1, output);
}

void emit_orr_sr(reg_t dest, reg_t src, std::vector<uint32_t> &output) {
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

void emit_blr(reg_t addr, std::vector<uint32_t> &output) {
    // branch to an address in register and link
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/BLR--Branch-with-link-to-register-
    uint32_t code = 0xd63f0000;
    code |= addr << 5;
    output.push_back(code);
}

void emit_stp(reg_t daddr, int32_t doffset, reg_t src1, reg_t src2,
              ls_mode mode, std::vector<uint32_t> &output) {
    // store reg src1, src2 at addr
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/STP--Store-pair-of-registers-?lang=en#iclass_pre_index
    uint32_t code = 0xa8000000;
    emit_stp_ldp(code, daddr, src1, src2, doffset, mode, output);
}

void emit_ldp(reg_t saddr, int32_t soffset, reg_t dest1, reg_t dest2,
              ls_mode mode, std::vector<uint32_t> &output) {
    // load reg dest1, dest2 from addr
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/LDP--Load-pair-of-registers-
    uint32_t code = 0xa8400000;
    emit_stp_ldp(code, saddr, dest1, dest2, soffset, mode, output);
}

void emit_ldr_imm(reg_t saddr, int32_t soffset, reg_t dest, ls_mode mode,
                  std::vector<uint32_t> &output) {
    // load reg from a mem location
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/LDR--immediate---Load-register--immediate--
    uint32_t code = 0xf8400000;
    emit_ldr_str(code, saddr, dest, soffset, mode, output);
}

void emit_str_imm(reg_t daddr, int32_t doffset, reg_t src, ls_mode mode,
                  std::vector<uint32_t> &output) {
    // store reg to a mem location
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/STR--immediate---Store-register--immediate--
    uint32_t code = 0xf8000000;
    emit_ldr_str(code, daddr, src, doffset, mode, output);
}
