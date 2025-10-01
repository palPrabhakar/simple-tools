#include "parser.h"
#include "unordered_map"
#include <format>
#include <ranges>
#include <stdexcept>
#include <string>
#include <unordered_map>

#define PARSE_0(op)                                                            \
    void parse_##op([[maybe_unused]] const sjp::Json &inst,                    \
                    [[maybe_unused]] std::vector<uint32_t> &code) {            \
        throw std::runtime_error("error: opcode: " #op " not implemented\n");  \
    }

#define PARSE_1(op)                                                            \
    void parse_##op(const sjp::Json &inst, std::vector<uint32_t> &code);

#define X(op, bit) PARSE_##bit(op)
OPCODE_LIST
#undef X

#undef EMIT_0
#undef EMIT_1

typedef void (*Parser)(const sjp::Json &, std::vector<uint32_t> &code);

#define TEST_LIST X(add, 0)

const std::unordered_map<std::string, Parser> parser_list = {
#define X(op, bit) {#op, parse_##op},
    OPCODE_LIST
#undef X
};

std::vector<uint32_t> get_code([[maybe_unused]] const sjp::Json &f) {
    std::vector<uint32_t> code;
    auto instrs = f.Get("instrs").value();
    for (auto i : std::views::iota(0ul, instrs.Size())) {
        auto instr = instrs.Get(i).value();
        auto opcode = instr.Get("op")->Get<std::string>().value();
        if (!parser_list.contains(opcode)) {
            throw std::runtime_error(
                std::format("error: unsupported opcode: {}\n", opcode));
        }
        parser_list.at(opcode)(instr, code);
    }
    return code;
}

static inline uint32_t get_register(std::string reg) {
    uint32_t idx =
        static_cast<uint32_t>(std::stoul(reg.substr(1, reg.size() - 1)));
    if (idx >= REG_SIZE) {
        throw std::runtime_error(
            "error: no support for more than 32 registers\n");
    }
    return idx;
}

static inline std::string get_dest(const sjp::Json &inst) {
    return inst.Get("dest")->Get<std::string>().value();
}

void parse_const(const sjp::Json &inst, std::vector<uint32_t> &code) {
    // mov wide with zero
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/MOVZ--Move-wide-with-zero-?lang=en
    uint32_t movz = 0xd2800000;

    if (inst.Get("type")->Get<std::string>() != "int") {
        throw std::runtime_error("error: only int types supported\n");
    }

    int imm = static_cast<int>(inst.Get("value")->Get<double>().value());

    if (imm < 0) {
        throw std::runtime_error(
            "error: no suppport for encoding negative numbers\n");
    }

    uint32_t reg = get_register(get_dest(inst));
    movz |= reg;
    movz |= static_cast<uint32_t>(imm << 5);
    code.push_back(movz);
}

void parse_add(const sjp::Json &inst, std::vector<uint32_t> &code) {
    // add shifted register
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/ADD--shifted-register---Add-optionally-shifted-register-?lang=en
    uint32_t add = 0x8b000000;

    if (inst.Get("type")->Get<std::string>() != "int") {
        throw std::runtime_error("error: only int types supported\n");
    }

    auto args = inst.Get("args").value();
    for (auto i : std::views::iota(0ul, args.Size())) {
        uint32_t src_reg =
            get_register(args.Get(i)->Get<std::string>().value());
        add |= (src_reg << (5 + i * 11));
    }

    uint32_t dest_reg = get_register(get_dest(inst));
    add |= dest_reg;
    code.push_back(add);
}

void parse_return(const sjp::Json &inst, std::vector<uint32_t> &code) {
    // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/RET--Return-from-subroutine-?lang=en
    // assume the return address is in w30 reg!
    uint32_t ret = 0xd65f03c0;

    auto args = inst.Get("args").value();
    assert(args.Size() < 2);

    if (args.Size() == 1) {
        // ORR - Bitwise OR
        // https://developer.arm.com/documentation/ddi0602/2025-09/Base-Instructions/ORR--shifted-register---Bitwise-OR--shifted-register--?lang=en
        uint32_t orr = 0xaa0003e0;
        uint32_t reg = get_register(args.Get(0)->Get<std::string>().value());
        orr |= reg << 16;
        code.push_back(orr);
    }

    code.push_back(ret);
}
