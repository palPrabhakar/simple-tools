#include <cstdlib>
#include <fstream>
#include <print>

#include "json.hpp"
#include "parser.hpp"

void interpret_program(sjp::Json &, int, char **);

int main(int argc, char **argv) {
    if (argc < 2) {
        std::println("Usage: ./main <bril.json> <...input_args>");
        std::exit(1);
    }

    std::ifstream input_stream(argv[1]);
    sjp::Parser parser(input_stream);
    sjp::Json json = parser.Parse();

    interpret_program(json, argc - 2, argv + 2);
}
