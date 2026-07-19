#include <functional>
#include <ranges>
#include <stack>
#include <stdexcept>
#include <unordered_map>
#include <variant>

#include "json.hpp"
#include "opcode.hpp"

struct Frame;

using Value = std::variant<bool, float, int>;

struct Frame {
    std::unordered_map<std::string, Value> variables;
    std::unordered_map<std::string, size_t> labels;
    std::string name;
    size_t ip;
    size_t end;
};

struct Context {
    /*
     * Don't need to store a reference here
     * Every Json object is internally a shared_ptr.
     * So making copies only increases the reference
     * count of the original object
     */
    std::unordered_map<std::string, const sjp::Json> functions;
    std::stack<Frame> frames;
    Value ret_val;
};

#define INTERPRET_0(op)                                                        \
    void interpret_##op([[maybe_unused]] Context &ctx,                         \
                        [[maybe_unused]] const sjp::Json &inst) {              \
        throw std::runtime_error("error: opcode: " #op " not implemented\n");  \
    }

#define INTERPRET_1(op)                                                        \
    void interpret_##op(Context &ctx, const sjp::Json &inst);

#define X(op, bit) INTERPRET_##bit(op)
OPCODE_LIST
#undef X

typedef void (*Interpreter)(Context &, const sjp::Json &);

#define TEST_LIST X(add, 0)

const std::unordered_map<std::string, Interpreter> opcode_list = {
#define X(op, bit) {#op, interpret_##op},
    OPCODE_LIST
#undef X
};

// json helpers
#define get_dest(x) x.Get("dest")->Get<std::string>().value()
#define get_type(x) x.Get("type")->Get<std::string>().value()
#define value_bool(x) x.Get("value")->Get<bool>().value()
#define value_float(x) static_cast<float>(x.Get("value")->Get<double>().value())
#define value_int(x) static_cast<int>(x.Get("value")->Get<double>().value())
#define get_args(x, i) x.Get("args")->Get(i)->Get<std::string>().value()
#define args_size(x) x.Get("args")->Size()
#define has_opcode(x) x.Get("op").has_value()
#define has_label(x) x.Get("label").has_value()
#define labels_size(x) x.Get("labels")->Size()
#define get_labels(x, i) x.Get("labels")->Get(i)->Get<std::string>().value()

// frame helpers
#define get_arg_v(f, in, i) f.variables[get_args(in, i)]
#define set_dest(f, in, val) f.variables[get_dest(in)] = val

#define __frame(x) auto &frame = x.frames.top();
#define _frame __frame(ctx)
#define _inc_ip ++frame.ip

constexpr auto &_iota = std::views::iota;

void interpret_function(Context &ctx, std::string fn_name);

Context create_context(const sjp::Json &program) {
    Context ctx;

    auto functions = program.Get("functions");
    for (auto i : _iota(0u, functions->Size())) {
        auto f = functions->Get(i);
        auto name = f->Get("name")->Get<std::string>();
        ctx.functions.emplace(name.value(), f.value());
    }

    return ctx;
}

void setup_frame(Context &ctx, std::string fn_name, std::vector<Value> v_args) {
    const auto func = ctx.functions.at(fn_name);

    Frame frame;
    frame.name = fn_name;
    frame.ip = 0;
    frame.end = func.Get("instrs")->Size();

    const auto args = func.Get("args");

    if (args.has_value()) {
        if (v_args.size() != args->Size()) {
            throw std::runtime_error("args size mismatch\n");
        }

        for (auto i : _iota(0u, args->Size())) {
            auto arg = args->Get(i).value();
            auto name = arg.Get("name")->Get<std::string>().value();
            switch (get_type(arg)[0]) {
#define X(t, v)                                                                \
    case v:                                                                    \
        frame.variables.emplace(name, std::get<t>(v_args[i]));                 \
        break;
                TYPE_LIST
#undef X
            default:
                throw std::runtime_error("invalid type\n");
            }
        }
    }

    ctx.frames.push(frame);
}

void find_label(Context &ctx, std::string lbl_name) {
    _frame;

    const auto func = ctx.functions.at(frame.name);
    const auto instrs = func.Get("instrs").value();

    size_t i = frame.ip + 1;
    while (i < frame.end) {
        auto instr = instrs.Get(i).value();
        if (has_label(instr)) {
            auto lbl = instr.Get("label")->Get<std::string>().value();
            if (lbl == lbl_name) {
                frame.labels[lbl] = i;
                return;
            }
        }
    }

    throw std::runtime_error(std::format("label: {} not found\n", lbl_name));
}

void interpret_program(sjp::Json &program) {
    auto ctx = create_context(program);
    // TODO:
    // parse main args
    setup_frame(ctx, "main", {});
    interpret_function(ctx, "main");
}

void interpret_function(Context &ctx, std::string fn_name) {
    if (!ctx.functions.contains(fn_name)) {
        throw std::runtime_error(
            std::format("{} function not found\n", fn_name));
    }

    const auto func = ctx.functions.at(fn_name);
    const auto instrs = func.Get("instrs").value();

    auto &frame = ctx.frames.top();

    while (frame.ip < frame.end) {
        const auto instr = instrs.Get(frame.ip).value();

        if (has_opcode(instr)) {
            const auto opcode = instr.Get("op")->Get<std::string>().value();
            opcode_list.at(opcode)(ctx, instr);
        } else {
            assert(has_label(instr) && "invalid instruction\n");
            opcode_list.at("label")(ctx, instr);
        }
    }

    ctx.frames.pop();
}

Value get_value(const sjp::Json &instr) {
    switch (get_type(instr)[0]) {
#define X(t, v)                                                                \
    case v:                                                                    \
        return value_##t(instr);
        TYPE_LIST
#undef X
    default:
        throw std::runtime_error("invalid type\n");
    }
}

void interpret_const(Context &ctx, const sjp::Json &instr) {
    _frame;
    auto dest = get_dest(instr);
    set_dest(frame, instr, get_value(instr));
    _inc_ip;
}

template <template <typename> typename Op, typename T>
T operation(Context &ctx, const sjp::Json &instr) {
    _frame;
    Op<T> op;
    return op(std::get<T>(get_arg_v(frame, instr, 0)),
              std::get<T>(get_arg_v(frame, instr, 1)));
}

void interpret_add(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::plus, int>(ctx, instr)));
    _inc_ip;
}

