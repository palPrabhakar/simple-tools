#include <errno.h>
#include <print>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

void func_exit([[maybe_unused]] std::vector<std::string> args) { exit(0); }

void func_pwd(std::vector<std::string> args) {
    // pwd
    if (args.size() > 1) {
        std::print("pwd: usage: pwd\n");
        return;
    }
    auto cwd = get_current_dir_name();
    std::print("{}\n", cwd);
    free(cwd);
}

void func_cd(std::vector<std::string> args) {
    // cd
    if (args.size() == 1 || args.size() > 2) {
        // change to home directory
        std::print("cd: usage: cd <path>\n");
        return;
    }

    auto status = chdir(args[1].c_str());
    if (status == -1) {
        auto err = errno;
        switch (err) {
        case EACCES:
            std::print("cd: {}: permission denied\n", args[1]);
            break;
        case ENOENT:
            std::print("cd: {}: invalid directory\n", args[1]);
            break;
        case ENOTDIR:
            std::print("cd: {}: invalid directory path\n", args[1]);
            break;
        default:
            std::print("cd: {}: error changing directory\n", args[1]);
            break;
        }
    }
}

void func_external(std::vector<std::string> args) {
    std::vector<char *> argv;
    for (auto &arg : args) {
        argv.push_back(arg.data());
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
        execvp(argv[0], argv.data());
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
            // if (WIFEXITED(wstatus)) {
            //     std::print("exited status={}\n", WEXITSTATUS(wstatus));
            // } else if (WIFSIGNALED(wstatus)) {
            //     std::print("killed by signal {}\n", WTERMSIG(wstatus));
            // } else if (WIFSTOPPED(wstatus)) {
            //     std::print("stopped by signal {}\n", WSTOPSIG(wstatus));
            // } else if (WIFCONTINUED(wstatus)) {
            //     std::print("continued\n");
            // }
        } while (!WIFEXITED(wstatus) && !WIFSIGNALED(wstatus));
    }
}
