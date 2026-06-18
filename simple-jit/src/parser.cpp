#include <array>
#include <format>
#include <ranges>
#include <stdexcept>
#include <string>
#include <map>
#include <sys/types.h>
#include <vector>

#include "emitter.h"
#include "parser.h"

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

std::vector<uint32_t> get_code(const sjp::Json &f) {
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

static inline void check_type_int(const sjp::Json &inst) {
    if (inst.Get("type")->Get<std::string>() != "int") {
        throw std::runtime_error("error: only int type supported\n");
    }
}

static inline reg_t get_register(std::string reg) {
    uint32_t idx =
        static_cast<reg_t>(std::stoul(reg.substr(1, reg.size() - 1)));
    if (idx >= REG_SIZE) {
        throw std::runtime_error(
            "error: no support for more than 32 registers\n");
    }
    return idx;
}

static inline uint32_t get_dest(const sjp::Json &inst) {
    return get_register(inst.Get("dest")->Get<std::string>().value());
}

static inline std::array<uint32_t, 2> get_srcs(const sjp::Json &inst) {
    auto args = inst.Get("args").value();
    assert(args.Size() == 2 && "err: expected only 2 args");

    std::array<uint32_t, 2> srcs;
    for (auto i : std::views::iota(0ul, 2ul)) {
        srcs[i] = get_register(args.Get(i)->Get<std::string>().value());
    }
    return srcs;
}

static inline std::vector<reg_t> get_args(const sjp::Json &inst) {
    assert(inst.Get("args").has_value() && "err: expected more than 0 args");

    auto args = inst.Get("args").value();
    std::vector<reg_t> regs(args.Size());

    for (auto i : std::views::iota(0ul, args.Size())) {
        regs[i] = get_register(args.Get(i)->Get<std::string>().value());
    }

    return regs;
}

void parse_const(const sjp::Json &inst, std::vector<uint32_t> &code) {
    /* TODO:
     * Use orr(immed) to encode immeds that can't be encoded using movz/movn
     */
    check_type_int(inst);

    int64_t imm =
        static_cast<int64_t>(inst.Get("value")->Get<double>().value());

    uint32_t dest = get_dest(inst);
    if (imm < 0) {
        assert(!((imm | ONES_16) ^ ONES_32) &&
               "err: only support for 16-bit imm\n");
        // imm = ~imm;
        emit_movn(dest, static_cast<uint16_t>(imm), 0, code);
    } else {
        assert(!(imm >> 16) && "err: only support for 16-bit imm\n");
        emit_movz(dest, static_cast<uint16_t>(imm), 0, code);
    }
}

void parse_add(const sjp::Json &inst, std::vector<uint32_t> &code) {
    check_type_int(inst);
    auto srcs = get_srcs(inst);
    uint32_t dest_reg = get_dest(inst);
    emit_add_sr(dest_reg, srcs[0], srcs[1], code);
}

void parse_sub(const sjp::Json &inst, std::vector<uint32_t> &code) {
    check_type_int(inst);
    auto srcs = get_srcs(inst);
    uint32_t dest_reg = get_dest(inst);
    emit_sub_sr(dest_reg, srcs[0], srcs[1], code);
}

void parse_mul(const sjp::Json &inst, std::vector<uint32_t> &code) {
    check_type_int(inst);
    auto srcs = get_srcs(inst);
    uint32_t dest_reg = get_dest(inst);
    emit_mul(dest_reg, srcs[0], srcs[1], code);
}

void parse_div(const sjp::Json &inst, std::vector<uint32_t> &code) {
    check_type_int(inst);
    auto srcs = get_srcs(inst);
    uint32_t dest_reg = get_dest(inst);
    emit_sdiv(dest_reg, srcs[0], srcs[1], code);
}

void parse_ret(const sjp::Json &inst, std::vector<uint32_t> &code) {
    if (inst.Get("args").has_value()) {
        auto args = inst.Get("args").value();
        assert(args.Size() < 2 && "err: more than one return value");

        if (args.Size() == 1) {
            uint32_t reg =
                get_register(args.Get(0)->Get<std::string>().value());
            emit_orr_sr(0ul, reg, code);
        }
    }

    emit_ret(code);
}

void print_int(size_t, ...);

void parse_print(const sjp::Json &inst, std::vector<uint32_t> &code) {
    auto args = get_args(inst);

    if (args.size() > 7) {
        throw std::runtime_error(
            "err: print supports only 7 args at this time\n");
    }

    std::map<reg_t, reg_t> pos;
    for (auto i : std::views::iota(0u, args.size())) {
        if (args[i] != i + 1) {
            // args[i] not in reg i + 1
            // record reg loc of  i + 1
            pos[i + 1] = args[i];
        }
    }

    for(auto [k, v] : pos) {
        // store reg k at loc_k
        emit_str_imm(SP, -8, k, ls_mode::pre, code);
    }

    for(auto [k, v]: pos) {
        // store reg v at loc_v
        emit_str_imm(SP, -8, v, ls_mode::pre, code);
    }

    int offset = static_cast<int>((pos.size() - 1) * 8);
    for(auto [k, v]: pos) {
        // load reg k from loc_v
        emit_ldr_imm(SP, offset, k, ls_mode::offset, code);
        offset -= 8;
    }

    // set num args in x0
    emit_movz(0, static_cast<uint16_t>(args.size()), 0, code);

    // prepare branch dest address
    // using r16 -> IP0 for holding branch instruction
    auto print_addr = reinterpret_cast<uint64_t>(print_int);
    reg_t breg = 16;
    emit_movz(breg, (print_addr & ONES_16), 0, code);
    print_addr >>= 16;
    emit_movk(breg, (print_addr & ONES_16), 1, code);
    print_addr >>= 16;
    emit_movk(breg, (print_addr & ONES_16), 2, code);
    print_addr >>= 16;
    emit_movk(breg, (print_addr & ONES_16), 3, code);

    emit_stp(SP, -16, 29, 30, ls_mode::pre, code);

    emit_mov_sp(29, SP, code);

    emit_blr(breg, code);

    emit_ldp(SP, 16, 29, 30, ls_mode::post, code);

    for(auto [k, v]: std::views::reverse(pos)) {
        // load reg v from loc_v
        emit_ldr_imm(SP, 8, v, ls_mode::post, code);
    }

    for(auto [k, v]: std::views::reverse(pos)) {
        // load reg k from loc_k
        emit_ldr_imm(SP, 8, k, ls_mode::post, code);
    }
}
