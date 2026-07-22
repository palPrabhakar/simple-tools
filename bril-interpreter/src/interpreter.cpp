#include <functional>
#include <ranges>
#include <stack>
#include <stdexcept>
#include <unordered_map>
#include <variant>

#include "json.hpp"
#include "opcode.hpp"

struct Frame;

using Value =
    std::variant<bool, char, float, int, bool *, char *, float *, int *>;

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

const std::unordered_map<std::string, Interpreter> opcode_list = {
#define X(op, bit) {#op, interpret_##op},
    OPCODE_LIST
#undef X
};

// json helpers
#define has_dest(x) x.Get("dest").has_value()
#define get_dest(x) x.Get("dest")->Get<std::string>().value()
#define is_ptr(x) (x.Get("type")->type == sjp::JsonType::jobject)
#define get_type(x) x.Get("type")->Get<std::string>().value()
#define get_ptr_type(x) x.Get("type")->Get("ptr")->Get<std::string>().value()
#define get_args(x, i) x.Get("args")->Get(i)->Get<std::string>().value()
#define args_size(x) x.Get("args")->Size()
#define has_opcode(x) x.Get("op").has_value()
#define has_label(x) x.Get("label").has_value()
#define labels_size(x) x.Get("labels")->Size()
#define get_labels(x, i) x.Get("labels")->Get(i)->Get<std::string>().value()
#define get_name(x) x.Get("name")->Get<std::string>().value();
#define value_char(x) x.Get("value")->Get<std::string>().value()[0]
#define value_bool(x) x.Get("value")->Get<bool>().value()
#define value_float(x) static_cast<float>(x.Get("value")->Get<double>().value())
#define value_int(x) static_cast<int>(x.Get("value")->Get<double>().value())

// frame helpers
#define get_arg_v(f, in, i) f.variables[get_args(in, i)]
#define set_dest(f, in, val) f.variables[get_dest(in)] = val

#define __frame(x) auto &frame = x.frames.top();
#define _frame __frame(ctx)
#define _inc_ip ++frame.ip

constexpr auto &_iota = std::views::iota;

void interpret_function(Context &ctx, std::string fn_name, Frame frame);

#define IMPLEMENT_BINOP(name, op, in_type, out_type)                           \
    void interpret_##name(Context &ctx, const sjp::Json &instr) {              \
        _frame;                                                                \
        set_dest(frame, instr,                                                 \
                 (operation<op, in_type, out_type>(ctx, instr)));              \
        _inc_ip;                                                               \
    }

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

std::vector<Value> get_main_args(Context &ctx, int argc, char **argv) {
    const auto func = ctx.functions.at("main");

    const auto args = func.Get("args");

    if (!args.has_value()) {
        return {};
    }

    if (static_cast<size_t>(argc) < args->Size()) {
        throw std::runtime_error(std::format(
            "main: argument size mismatch. expected: {} - found: {}",
            args->Size(), argc));
    }

    std::vector<Value> v_args;

    for (auto i : _iota(0u, args->Size())) {
        auto arg = args->Get(i).value();
        switch (get_type(arg)[0]) {
        case 'b': {
            if (std::string_view(argv[i]) == std::string_view("true") ||
                std::string_view(argv[i]) == std::string_view("1")) {
                v_args.emplace_back(true);
            } else if (std::string_view(argv[i]) == std::string_view("false") ||
                       std::string_view(argv[i]) == std::string_view("0")) {
                v_args.emplace_back(false);
            } else {
                throw std::runtime_error(std::format(
                    "invalid argument {}. Cannot covert to bool\n", argv[i]));
            }
        } break;
        case 'i': {
            if (std::string_view(argv[i]).find('.') != std::string::npos) {
                throw std::invalid_argument("float string not allowed");
            }
            v_args.emplace_back(std::stoi(argv[i]));
        } break;
        case 'f':
            v_args.emplace_back(std::stof(argv[i]));
            break;
        default:
            throw std::runtime_error("invalid type\n");
        }
    }

    return v_args;
}

