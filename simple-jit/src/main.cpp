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

    if (ptr == MAP_FAILED) {
        std::cout << "eror" << std::endl;
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    return ptr;
}

void jit_bril() {
    std::ifstream file(
        "/home/pal/workspace/simple-tools/simple-jit/test/add.json");
    sjp::Parser parser(file);
    auto json = parser.Parse();
    auto jf = json.Get("functions")->Get(0).value();

    auto code = get_code(jf);

    void *m = alloc_executable_memory(SIZE, nullptr);
    memcpy(m, code.data(), code.size() * sizeof(code.data()));

    jit_f f = (jit_f)m;
    long result = f();

    std::cout << std::dec << "result: " << result << std::endl;
}

void jit_simple_add() {
    std::vector<uint32_t> code;

    uint64_t addr =
        (reinterpret_cast<uint64_t>(jit_simple_add) + 130 * 1024 * 1024) &
        (MASK_ALL_1 << 12);
    void *m = alloc_executable_memory(SIZE, nullptr);

    code.push_back(0xA9BF7BFD);
    code.push_back(0x910003FD);

    code.push_back(0xD2800280); // x0
    code.push_back(0xD28002C1); // x1

    int64_t pc = reinterpret_cast<int64_t>(m) +
                 static_cast<int64_t>(code.size() * sizeof(uint32_t));
    int64_t off = reinterpret_cast<int64_t>(simple_add) - pc;

    std::cout << std::hex << "PC: " << pc << std::endl;
    std::cout << "jit_simple_add: " << reinterpret_cast<int64_t>(jit_simple_add)
              << std::endl;
    std::cout << "simple_add: " << reinterpret_cast<int64_t>(simple_add)
              << std::endl;
    std::cout << "simple_add - jit_simple_add: "
              << reinterpret_cast<uint64_t>(simple_add) -
                     reinterpret_cast<uint64_t>(jit_simple_add)
              << std::endl;
    std::cout << "offset: " << off << std::endl;

    if (off < -0x2000000 || off > 0x1FFFFFF) {
        std::cerr << "Encoding using adrp, add, blr\n";
        uint32_t base_adrp = 0x90000000;
        // need to use adrp and
        pc &= (MASK_ALL_1 << 12);
        int64_t loc = static_cast<int64_t>(
            reinterpret_cast<uint64_t>(simple_add) & (MASK_ALL_1 << 12));
        int64_t off = loc - pc;
        if (off < 0) {
            std::cout << "Negative offset\n";
        }
        off >>= 12;
        if (off < -1048576 || off > 1048575) {
            // use movz/movk to build the address in register
            uint64_t sa = reinterpret_cast<uint64_t>(simple_add);
            uint32_t base_movz = 0xd2800000;
            base_movz |= (sa & 0xFFFF) << 5;
            base_movz |= 0x2;
            code.push_back(base_movz);
            sa >>= 16;

            uint32_t base_movk2 = 0xf2a00000; // lsl-2
            base_movk2 |= (sa & 0xFFFF) << 5;
            base_movk2 |= 0x2;
            code.push_back(base_movk2);
            sa >>= 16;

            uint32_t base_movk3 = 0xf2c00000; // lsl-3
            base_movk3 |= (sa & 0xFFFF) << 5;
            base_movk3 |= 0x2;
            code.push_back(base_movk3);
            sa >>= 16;

            uint32_t base_movk4 = 0xf2e00000; // lsl-3
            base_movk4 |= (sa & 0xFFFF) << 5;
            base_movk4 |= 0x2;
            code.push_back(base_movk4);

        } else {
            off &= 0x1FFFFF;
            std::cout << "off: " << off << std::endl;
            base_adrp |= ((off & 0x3) << 29);
            std::cout << "base_adrp: " << base_adrp << std::endl;
            off >>= 2;
            base_adrp |= (off << 5);
            base_adrp |= 0x2;
            std::cout << "base_adrp: " << base_adrp << std::endl;
            code.push_back(base_adrp);

            uint32_t base_add = 0x91000000;
            uint64_t offset =
                reinterpret_cast<uint64_t>(simple_add) & MASK_4095;
            base_add |= (offset << 10);
            base_add |= (0x2 << 5);
            base_add |= 0x2; // dest
            std::cout << "base_add: " << base_add << std::endl;
            code.push_back(base_add);
        }

        uint32_t base_blr = 0xd63f0000;
        base_blr |= (0x2 << 5);
        std::cout << "base_blr: " << base_blr << std::endl;
        code.push_back(base_blr);
    } else {
        std::cerr << "Encoding using bl\n";
        uint32_t base_bl = 0x94000000;
        // can directly encode the immed offset in the bl instr
        if (off < 0) {
            std::cout << "Negative offset\n";
        }
        off = (off >> 2);
        std::cout << "Offset: " << off << std::endl;
        base_bl |= static_cast<uint32_t>(off & 0x3FFFFFF);
        std::cout << "base_bl: " << base_bl << std::endl;
        code.push_back(base_bl);
    }

    code.push_back(0xA8C17BFD);
    code.push_back(0xD65F03C0);

    memcpy(m, code.data(), code.size() * sizeof(code.data()));

    jit_i p = (jit_i)m;
    std::cout << "Calling jit func" << std::endl;
    int res = p();
    std::cout << std::dec << "Result: " << res << std::endl;
}

int main() {
    std::cout << "Simple JIT" << std::endl;

    // jit_bril();

    jit_simple_add();

    return 0;
}

int simple_add(int x, int y) {
    int res = x + y;
    printf("%d + %d = %d\n", x, y, res);
    return res;
}