void interpret_sub(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::minus, int>(ctx, instr)));
    _inc_ip;
}

void interpret_mul(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::multiplies, int>(ctx, instr)));
    _inc_ip;
}

void interpret_div(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::divides, int>(ctx, instr)));
    _inc_ip;
}

void interpret_eq(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::equal_to, int>(ctx, instr)));
    _inc_ip;
}

void interpret_lt(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::less, int>(ctx, instr)));
    _inc_ip;
}

void interpret_gt(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::greater, int>(ctx, instr)));
    _inc_ip;
}

void interpret_le(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::less_equal, int>(ctx, instr)));
    _inc_ip;
}

void interpret_ge(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::greater_equal, int>(ctx, instr)));
    _inc_ip;
}

void interpret_fadd(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::plus, float>(ctx, instr)));
    _inc_ip;
}

void interpret_fsub(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::minus, float>(ctx, instr)));
    _inc_ip;
}

void interpret_fmul(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::multiplies, float>(ctx, instr)));
    _inc_ip;
}

void interpret_fdiv(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::divides, float>(ctx, instr)));
    _inc_ip;
}

void interpret_feq(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::equal_to, float>(ctx, instr)));
    _inc_ip;
}

void interpret_flt(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::less, float>(ctx, instr)));
    _inc_ip;
}

void interpret_fgt(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::greater, float>(ctx, instr)));
    _inc_ip;
}

void interpret_fle(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::less_equal, float>(ctx, instr)));
    _inc_ip;
}

void interpret_fge(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, (operation<std::greater_equal, float>(ctx, instr)));
    _inc_ip;
}

void interpret_print(Context &ctx, const sjp::Json &instr) {
    _frame;
    auto size = args_size(instr);
    for (auto i : _iota(0u, size - 1)) {
        std::visit([](auto &&arg) { std::cout << arg; },
                   frame.variables[get_args(instr, i)]);
        std::cout << " ";
    }
    std::visit([](auto &&arg) { std::cout << arg; },
               frame.variables[get_args(instr, size - 1)]);
    std::cout << "\n";
    _inc_ip;
}

void interpret_ret(Context &ctx, const sjp::Json &instr) {
    _frame;
    if (instr.Get("args").has_value()) {
        assert(args_size(instr) == 1 && "more than one return value\n");
        ctx.ret_val = get_arg_v(frame, instr, 0);
    }
    frame.ip = frame.end;
}

void interpret_jmp(Context &ctx, const sjp::Json &instr) {
    _frame;
    assert(labels_size(instr) == 1 && "more than one labels in jmp instr\n");
    auto lbl = get_labels(instr, 0);

    if (!frame.labels.contains(lbl)) {
        find_label(ctx, lbl);
    }

    frame.ip = frame.labels[lbl] + 1;
}

void interpret_label(Context &ctx, const sjp::Json &instr) {
    _frame;
    auto lbl_name = instr.Get("label")->Get<std::string>().value();
    frame.labels[lbl_name] = frame.ip;
    _inc_ip;
}
