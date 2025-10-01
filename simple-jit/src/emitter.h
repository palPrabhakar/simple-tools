#pragma once

#include <cstdint>
#include <vector>

constexpr uint64_t ONES_16 = 0xFFFF;

void emit_movz(uint32_t dest, uint64_t imm, std::vector<uint32_t> &output);

void emit_movn(uint32_t dest, int64_t imm, std::vector<uint32_t> &output);

void emit_add_sr(uint32_t dest, uint32_t src0, uint32_t src1,
                 std::vector<uint32_t> &output);

void emit_sub_sr(uint32_t dest, uint32_t src0, uint32_t src1,
                 std::vector<uint32_t> &output);

void emit_mul(uint32_t dest, uint32_t src0, uint32_t src1,
                 std::vector<uint32_t> &output);

void emit_sdiv(uint32_t dest, uint32_t src0, uint32_t src1,
                 std::vector<uint32_t> &output);

void emit_orr_sr(uint32_t dest, uint32_t src, std::vector<uint32_t> &output);

void emit_ret(std::vector<uint32_t> &output);
