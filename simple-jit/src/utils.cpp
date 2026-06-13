#include <cstdarg>
#include <iostream>
#include <ranges>

void print_int(size_t count, ...) {
    va_list args;
    va_start(args, count);

    for(auto _ : std::views::iota(0ul, count - 1)) {
        std::cout<<va_arg(args, int)<<" ";
    }
    std::cout<<va_arg(args, int)<<"\n";

    va_end(args);
}
