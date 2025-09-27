#include <cstdlib>
#include <iostream>
#include <print>
#include <sstream>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <unordered_set>
#include <vector>

std::unordered_set<std::string> inbuilts = {"exit"};

int main(int argc, char **argv) {
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

        // check for inbuilts
        if (cmds.front() == "exit") {
            exit(0);
        } else {
            std::vector<char *> args;
            for (auto &cmd : cmds) {
                args.push_back(cmd.data());
            }

            int wstatus;
            pid_t pid, w;

            pid = fork();

            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            }

            if (pid == 0) {
                // child process
                execvp(args[0], args.data());
                perror("execvpe");
                exit(EXIT_FAILURE);
            } else {
                // parent
                do {
                    w = waitpid(pid, &wstatus, WCONTINUED | WUNTRACED);
                    if (w == -1) {
                        perror("waitpid");
                        exit(EXIT_FAILURE);
                    }

                    if (WIFEXITED(wstatus)) {
                        std::print("exited status={}\n", WEXITSTATUS(wstatus));
                    } else if (WIFSIGNALED(wstatus)) {
                        std::print("killed by signal {}\n", WTERMSIG(wstatus));
                    } else if (WIFSTOPPED(wstatus)) {
                        std::print("stopped by signal {}\n", WSTOPSIG(wstatus));
                    } else if (WIFCONTINUED(wstatus)) {
                        std::print("continued\n");
                    }
                } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
            }
        }
    }
}
