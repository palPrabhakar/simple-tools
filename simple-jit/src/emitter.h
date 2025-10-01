#pragma once

#include <cstdint>
#include <vector>

void emit_movz(uint32_t dest, int, std::vector<uint32_t> &output);

void emit_add_sr(uint32_t dest, uint32_t src0, uint32_t src1,
                 std::vector<uint32_t> &output);

void emit_orr_sr(uint32_t dest, uint32_t src, std::vector<uint32_t> &output);

void emit_ret(std::vector<uint32_t> &output);
