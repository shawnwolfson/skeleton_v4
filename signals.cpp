#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

void ctrlZHandler(int sig_num) {
  cout << "smash: got ctrl-Z" << endl;
  SmallShell& smash = SmallShell::getInstance();
  if(smash.current_process_running_in_foreground_pid > NO_PROCCESS)
  {
    smash.jobs_list.addJob((*(smash.last_command)).c_str(), smash.current_process_running_in_foreground_pid, true);
    kill(smash.current_process_running_in_foreground_pid, SIGSTOP);
    cout << "smash: process " << smash.current_process_running_in_foreground_pid << " was stopped" << endl;
    *(smash.last_command) = "";
    smash.current_process_running_in_foreground_pid = NO_PROCCESS;
  }
}


void ctrlCHandler(int sig_num) {
  cout << "smash: got ctrl-C" << endl;
  SmallShell& smash = SmallShell::getInstance();
  if(smash.current_process_running_in_foreground_pid > NO_PROCCESS)
  {
    kill(smash.current_process_running_in_foreground_pid, SIGKILL);
    cout << "smash: process " << smash.current_process_running_in_foreground_pid << " was killed" << endl;
    *(smash.last_command) = "";
    smash.current_process_running_in_foreground_pid = NO_PROCCESS;
  }
}

void alarmHandler(int sig_num) {
  // TODO: Add your implementation
}

