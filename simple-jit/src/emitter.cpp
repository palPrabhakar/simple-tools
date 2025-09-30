#include "emitter.h"
#include "unordered_map"
#include <stdexcept>
#include <unordered_map>

#define EMIT_0(op)                                                             \
    void emit_##op([[maybe_unused]] const sjp::Json &inst,                     \
                   [[maybe_unused]] std::vector<uint32_t> &code) {             \
        throw std::runtime_error("opcode: " #op " not implemented\n");         \
    }

#define EMIT_1(op)                                                             \
    void emit_##op(const sjp::Json &inst, std::vector<uint32_t> &code);

#define X(op, bit) EMIT_##bit(op)
OPCODE_LIST
#undef X

#undef EMIT_0
#undef EMIT_1

typedef void (*Emitter)(const sjp::Json &, std::vector<uint32_t> &code);

#define TEST_LIST X(add, 0)

const std::unordered_map<std::string, Emitter> emitter_list = {
#define X(op, bit) {#op, emit_##op},
    OPCODE_LIST
#undef X
};

std::vector<uint32_t> get_code([[maybe_unused]] const sjp::Json &f) {
    return {};
}
