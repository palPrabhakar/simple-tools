#pragma once

#include <cstdint>
#include <vector>

#define _cast_uint32(x) static_cast<uint32_t>(x)

constexpr uint64_t ONES_64 = 0xFFFFFFFFFFFFFFFF;
constexpr uint64_t ONES_16 = 0xFFFF;

using reg_t = uint32_t;

constexpr reg_t SP = 0x1F;

enum class LS_MODE { POST = 1, SIGNED = 2, PRE = 3 };

void emit_movz(reg_t dest, uint16_t imm, unsigned char shift,
               std::vector<uint32_t> &output);

void emit_movn(reg_t dest, uint16_t imm, unsigned char shift,
               std::vector<uint32_t> &output);

void emit_movk(reg_t dest, uint16_t imm, unsigned char shift,
               std::vector<uint32_t> &output);

void emit_mov_sp(reg_t dest, reg_t src, std::vector<uint32_t> &output);

void emit_add_sr(reg_t dest, reg_t src0, reg_t src1,
                 std::vector<uint32_t> &output);

void emit_sub_sr(reg_t dest, reg_t src0, reg_t src1,
                 std::vector<uint32_t> &output);

void emit_mul(reg_t dest, reg_t src0, reg_t src1,
              std::vector<uint32_t> &output);

void emit_sdiv(reg_t dest, reg_t src0, reg_t src1,
               std::vector<uint32_t> &output);

void emit_orr_sr(reg_t dest, reg_t src, std::vector<uint32_t> &output);

void emit_ret(std::vector<uint32_t> &output);

void emit_blr(reg_t addr, std::vector<uint32_t> &output);

void emit_stp(reg_t daddr, int32_t doffset, reg_t src1, reg_t src2,
              LS_MODE mode, std::vector<uint32_t> &output);

void emit_ldp(reg_t saddr, int32_t soffset, reg_t dest1, reg_t dest2,
              LS_MODE mode, std::vector<uint32_t> &output);

void emit_ldr_imm(reg_t saddr, int32_t soffset, reg_t dest, LS_MODE mode,
                  std::vector<uint32_t> &output);

void emit_str_imm(reg_t daddr, int32_t doffset, reg_t src, LS_MODE mode,
                  std::vector<uint32_t> &output);