Frame setup_frame(Context &ctx, std::string fn_name, std::vector<Value> v_args) {
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
            auto name = get_name(arg);

            auto is_ptr = is_ptr(arg);
            auto scond = is_ptr ? get_ptr_type(arg) : get_type(arg);
            switch (scond[0]) {
#define X(t, v, _)                                                             \
    case v: {                                                                  \
        if (is_ptr)                                                            \
            frame.variables.emplace(name, std::get<t *>(v_args[i]));           \
        else                                                                   \
            frame.variables.emplace(name, std::get<t>(v_args[i]));             \
        break;                                                                 \
    }
                TYPE_LIST
#undef X
            default:
                throw std::runtime_error("invalid type\n");
            }
        }
    }

    return frame;
}

void find_label(Context &ctx, std::string lbl_name) {
    _frame;

    const auto func = ctx.functions.at(frame.name);
    const auto instrs = func.Get("instrs").value();

    size_t i = frame.ip;
    while (++i < frame.end) {
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

void interpret_program(sjp::Json &program, int argc, char **argv) {
    auto ctx = create_context(program);
    auto frame = setup_frame(ctx, "main", get_main_args(ctx, argc, argv));
    interpret_function(ctx, "main", frame);
}

void interpret_function(Context &ctx, std::string fn_name, Frame fn_frame) {
    if (!ctx.functions.contains(fn_name)) {
        throw std::runtime_error(
            std::format("{} function not found\n", fn_name));
    }

    const auto func = ctx.functions.at(fn_name);
    const auto instrs = func.Get("instrs").value();

    ctx.frames.push(fn_frame);
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

Value get_json_value(const sjp::Json &instr) {
    switch (get_type(instr)[0]) {
#define X(t, v, i)                                                             \
    case v:                                                                    \
        return value_##t(instr);
        TYPE_LIST
#undef X
    default:
        throw std::runtime_error("invalid type\n");
    }
}

template <template <typename> typename Op, typename T, typename R = T>
R operation(Context &ctx, const sjp::Json &instr) {
    _frame;
    Op<T> op;
    return op(std::get<T>(get_arg_v(frame, instr, 0)),
              std::get<T>(get_arg_v(frame, instr, 1)));
}

void interpret_const(Context &ctx, const sjp::Json &instr) {
    _frame;
    auto dest = get_dest(instr);
    set_dest(frame, instr, get_json_value(instr));
    _inc_ip;
}

// int
IMPLEMENT_BINOP(add, std::plus, int, int)
IMPLEMENT_BINOP(sub, std::minus, int, int)
IMPLEMENT_BINOP(mul, std::multiplies, int, int)
IMPLEMENT_BINOP(div, std::divides, int, int)
IMPLEMENT_BINOP(eq, std::equal_to, int, bool)
IMPLEMENT_BINOP(lt, std::less, int, bool)
IMPLEMENT_BINOP(gt, std::greater, int, bool)
IMPLEMENT_BINOP(le, std::less_equal, int, bool)
IMPLEMENT_BINOP(ge, std::greater_equal, int, bool)

// float
IMPLEMENT_BINOP(fadd, std::plus, float, float)
IMPLEMENT_BINOP(fsub, std::minus, float, float)
IMPLEMENT_BINOP(fmul, std::multiplies, float, float)
IMPLEMENT_BINOP(fdiv, std::divides, float, float)
IMPLEMENT_BINOP(feq, std::equal_to, float, bool)
IMPLEMENT_BINOP(flt, std::less, float, bool)
IMPLEMENT_BINOP(fgt, std::greater, float, bool)
IMPLEMENT_BINOP(fle, std::less_equal, float, bool)
IMPLEMENT_BINOP(fge, std::greater_equal, float, bool)

// bool
IMPLEMENT_BINOP(and, std::logical_and, bool, bool)
IMPLEMENT_BINOP(or, std::logical_or, bool, bool)

// char
IMPLEMENT_BINOP(ceq, std::equal_to, char, bool)
IMPLEMENT_BINOP(clt, std::less, char, bool)
IMPLEMENT_BINOP(cgt, std::greater, char, bool)
IMPLEMENT_BINOP(cle, std::less_equal, char, bool)
IMPLEMENT_BINOP(cge, std::greater_equal, char, bool)

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
        ctx.ret_val = get_arg_v(frame, instr, 0);
    }
    frame.ip = frame.end;
}

void jmp_to_label(Context &ctx, const std::string lbl) {
    _frame;
    if (!frame.labels.contains(lbl)) {
        find_label(ctx, lbl);
    }
    frame.ip = frame.labels[lbl] + 1;
}

void interpret_jmp(Context &ctx, const sjp::Json &instr) {
    auto lbl = get_labels(instr, 0);
    jmp_to_label(ctx, lbl);
}

void interpret_br(Context &ctx, const sjp::Json &instr) {
    _frame;
    auto cond = std::get<bool>(get_arg_v(frame, instr, 0));
    std::string lbl = cond ? get_labels(instr, 0) : get_labels(instr, 1);
    jmp_to_label(ctx, lbl);
}

void interpret_label(Context &ctx, const sjp::Json &instr) {
    _frame;
    auto lbl_name = instr.Get("label")->Get<std::string>().value();
    frame.labels[lbl_name] = frame.ip;
    _inc_ip;
}

void interpret_id(Context &ctx, const sjp::Json &instr) {
    _frame;
    set_dest(frame, instr, get_arg_v(frame, instr, 0));
    _inc_ip;
}

void interpret_call(Context &ctx, const sjp::Json &instr) {
    _frame;

    std::vector<Value> v_args;
    for (auto i : _iota(0u, args_size(instr))) {
        v_args.emplace_back(get_arg_v(frame, instr, i));
    }

    auto fn_name = instr.Get("funcs")->Get(0)->Get<std::string>().value();
    auto new_frame = setup_frame(ctx, fn_name, std::move(v_args));
    interpret_function(ctx, fn_name, new_frame);

    if (has_dest(instr)) {
        set_dest(frame, instr, ctx.ret_val);
    }
    _inc_ip;
}

void interpret_not(Context &ctx, const sjp::Json &instr) {
    _frame;
    auto value = std::get<bool>(get_arg_v(frame, instr, 0));
    set_dest(frame, instr, !value);
    _inc_ip;
}

void interpret_alloc(Context &ctx, const sjp::Json &instr) {
    _frame;
    auto size = static_cast<size_t>(std::get<int>(get_arg_v(frame, instr, 0)));
    switch (get_ptr_type(instr)[0]) {
#define X(t, v, i)                                                             \
    case v:                                                                    \
        set_dest(frame, instr, new t[size]);                                   \
        break;
        TYPE_LIST
#undef X
    }
    _inc_ip;
}

void interpret_free(Context &ctx, const sjp::Json &instr) {
    _frame;
    auto ptr = get_arg_v(frame, instr, 0);
    switch (ptr.index()) {
#define X(t, v, i)                                                             \
    case i:                                                                    \
        delete std::get<t *>(ptr);                                             \
        break;
        TYPE_LIST
#undef X
    default:
        throw std::runtime_error("invalid ptr type\n");
    }
    _inc_ip;
}

void interpret_store(Context &ctx, const sjp::Json &instr) {
    _frame;

    auto ptr = get_arg_v(frame, instr, 0);
    auto val = get_arg_v(frame, instr, 1);

    if (val.index() + 4 != ptr.index()) {
        throw std::runtime_error("err ptr and value type don't match\n");
    }

    switch (ptr.index()) {
#define X(t, v, i)                                                             \
    case i:                                                                    \
        *(std::get<t *>(ptr)) = std::get<t>(val);                              \
        break;
        TYPE_LIST
#undef X
    default:
        throw std::runtime_error("invalid type\n");
    }
    _inc_ip;
}

void interpret_load(Context &ctx, const sjp::Json &instr) {
    _frame;
    auto ptr = get_arg_v(frame, instr, 0);
    switch (get_type(instr)[0]) {
#define X(t, v, i)                                                             \
    case v:                                                                    \
        set_dest(frame, instr, *(std::get<t *>(ptr)));                         \
        break;
        TYPE_LIST
#undef X
    }
    _inc_ip;
}

void interpret_ptradd(Context &ctx, const sjp::Json &instr) {
    _frame;
    auto val = std::get<int>(get_arg_v(frame, instr, 1));
    auto ptr = get_arg_v(frame, instr, 0);
    switch (get_ptr_type(instr)[0]) {
#define X(t, v, i)                                                             \
    case v: {                                                                  \
        assert(ptr.index() == i && "dest and src ptr type don't match\n");     \
        set_dest(frame, instr, std::get<t *>(ptr) + val);                      \
        break;                                                                 \
    }
        TYPE_LIST
#undef X
    }
    _inc_ip;
}

void interpret_nop(Context &ctx, [[maybe_unused]] const sjp::Json &json) {
    _frame;
    _inc_ip;
}
