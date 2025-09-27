#include <cstdlib>
#include <iostream>
#include <print>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

using Arguments = std::vector<std::string>;
typedef void (*func)(Arguments);

void func_exit(Arguments);
void func_pwd(Arguments);
void func_cd(Arguments);
void func_external(Arguments);

std::unordered_map<std::string, func> builtins = {{"exit", func_exit},
                                                  {"pwd", func_pwd},
                                                  {"cd", func_cd}};

int main(void) {
    std::print("Simple Shell\n");

    // main loop
    while (true) {
        std::print(">>> ");
        std::string cmdline;
        std::getline(std::cin, cmdline);
        std::stringstream cmdstream(cmdline);
        std::vector<std::string> cmds;
        std::string cmd;
        while (cmdstream >> cmd) {
            cmds.push_back(cmd);
        }

        builtins.contains(cmds.front())
            ? builtins.at(cmds.front())(std::move(cmds))
            : func_external(std::move(cmds));
    }

    return 0;
}
