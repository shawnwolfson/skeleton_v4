#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <iterator>
#include <bits/stdc++.h>

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define MAX_COMMAND_LENGTH (80)
#define NO_PROCCESS (-1)

class Command {
 public:
  const char* cmd_line;
  Command(const char* cmd_line);
  virtual ~Command() {}
  virtual void execute() = 0;
  //virtual void prepare();
  //virtual void cleanup();
  // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
 public:
  BuiltInCommand(const char* cmd_line);
  virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
 public:
  ExternalCommand(const char* cmd_line);
  virtual ~ExternalCommand() {}
  void execute() override;
};

class PipeCommand : public Command {
  std::string first_command;
  std::string second_command;
  std::string delimiter;

 public:
  PipeCommand(const char* cmd_line);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
  std::string* short_command;
  std::string* clean_cmd_line;
  std::string* file_name;
  bool do_override;
  int fd;
 public:
  explicit RedirectionCommand(const char* cmd_line);
  virtual ~RedirectionCommand() {
    if(clean_cmd_line != nullptr)
    {
      delete clean_cmd_line;
    }
    if(file_name != nullptr)
    {
      delete file_name;
    }
    if(short_command != nullptr)
    {
      delete short_command;
    }
  }
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
  public:
    char** plastPwd;
    ChangeDirCommand(const char* cmd_line, char** plastPwd);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
 public:
  GetCurrDirCommand(const char* cmd_line);
  virtual ~GetCurrDirCommand() {}
  void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
 public:
  ShowPidCommand(const char* cmd_line);
  virtual ~ShowPidCommand() {}
  void execute() override;
};

class ChpromptCommand : public BuiltInCommand {
  public:
  ChpromptCommand(const char* cmd_line);
  virtual ~ChpromptCommand() {}
  void execute() override;
};

class JobsList;
class QuitCommand : public BuiltInCommand {
 JobsList* jobs_list;
 public:
  QuitCommand(const char* cmd_line, JobsList* jobs);
  virtual ~QuitCommand() {}
  void execute() override;
};


class JobsList {
 public:
  class JobEntry {
  public:
    int job_id;
    __pid_t job_pid;
    time_t entered_list_time;
    char** process_args;
    int num_of_args;
    bool is_background;
    bool is_stopped;
    std::string* cmd_line;
    JobEntry(int job_id, __pid_t job_pid, time_t entred_list_time, char** process_args,int num_of_args, bool is_background, std::string* cmd_line, bool is_stopped);
    virtual ~JobEntry() {}
  };

 public:
  std::vector<JobEntry> jobs_list;
  int jobs_counter = 1;
  JobsList();
  virtual ~JobsList() {}
  void addJob(const char* cmd_line, __pid_t job_pid, bool isStopped = false);
  void printJobsList();
  void killAllJobs(int* killed);
  void removeFinishedJobs();
  JobEntry * getJobById(int jobId);
  void removeJobById(int jobId);
  JobEntry * getLastJob(int* lastJobId);
  JobEntry *getLastStoppedJob(int *jobId);
  // TODO: Add extra methods or modify exisitng ones as needed
};

class JobsCommand : public BuiltInCommand {
 public:
  JobsList* jobs_list;
  JobsCommand(const char* cmd_line, JobsList* jobs_list);
  virtual ~JobsCommand() {}
  void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
 JobsList* jobs_list;
 public:
  ForegroundCommand(const char* cmd_line, JobsList* jobs_list);
  virtual ~ForegroundCommand() {}
  void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
 JobsList* jobs_list;
 public:
  BackgroundCommand(const char* cmd_line, JobsList* jobs_list);
  virtual ~BackgroundCommand() {}
  void execute() override;
};

class TimeoutCommand : public BuiltInCommand {
/* Optional */
// TODO: Add your data members
 public:
  explicit TimeoutCommand(const char* cmd_line);
  virtual ~TimeoutCommand() {}
  void execute() override;
};

class FareCommand : public BuiltInCommand {
  int words_replaced;
 public:
  FareCommand(const char* cmd_line);
  virtual ~FareCommand() {}
  void execute() override;
};

class SetcoreCommand : public BuiltInCommand {
  /* Optional */
  // TODO: Add your data members
 public:
  SetcoreCommand(const char* cmd_line);
  virtual ~SetcoreCommand() {}
  void execute() override;
};

class KillCommand : public BuiltInCommand {
  /* Bonus */
 // TODO: Add your data members
 public:
  KillCommand(const char* cmd_line, JobsList* jobs);
  virtual ~KillCommand() {}
  void execute() override;
};

class SmallShell {
 private:
  
  SmallShell();
 public:
  std::string line_prompt;
  JobsList jobs_list;
  char* last_pwd;
  int current_process_running_in_foreground_pid;
  std::string* last_command;
  bool cur_pipe;
  Command *CreateCommand(const char* cmd_line);
  SmallShell(SmallShell const&)      = delete; // disable copy ctor
  void operator=(SmallShell const&)  = delete; // disable = operator
  static SmallShell& getInstance() // make SmallShell singleton
  {
    static SmallShell instance; // Guaranteed to be destroyed.
    // Instantiated on first use.
    return instance;
  }
  ~SmallShell();
  void executeCommand(const char* cmd_line);
};

#endif //SMASH_COMMAND_H_
