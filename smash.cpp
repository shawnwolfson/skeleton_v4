#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"
#include "signals.h"

#define FAIL (-1)
int main(int argc, char* argv[]) {
    if(signal(SIGTSTP , ctrlZHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-Z handler");
    }
    if(signal(SIGINT , ctrlCHandler)==SIG_ERR) {
        perror("smash error: failed to set ctrl-C handler");
    }
    //setup of handler
    struct sigaction act;
    act.sa_handler = &alarmHandler;
    act.sa_flags = SA_RESTART;
    sigemptyset(&(act.sa_mask));
    if(sigaction(SIGALRM, &act, nullptr) == FAIL)
    {
        perror("smash error: sigaction failed");
    }
    SmallShell& smash = SmallShell::getInstance();
    while(true) {
        std::cout << smash.line_prompt << "> ";
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.executeCommand(cmd_line.c_str());
    }
    return 0;
}

/*TODO
1. Check if builtin commands with apresand need to remove it, especially cd ..& and chprompt 44&
2. Add smash errors for syscalls
3. Post piazza to figure out prints in background timeout
4. Bonus
5. HARDCORE DEBUG
*/