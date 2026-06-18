#include <format>
#include <iostream>
#include <vector>

#include "../src/emitter.h"

#define EXPECT(name, a, b)                                                     \
    if (a == b) {                                                              \
        std::cout << std::hex                                                  \
                  << std::format(                                              \
                         "\033[1;32mTest: {} - Pass: {:0x} == {:0x}\033[0m\n", \
                         name, a, b);                                          \
    } else {                                                                   \
        std::cout << std::hex                                                  \
                  << std::format(                                              \
                         "\033[1;31mTest: {} - Fail: {:0x} != {:0x}\033[0m\n", \
                         name, a, b);                                          \
    }

#define TEST(test, a, b)                                                       \
    test;                                                                      \
    EXPECT(#test, a, b)

void test_movz() {
    std::vector<uint32_t> code;
#define movz emit_movz
    TEST(movz(0, 0x0001, 0, code), code.back(), 0xd2800020);
    TEST(movz(0, 0x0001, 1, code), code.back(), 0xd2a00020);
    TEST(movz(1, 0x8001, 0, code), code.back(), 0xd2900021);
    TEST(movz(2, 0xFFFF, 0, code), code.back(), 0xd29fffe2);
    TEST(movz(2, 0x0002, 1, code), code.back(), 0xd2a00042);
    TEST(movz(4, 0x0457, 2, code), code.back(), 0xd2c08ae4);
    TEST(movz(5, 0x03E9, 3, code), code.back(), 0xd2e07d25);
#undef movz
}

void test_movn() {
    /*
     * TODO:
     *    0:	b26fbbe2 	mov	x2, #0xfffffffffffe0000    	//
     * #-131072
     */
    std::vector<uint32_t> code;
#define movn emit_movn
    TEST(movn(0, 0xFFFF, 0, code), code.back(), 0x92800000);
    TEST(movn(0, 0xFFFE, 1, code), code.back(), 0x92a00020);
    TEST(movn(1, 0x7FFF, 0, code), code.back(), 0x92900001);
    TEST(movn(2, 0x0000, 0, code), code.back(), 0x929fffe2);
    TEST(movn(2, 0xFC16, 1, code), code.back(), 0x92a07d22);
    TEST(movn(4, 0xFC16, 2, code), code.back(), 0x92c07d24);
    TEST(movn(5, 0xFC16, 3, code), code.back(), 0x92e07d25);
#undef movn
}

void test_add_sr() {
    std::vector<uint32_t> code;
#define add emit_add_sr
    TEST(add(0, 1, 0, code), code.back(), 0x8b000020)
#undef add
}

void test_sub_sr() {
    std::vector<uint32_t> code;
#define sub emit_sub_sr
    TEST(sub(0, 1, 0, code), code.back(), 0xcb000020)
#undef sub
}

void test_ldr_imm() {
    std::vector<uint32_t> code;
#define ldr emit_ldr_imm
    TEST(ldr(SP, 32, 0, ls_mode::offset, code), code.back(), 0xf94013e0);
    TEST(ldr(SP, -32, 3, ls_mode::pre, code), code.back(), 0xf85e0fe3);
    TEST(ldr(SP, 48, 1, ls_mode::pre, code), code.back(), 0xf8430fe1);
    TEST(ldr(SP, 32, 4, ls_mode::post, code), code.back(), 0xf84207e4);
    TEST(ldr(SP, -64, 2, ls_mode::post, code), code.back(), 0xf85c07e2);
#undef ldr
}

void test_str_imm() {
    std::vector<uint32_t> code;
#define str emit_str_imm
    TEST(str(SP, 40, 0, ls_mode::offset, code), code.back(), 0xf90017e0);
    TEST(str(SP, -32, 3, ls_mode::pre, code), code.back(), 0xf81e0fe3);
    TEST(str(SP, 48, 1, ls_mode::pre, code), code.back(), 0xf8030fe1);
    TEST(str(SP, 32, 4, ls_mode::post, code), code.back(), 0xf80207e4);
    TEST(str(SP, -64, 2, ls_mode::post, code), code.back(), 0xf81c07e2);
#undef str
}

int main() {
    std::cout << "Emitter Test\n";
    test_movz();
    test_movn();
    test_add_sr();
    test_sub_sr();
    test_ldr_imm();
    test_str_imm();
    return 0;
}
