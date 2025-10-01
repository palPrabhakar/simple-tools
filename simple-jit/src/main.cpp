#include <cassert>
#include <cstdio>
#include <cstring>
#include <dlfcn.h>
#include <fstream>
#include <iostream>
#include <ranges>
#include <sys/mman.h>
#include <vector>

#include "json.hpp"
#include "parser.hpp"

#include "parser.h"

int simple_add(int x, int y);

constexpr size_t SIZE = 1024;

constexpr uint64_t MASK_ALL_1 = 0xFFFFFFFFFFFFFFFF;
constexpr uint64_t MASK_4095 = 0xFFF; // lowest 12-bits are 1

typedef long (*jit_f)(void);
typedef int (*jit_i)(void);
typedef void (*jit_p)(void);
typedef int (*printf_ptr)(const char *restrict, ...);

// constexpr size_t REG_SIZE = 32;

void do_print(void) { printf("Hello World\n"); }

const char *hello_str = "Hello World\n";

void *alloc_executable_memory(size_t size, void *addr) {
    void *ptr = mmap(addr, size, PROT_READ | PROT_WRITE | PROT_EXEC,
                     MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    if (ptr == nullptr) {
        perror("mmap");
        return nullptr;
    }

    return ptr;
}

std::vector<uint32_t> get_code_aarch64(sjp::Json &jfunc) {
    std::vector<uint32_t> code;
    auto instrs = jfunc.Get("instrs").value();
    for (size_t i = 0; i < instrs.Size(); ++i) {
        auto instr = instrs.Get(i).value();
        auto opcode = instr.Get("op")->Get<std::string>();
        if (opcode == "const") {
            if (instr.Get("type")->Get<std::string>() == "int") {
                int val =
                    static_cast<int>(instr.Get("value")->Get<double>().value());

                if (val < 0) {
                    std::cerr << "No support for encoding negative numbers at "
                                 "this time!\n";
                    abort();
                }

                std::string dest =
                    instr.Get("dest")->Get<std::string>().value();
                auto regIdx = static_cast<size_t>(
                    std::stoi(dest.substr(1, dest.size() - 1)));
                uint32_t base_code = 0xd2800000;

                if (regIdx < REG_SIZE) {
                    base_code |= regIdx;
                    base_code |= (uint32_t)(val << 5);
                } else {
                    std::cerr << "No support for more than 32 registers\n";
                    abort();
                }
                code.push_back(base_code);
            }
        } else if (opcode == "add") {
            if (instr.Get("type")->Get<std::string>() == "int") {
                uint32_t base_code = 0x8b000000;

                auto args = instr.Get("args").value();
                for (auto i : std::views::iota(0ul, args.Size())) {
                    auto arg = args.Get(i)->Get<std::string>().value();
                    auto regIdx = static_cast<size_t>(
                        std::stoi(arg.substr(1, arg.size() - 1)));
                    if (regIdx < REG_SIZE) {
                        base_code |= (regIdx << (5 + i * 11));
                    } else {
                        std::cerr << "No support for more than 32 registers\n";
                        abort();
                    }
                }

                std::string dest =
                    instr.Get("dest")->Get<std::string>().value();
                auto regIdx = static_cast<size_t>(
                    std::stoi(dest.substr(1, dest.size() - 1)));
                if (regIdx < REG_SIZE) {
                    base_code |= regIdx;
                } else {
                    std::cerr << "No support for more than 32 registers\n";
                    abort();
                }
                code.push_back(base_code);
            }
        } else if (opcode == "return") {
            // assume the return address is in w30 reg!
            uint32_t base_code = 0xd65f03c0;

            auto args = instr.Get("args").value();
            assert(args.Size() == 1);
            auto arg = args.Get(0)->Get<std::string>().value();
            auto regIdx =
                static_cast<size_t>(std::stoi(arg.substr(1, arg.size() - 1)));
            if (regIdx < REG_SIZE && regIdx != 0) {
                // move the result to w0 register
                uint32_t base_code = 0xaa0003e0;
                base_code |= regIdx << 16;
                code.push_back(base_code);
            } else {
                std::cerr << "No support for more than 32 registers\n";
                abort();
            }
            code.push_back(base_code);
        } else {
            std::cerr << "Panik: Invalid Opcode\n";
            abort();
        }
    }
    return code;
}

void jit_bril() {
    std::ifstream file(
        "/home/pal/workspace/simple-tools/simple-jit/test/add.json");
    sjp::Parser parser(file);
    auto json = parser.Parse();
    auto jf = json.Get("functions")->Get(0).value();

    auto code = get_code(jf);

    // auto code = get_code_aarch64(jf);

    void *m = alloc_executable_memory(SIZE, nullptr);
    memcpy(m, code.data(), code.size() * sizeof(code.data()));

    std::cout << std::hex << "PC: " << reinterpret_cast<uint64_t>(m)
              << std::endl;

    jit_f f = (jit_f)m;
    long result = f();

    std::cout << "Result: " << result << std::endl;
}

void jit_printf() {
    void *libc_handle;
    void *printf_handle;

    libc_handle = dlopen("/usr/bin/lic.so.6", RTLD_NOW | RTLD_GLOBAL);
    if (!libc_handle) {
        std::cerr << "Error loading libc: " << dlerror() << std::endl;
        return;
    }

    printf_handle = dlsym(libc_handle, "printf");

    std::vector<uint32_t> code;
    void *m = alloc_executable_memory(
        SIZE, reinterpret_cast<void *>(reinterpret_cast<uint64_t>(jit_printf) -
                                       2 * 4096));

    code.push_back(0xA9BF7BFD); // stp x29, x30, [sp, #-16]!
    code.push_back(0x910003FD); // mov x29, sp
                                //
    std::cout << std::hex << "PC: " << reinterpret_cast<uint64_t>(m)
              << std::endl;
    std::cout << "printf: " << reinterpret_cast<uint64_t>(printf_handle)
              << std::endl;
    std::cout << "hello_str: " << reinterpret_cast<uint64_t>(&hello_str)
              << std::endl;

    auto addr_paligned =
        reinterpret_cast<uint64_t>(&hello_str) & (MASK_ALL_1 << 12);
    uint32_t offset = reinterpret_cast<uint64_t>(&hello_str) & MASK_4095;

    auto pc_paligned =
        (reinterpret_cast<uint64_t>(m) + code.size() * sizeof(uint32_t)) &
        (MASK_ALL_1 << 12);
    std::cout << "pc_paligned: " << pc_paligned << std::endl;

    int64_t diff =
        static_cast<int64_t>(addr_paligned) - static_cast<int64_t>(pc_paligned);
    if (diff < 0) {
        std::cout << "NEGATIVE OFFSET STR" << std::endl;
    }
    std::cout << "diff(str_pal - pc_pal): " << diff << std::endl;

    int64_t pc_rel_off =
        static_cast<int64_t>(addr_paligned) - static_cast<int64_t>(pc_paligned);
    int64_t sign = (pc_rel_off >> 63) & 1;
    int64_t rem = (pc_rel_off >> 12) && (MASK_ALL_1 >> 12);
    int64_t immlo = (rem & 3) << 28;
    int64_t immhi = (sign << 18) | (immlo >> 12);

    uint32_t base_adrp = 0x90000000;
    base_adrp = base_adrp + (unsigned int)immlo + (unsigned int)immhi;

    std::cout << "base_adrp: " << base_adrp << std::endl;
    code.push_back(base_adrp);

    uint32_t base_add = 0x91000000;
    base_add = base_add + (offset << 10);
    std::cout << "base_add: " << base_add << std::endl;
    code.push_back(base_add);

    uint32_t base_bl = 0x94000000;
    int32_t printf_off = static_cast<int32_t>(
        reinterpret_cast<int64_t>(printf_handle) -
        reinterpret_cast<int64_t>(m) - 2 * sizeof(uint32_t));
    int32_t ss = (printf_off >> 31) & 1;
    printf_off = (printf_off & 0x03FFFFFF) | (ss << 24);
    base_bl |= (uint32_t)printf_off;
    std::cout << "base_bl: " << base_bl << std::endl;
    code.push_back(base_bl);

    code.push_back(0xA8C17BFD);
    code.push_back(0xD65F03C0);

    memcpy(m, code.data(), code.size() * sizeof(code.data()));

    jit_p p = (jit_p)m;
    std::cout << "Calling printf: " << std::endl;
    p();

    dlclose(libc_handle);
}

void jit_simple_add() {
    std::vector<uint32_t> code;
    void *m = alloc_executable_memory(
        SIZE, reinterpret_cast<void *>(
                  reinterpret_cast<uint64_t>(jit_simple_add) - 2 * 4096));
    code.push_back(0xA9BF7BFD);
    code.push_back(0x910003FD);

    code.push_back(0xD2800280);
    code.push_back(0xD28002C1);

    uint32_t base_bl = 0x94000000;
    int64_t pc = reinterpret_cast<int64_t>(m) +
                 static_cast<int64_t>(code.size() * sizeof(uint32_t));
    int32_t off =
        static_cast<int32_t>(reinterpret_cast<int64_t>(simple_add) - pc);

    std::cout << std::hex << "PC: " << pc << std::endl;
    std::cout << "simple_add: " << reinterpret_cast<int64_t>(simple_add)
              << std::endl;
    std::cout << "offset: " << off << std::endl;

    if (off < 0) {
        std::cout << "NEGATIVE OFFSET" << std::endl;
        return;
    }

    off = (off >> 2);
    std::cout << "Offset: " << off << std::endl;
    base_bl |= (uint32_t)off;
    std::cout << "base_bl: " << base_bl << std::endl;
    code.push_back(base_bl);

    code.push_back(0xA8C17BFD);
    code.push_back(0xD65F03C0);

    memcpy(m, code.data(), code.size() * sizeof(code.data()));

    jit_i p = (jit_i)m;
    std::cout << "Calling jit func" << std::endl;
    int res = p();
    std::cout << "Result: " << res << std::endl;
}

int main() {
    std::cout << "Simple JIT" << std::endl;

    jit_bril();

    // jit_printf();

    // jit_simple_add();

    return 0;
}

int simple_add(int x, int y) { return x + y; }
