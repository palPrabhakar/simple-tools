#include <iostream>
#include <format>
#include <vector>

#include "../src/emitter.h"

#define EXPECT(name, a, b)                                                     \
    if (a == b) {                                                              \
        std::cout << std::format(                                              \
            "\033[1;32mTest: {} - Pass: {} == {}\033[0m\n", name, a, b);       \
    } else {                                                                   \
        std::cout << std::format(                                              \
            "\033[1;31mTest: {} - Fail: {} != {}\033[0m\n", name, a, b);       \
    }

#define TEST_MOV(test, a, b)                                                   \
    test;                                                                      \
    EXPECT(#test, a, b)

void test_movz() {
    std::vector<uint32_t> code;
    TEST_MOV(emit_movz(0, 1, code), code.back(), 0xd2800020);
    TEST_MOV(emit_movz(0, 65536, code), code.back(), 0xd2a00020);
    TEST_MOV(emit_movz(1, 32769, code), code.back(), 0xd2900021);
    TEST_MOV(emit_movz(2, 65535, code), code.back(), 0xd29fffe2);
    TEST_MOV(emit_movz(2, 131072, code), code.back(), 0xd2a00042);
    TEST_MOV(emit_movz(4, 4771708665856, code), code.back(), 0xd2c08ae4);
    TEST_MOV(emit_movz(5, 281756451687366656, code), code.back(), 0xd2e07d25);
}

void test_movn() {
    /*
     * TODO:
     *    0:	b26fbbe2 	mov	x2, #0xfffffffffffe0000    	//
     * #-131072
     */
    std::vector<uint32_t> code;
    TEST_MOV(emit_movn(0, -1, code), code.back(), 0x92800000);
    TEST_MOV(emit_movn(0, -65537, code), code.back(), 0x92a00020);
    TEST_MOV(emit_movn(1, -32769, code), code.back(), 0x92900001);
    TEST_MOV(emit_movn(2, -65536, code), code.back(), 0x929fffe2);
    TEST_MOV(emit_movn(2, -65601537, code), code.back(), 0x92a07d22);
    TEST_MOV(emit_movn(4, -4299262263297, code), code.back(), 0x92c07d24);
    TEST_MOV(emit_movn(5, -281756451687366657, code), code.back(), 0x92e07d25);
}

int main() {
    std::cout<<"Emitter Test\n";
    test_movz();
    test_movn();
    return 0;
}
