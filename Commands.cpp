#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <fcntl.h>
#include <sched.h>
#include <thread>

//delete this
#define MAX_ARGS 20
#define MIN_ARGS 2
#define LINUX_MAX_PATH_LENGTH 4096
#define FAIL -1
#define NO_JOB_ID -1
#define BUFFER_SIZE 1024

using namespace std;
const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cout << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cout << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char* cmd_line, char** args) {
  FUNC_ENTRY()
  int i = 0;
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args[i] = (char*)malloc(s.length()+1);
    memset(args[i], 0, s.length()+1);
    strcpy(args[i], s.c_str());
    args[++i] = NULL;
  }
  return i;

  FUNC_EXIT()
}

static char** PrepareArgs (const char* cmd_line, int* arg_num)
{
  char** args = (char**) malloc(sizeof(char *)*MAX_ARGS); 
  if (args == nullptr) return nullptr;
  for (int i =0; i<MAX_ARGS; i++) {
    args[i] = nullptr;
  }
  *arg_num = _parseCommandLine(cmd_line,args);
  return args;
}

void FreeArgs (char** args , int arg_num)
{
  for (int i = 0; i<arg_num; i++)
  {
    if(args[i] != nullptr)
    {
      free(args[i]);
    }

  }
  if(args != nullptr)
  {
    free(args);
  }
}
bool compareEntries(JobsList::JobEntry entry1, JobsList::JobEntry entry2)
{
  return (entry1.job_id < entry2.job_id);
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(char* cmd_line) {
  const string str(cmd_line);
  // find last character other than spaces
  unsigned int idx = str.find_last_not_of(WHITESPACE);
  // if all characters are spaces then return
  if (idx == string::npos) {
    return;
  }
  // if the command line does not end with & then return
  if (cmd_line[idx] != '&') {
    return;
  }
  // replace the & (background sign) with space and then remove all tailing spaces.
  cmd_line[idx] = ' ';
  // truncate the command line string up to the last non-space character
  cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

bool _is_number (char* string)
{
  for (int i =0; string[i] !='\0';i++)
  {
    if(!isdigit(string[i]))
    {
      if(i ==0 && string[i] == '-')
      {
        continue;
      }
      return false;
    }
  }
  return true;
}

bool isBackGroundAndBasicCommand(string first_word)
{
  return ((first_word == "chprompt&") || (first_word == "showpid&") || (first_word == "pwd&") || (first_word == "cd&") || (first_word == "jobs&") || \
          (first_word == "fg&") || (first_word == "bg&") || (first_word == "quit&"));
}



// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() : line_prompt("smash"),last_pwd(nullptr), current_process_running_in_foreground_pid(NO_PROCCESS), last_command(new string),cur_pipe(false), 
                           jobs_list({}), alarms_list({}), current_duration(0), last_alarm(new string), process_in_foreground_got_alarm(false),fg_job_id(-1)
                           {
                            smash_pid = getpid();
                            if (smash_pid == FAIL)
                            {
                              perror("smash error: getpid failed");
                              return;
                            }
                           };

SmallShell::~SmallShell() {
  if(last_command != nullptr)
  {
    delete last_command;
  }
  if(last_alarm != nullptr)
  {
    delete last_alarm;
  }
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line, bool is_timed_command) {

  string cmd_s = _trim(string(cmd_line));

  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  
  if((cmd_s).find(">") != std::string::npos)
  {
    return new RedirectionCommand(cmd_line);
  }
  
  if((cmd_s).find("|") != std::string::npos)
  {
    return new PipeCommand(cmd_line);
  }

  if(isBackGroundAndBasicCommand(firstWord))
  {
    firstWord.pop_back();
  }

  *last_command = cmd_line;

  if (firstWord.compare("chprompt") == 0) {
    return new ChpromptCommand(cmd_line);
  }
  else if (firstWord.compare("showpid") == 0)
  {
    return new ShowPidCommand(cmd_line);
  }
  else if(firstWord.compare("pwd") == 0)
  {
    return new GetCurrDirCommand(cmd_line);
  }
  else if (firstWord.compare("cd") == 0)
  {
    return new ChangeDirCommand(cmd_line,&last_pwd);
  }
  else if (firstWord.compare("jobs") == 0)
  {
    return new JobsCommand(cmd_line, &(this->jobs_list));
  }
  else if (firstWord.compare("fg") == 0)
  {
    return new ForegroundCommand(cmd_line, &(this->jobs_list));
  }
  else if (firstWord.compare("bg") == 0)
  {
    return new BackgroundCommand(cmd_line, &(this->jobs_list));
  }
  else if (firstWord.compare("quit") == 0)
  {
    return new QuitCommand(cmd_line, &jobs_list);
  }
  else if (firstWord.compare("fare") == 0)
  {
    return new FareCommand(cmd_line);
  }
  else if (firstWord.compare("setcore") == 0)
  {
    return new SetcoreCommand(cmd_line, &(this->jobs_list));
  }
  else if (firstWord.compare("timeout") == 0)
  {
    return new TimeoutCommand(cmd_line);
  }
  else if (firstWord.compare("kill") == 0)
  {
    return new KillCommand(cmd_line, &(this->jobs_list));
  }
  else {
    return new ExternalCommand(cmd_line, is_timed_command);
  }
  return nullptr;
  
}

void SmallShell::executeCommand(const char *cmd_line, bool is_timed_command) {
  SmallShell& shell = SmallShell::getInstance();
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  shell.jobs_list.removeFinishedJobs();
  Command* cmd = CreateCommand(cmd_line, is_timed_command);
  cmd->execute();
  shell.current_process_running_in_foreground_pid = NO_PROCCESS;
  delete cmd;
  shell.current_duration = 0;
  shell.process_in_foreground_got_alarm = false;
  shell.is_fg_timed_command = false;
  /*if(firstWord.compare("kill"))
  {
    sleep(1);
  }*/
}

/////// Command Constructor //////

Command::Command (const char* cmd) :cmd_line(cmd){};

BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {};


////// Simple Built In Commands ////////

// Change Prompt

ChpromptCommand::ChpromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};

void ChpromptCommand::execute() {
  int arg_num = 0;
  char** args = PrepareArgs(cmd_line , &arg_num);
  std::string new_prompt;
  if (arg_num < MIN_ARGS) 
  {
    new_prompt = "smash";
  }
  else
  {
    new_prompt = args[1];
  }
  SmallShell & shell = SmallShell::getInstance();
  shell.line_prompt = new_prompt;
  FreeArgs(args,arg_num);
}

// Show PID

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};

void ShowPidCommand::execute() {
  SmallShell &shell = SmallShell::getInstance();

  cout<<"smash pid is "<< shell.smash_pid << endl;
}

// PWD 

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {};

void GetCurrDirCommand::execute() {
  char* path_buffer = (char *) malloc(sizeof(char) * LINUX_MAX_PATH_LENGTH);
  if(!path_buffer) return;
  auto path = getcwd(path_buffer, LINUX_MAX_PATH_LENGTH);
  if (path == nullptr)
  {
    perror("smash error: getcwd failed");
    free (path_buffer);
    return;
  }
  cout << path << endl;
  free(path_buffer);
}

// CD

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, char** plastPwd) : BuiltInCommand(cmd_line), plastPwd(plastPwd) {};

void ChangeDirCommand::execute() {
  char* plast_pwd_buffer = (char *) malloc(sizeof(char) * LINUX_MAX_PATH_LENGTH);
  int arg_num = 0;
  char** args = PrepareArgs(cmd_line , &arg_num);
  string special_char = "-";
  if (arg_num > 2)
  {
    cerr << "smash error: cd: too many arguments" << endl;
  }
  else if((special_char.compare(args[1]) == 0) && !(*plastPwd))
  {
    cerr << "smash error: cd: OLDPWD not set" << endl;
  }

  else 
  {
    if(getcwd(plast_pwd_buffer, LINUX_MAX_PATH_LENGTH) == nullptr)
    {
      perror("smash error: getcwd failed");
      FreeArgs(args,arg_num);
      free(plast_pwd_buffer);
      return;
    }
    if (special_char.compare(args[1]) == 0)
    {
      if(chdir(*plastPwd) == FAIL)
      {
          perror("smash error: chdir failed");
          FreeArgs(args,arg_num);
          free(plast_pwd_buffer);
          return;
      }
      else
      {
        if (*plastPwd)
        {
            free(*plastPwd);
        }
        *plastPwd = plast_pwd_buffer;
      }
    }
    else
    {
      if(chdir(args[1]) == FAIL)
      {
          perror("smash error: chdir failed");
          FreeArgs(args,arg_num);
          free(plast_pwd_buffer);
          return;
      }
      else
      {
        if (*plastPwd)
        {
          free(*plastPwd);
        }
       *plastPwd = plast_pwd_buffer;
      }
    }
  }
FreeArgs(args, arg_num);
}


//// Jobs List ////


JobsList::JobsList() : jobs_list({}), jobs_counter(0){};

JobsList::JobEntry::JobEntry(int job_id, __pid_t job_pid, time_t entered_list_time, char** process_args,int num_of_args, bool is_background, string* cmd_line, bool is_stopped, bool is_timed_job): job_id(job_id),
                             job_pid(job_pid),entered_list_time(entered_list_time),process_args(process_args),num_of_args(num_of_args),is_background(is_background),
                             cmd_line(cmd_line), is_stopped(is_stopped), is_timed_job(is_timed_job){};

JobsList::JobEntry* JobsList::getJobById(int jobId) 
{
  for(int i = 0; i < jobs_list.size(); i++)
  {
    if(jobs_list[i].job_id == jobId)
    {
      return &jobs_list[i];
    }
  }
  return nullptr;
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId)
{
  int last_job_id = NO_JOB_ID;
  for(int i = 0; i < jobs_list.size(); i++)
  {
    if(jobs_list[i].job_id > last_job_id)
    {
      last_job_id = jobs_list[i].job_id;
    }
  }
  *lastJobId = last_job_id;
  JobEntry* last_job_entry = getJobById(last_job_id);
  return last_job_entry;
}



void JobsList::addJob(const char* cmd_line, __pid_t job_pid, bool isStopped  ,bool is_timed_command , time_t duration, int force_job_id)
{
  this->removeFinishedJobs();
  int args_num = 0;
  char** args_for_job = PrepareArgs(cmd_line, &args_num);
  string temp = "timeout " + std::to_string(duration) + " ";
  string cmd_line_string = cmd_line;
  string* unique_cmd_line = new string(cmd_line_string);
  int new_job_id;
  if(force_job_id > -1)
  {
    new_job_id = force_job_id;
  }
  else
  {
    new_job_id = jobs_counter;
  }
  if(is_timed_command)
  {
    (*unique_cmd_line) = temp +(*unique_cmd_line);
    jobs_list.push_back(JobEntry(new_job_id, job_pid, time(nullptr), args_for_job, args_num, _isBackgroundComamnd(cmd_line),unique_cmd_line, isStopped, is_timed_command)); 
  }
  else
  {
    jobs_list.push_back(JobEntry(new_job_id, job_pid, time(nullptr), args_for_job, args_num, _isBackgroundComamnd(cmd_line), unique_cmd_line, isStopped, is_timed_command)); 
  }
  if(force_job_id == -1)
  {
  jobs_counter += 1;
  }
}

void JobsList::killAllJobs()
{
  this->removeFinishedJobs();
  for(int i = 0; i < jobs_list.size(); i++)
  {
    cout << jobs_list[i].job_pid << ": " << *(jobs_list[i].cmd_line) << endl;
    if(kill(jobs_list[i].job_pid, SIGKILL) == FAIL)
    {
      perror("smash error: kill failed");
      return;
    }
  }
}

void JobsList::printJobsList()
{
  this->removeFinishedJobs();
  sort(jobs_list.begin(), jobs_list.end(), compareEntries);
  for(int i = 0; i < jobs_list.size(); i++)
  {
    time_t elapsed_time = difftime(time(nullptr), jobs_list[i].entered_list_time);
    if(jobs_list[i].is_stopped == true)
    {
      cout << "[" << jobs_list[i].job_id << "] " << *(jobs_list[i].cmd_line) << " : ";
      cout << jobs_list[i].job_pid << " " << elapsed_time << " secs " << "(stopped)" << endl;
    }
    else
    {
      cout << "[" << jobs_list[i].job_id << "] " << *(jobs_list[i].cmd_line) << " : ";
      cout << jobs_list[i].job_pid << " " << elapsed_time << " secs" << endl;      
    }
  }
}

JobsList::JobEntry * JobsList::getLastStoppedJob(int *jobId)
{
  int last_job_id = NO_JOB_ID;
  for(auto& current_job : jobs_list)
  {
    if((current_job.job_id > last_job_id) && (current_job.is_stopped == true))
    {
      last_job_id = current_job.job_id;
    }
  }
  *jobId = last_job_id;
  JobEntry* last_job_entry = getJobById(last_job_id);
  return last_job_entry; 
}

void JobsList::removeJobById(int jobId)
{
  for(auto current_job = jobs_list.begin() ; current_job != jobs_list.end(); current_job++)  
  {
    if(current_job->job_id == jobId)
    {
      if(current_job->process_args != nullptr)
      {
        FreeArgs(current_job->process_args, current_job->num_of_args);
      }
      if(current_job->cmd_line != nullptr)
      {
        delete(current_job->cmd_line);
      }
      jobs_list.erase(current_job);
      break;
    }
  }
  //check if need to update jobs counter 
}

void JobsList::removeFinishedJobs()
{
  //Explanation for us: need to know for each job if it is finished, and if so then erase it. At the end update number of jobs and jobs counter
  int wstatus = 0;
  int options = WNOHANG;
  SmallShell &shell = SmallShell::getInstance();
  if(!shell.cur_pipe)
  {
  for (auto& current_job : jobs_list)
  {
    int return_value = waitpid(current_job.job_pid, &wstatus, options);
    //if returned with positive value then the son is finished
    if(return_value == -1 || return_value == current_job.job_pid)
    {
      this->removeJobById(current_job.job_id);
    }
  }
  }

  //after deleting need to calc the new counter
  int new_jobs_counter = 0;
  if(jobs_list.size() == 0)
  {
    jobs_counter = 1;
  }
  else
  {
    for(auto& current_job : jobs_list)
    {
      if(current_job.job_id > new_jobs_counter)
      {
        new_jobs_counter = current_job.job_id;
      }
    }
    jobs_counter = new_jobs_counter + 1;
  }
}
//Jobs command

JobsCommand::JobsCommand(const char* cmd_line, JobsList* jobs_list) : BuiltInCommand(cmd_line), jobs_list(jobs_list) {}

void JobsCommand::execute() {
  jobs_list->removeFinishedJobs();
  jobs_list->printJobsList();
}

//// Foreground Command/////
ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList* jobs): BuiltInCommand(cmd_line) , jobs_list(jobs){};
void ForegroundCommand::execute()
{
  SmallShell& smash = SmallShell::getInstance();
  int arg_num = 0;
  char** args = PrepareArgs(cmd_line , &arg_num);
  if ((arg_num  > 2))
  {
    cerr << "smash error: fg: invalid arguments"<<endl;
    FreeArgs(args,arg_num);
    return;
  }
  else if(arg_num == 2 && !_is_number(args[1]))
  {
    cerr << "smash error: fg: invalid arguments"<<endl;
    FreeArgs(args,arg_num);
    return;   
  }
  int last_job_id;
  JobsList::JobEntry* req_job;
  //specific job
  if (arg_num == 2)
  {
    req_job = smash.jobs_list.getJobById(stoi(args[1]));
    if (!req_job)
    {
      cerr << "smash error: fg: job-id "<<args[1]<<" does not exist" << endl;
      FreeArgs(args,arg_num);
      return;
    }
  }
  //last job
  else
  {
    int job_id;
    req_job = smash.jobs_list.getLastJob(&job_id);
    if (!req_job)
    {
      cerr << "smash error: fg: jobs list is empty"<<endl;
      FreeArgs(args,arg_num);
      return;
    }
  }

  cout << *(req_job->cmd_line) << " : " << req_job->job_pid << endl; 

  if (req_job->is_stopped)
  {
    if(kill(req_job->job_pid,SIGCONT) == FAIL)
    {
      perror("smash error: kill failed");
      FreeArgs(args,arg_num);
      return;
    }
  }
  *(smash.last_command) = *(req_job->cmd_line);
  smash.current_process_running_in_foreground_pid = req_job->job_pid;
  smash.fg_job_id = req_job->job_id;
  if (req_job->is_timed_job)
  {
    smash.is_fg_timed_command = true;
  }
  pid_t req_pid = req_job->job_pid;
  int process_status;
  jobs_list->removeJobById(req_job->job_id);
  if(waitpid(req_pid , &process_status, WUNTRACED) == FAIL)
  {
      perror("smash error: waitpid failed");
      FreeArgs(args,arg_num);
      return;  
  }
  
  smash.fg_job_id = -1;
  smash.is_fg_timed_command = false;
  FreeArgs(args,arg_num);
}

//// Background Commaned//////
BackgroundCommand::BackgroundCommand(const char* cmd_line, JobsList* jobs):BuiltInCommand(cmd_line) , jobs_list(jobs){};

void BackgroundCommand::execute()
{
  int arg_num = 0;
  char** args = PrepareArgs(cmd_line , &arg_num);
  SmallShell& smash = SmallShell::getInstance();
  if (arg_num  > 2)
  {
    cerr << "smash error: bg: invalid arguments" << endl;
    FreeArgs(args,arg_num);
    return;
  }
  else if(arg_num == 2 && !_is_number(args[1]))
  {
    cerr << "smash error: fg: invalid arguments"<<endl;
    FreeArgs(args,arg_num);
    return;   
  }
  int last_job_id;
  JobsList::JobEntry* req_job;

  if (arg_num == 2)
  {
    req_job = smash.jobs_list.getJobById(stoi(args[1]));
    if (!req_job)
    {
      cerr << "smash error: bg: job-id "<< args[1] <<" does not exist" <<endl;
      FreeArgs(args,arg_num);
      return;
    }
  }
  else
  {
    int job_id;
    req_job = jobs_list->getLastStoppedJob(&job_id);
    if (!req_job)
    {
      cerr << "smash error: bg: there is no stopped jobs to resume" << endl;
      FreeArgs(args,arg_num);
      return;
    }
  }

  if (!(req_job->is_stopped))
  {
      cerr << "smash error: bg: job-id "<< args[1] <<" is already running in the background" << endl;
      FreeArgs(args,arg_num);
      return;
  }
  *(smash.last_command) = *(req_job->cmd_line);
  cout << *(req_job->cmd_line) << " : " << req_job->job_pid << endl; 
  if(kill(req_job->job_pid,SIGCONT) == FAIL)
  {
      perror("smash error: kill failed");
      FreeArgs(args,arg_num);
      return;
  }
  req_job->is_stopped = false;
  FreeArgs(args,arg_num);
}


//// Quit /////

QuitCommand::QuitCommand(const char* cmd_line, JobsList* jobs) : BuiltInCommand(cmd_line) , jobs_list(jobs){};

void QuitCommand::execute()
{
  SmallShell& smash = SmallShell::getInstance();
  int arg_num = 0;
  char** args = PrepareArgs(cmd_line , &arg_num);
  string killfalg = "kill";
  jobs_list->removeFinishedJobs();
  if ((arg_num > 1) && (killfalg.compare(args[1])) == 0)
  {
    cout << "smash: sending SIGKILL signal to " << jobs_list->jobs_list.size() <<" jobs:" <<endl;
    smash.jobs_list.killAllJobs();
  }
  FreeArgs(args, arg_num);
  exit(0);
}

//// External Commandes ////

ExternalCommand::ExternalCommand(const char* cmd_line, bool is_timed_command) : Command(cmd_line), is_timed_command(is_timed_command){};

void ExternalCommand::execute()
{
  
  std::string command_line = _trim(string(cmd_line));
  bool is_complex_command = false;
  //Find out if the external command is complex or not
  if (command_line.find("*") != std::string::npos || command_line.find("?") != std::string::npos)
  {
    is_complex_command = true;
  }
   
  char new_command [MAX_COMMAND_LENGTH];
  strcpy(new_command,command_line.c_str());

  bool background_command = _isBackgroundComamnd(cmd_line);
  if(background_command)
  {
    _removeBackgroundSign(new_command);
  }  
  //Prepare arguments
  char* binbash = (char*)"/bin/bash";
  char* special_args[] =  {binbash,(char*)"-c",new_command,nullptr};

  int arg_num = 0;
  char** args = PrepareArgs(new_command , &arg_num);
  __pid_t process_pid = fork();
  if (process_pid == 0)
  {
    if (setpgrp() == FAIL)
    {
      perror("smash error: setpgrp failed");
      FreeArgs(args,arg_num);
      return;
    }
    if(is_complex_command)
    {
      if(execv(binbash,special_args) == FAIL)
      {
        perror("smash error: execvfailed");
        FreeArgs(args,arg_num);
        return;
      }
    }
    else
    {
      if (execvp(args[0],args) == FAIL)
      {
        perror("smash error: execvp failed");
        FreeArgs(args,arg_num);
        return;
      }
    }
  }
  //Father process e.g smash
  //father
  else
  {
    SmallShell &smash = SmallShell::getInstance();
    if(background_command) 
    {
      smash.jobs_list.addJob(cmd_line,process_pid,false, is_timed_command, smash.current_duration, smash.fg_job_id);
      if(is_timed_command)
      {
        smash.alarms_list.addAlarm(cmd_line, process_pid, smash.current_duration);
      }
      this->is_timed_command = false;
    }
    else
    {
      smash.current_process_running_in_foreground_pid = process_pid;
      *(smash.last_command) = cmd_line;
      if(is_timed_command)
      {
        smash.process_in_foreground_got_alarm = true;
      }
      int status;
      if(waitpid(process_pid,&status,WUNTRACED) == FAIL)
      {
        perror("smash error: waitpid failed");
        FreeArgs(args,arg_num);
        return;
      }
    }
  }
  FreeArgs(args,arg_num);
}


//// Redirection Commands ////

RedirectionCommand::RedirectionCommand(const char* cmd_line) : Command(cmd_line) {
  //Figure out if we need to append or override
  string cmd_line_as_string(cmd_line);
  cmd_line_as_string = _trim(cmd_line_as_string);
  int type_of_redirection = 0;
  string redirection_symbol = ">";
  for(auto letter : cmd_line_as_string)
  {
    if(letter == redirection_symbol[0])
    {
      type_of_redirection++;
    }
    if(type_of_redirection >= 2)
    {
      break;
    }
  }
  if(type_of_redirection == 1)
  {
    do_override = true;
  }
  else
  {
    do_override = false;
  }
  //Extract command for shell
  int first_occurence_of_red_sym = cmd_line_as_string.find_first_of('>');
  string command = cmd_line_as_string.substr(0, first_occurence_of_red_sym);
  //Extract filename
  int last_occurence_of_redirect_sym = cmd_line_as_string.find_last_of('>');
  string filename = cmd_line_as_string.substr(last_occurence_of_redirect_sym + 1, cmd_line_as_string.size());
  filename = _trim(filename);
  char temp_for_background_removal[FILENAME_MAX + 1];
  if(_isBackgroundComamnd(filename.c_str()))
  {
    strcpy(temp_for_background_removal, filename.c_str());
    _removeBackgroundSign(temp_for_background_removal);
    filename = (string)temp_for_background_removal;
  }

  //Do allocations
  this->short_command = new string(command);
  this->clean_cmd_line = new string(cmd_line_as_string);
  this->file_name = new string(filename);
}

void RedirectionCommand::execute()
{
  int fd = 0;
  int last_save_of_current_stdout = dup(1);
  if(close(1) == FAIL)
  {
    perror("smash error: close failed");
    return;
  }
  if(do_override)
  {
    fd = open((*file_name).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0655);
  }
  else
  {
    fd = open((*file_name).c_str(), O_WRONLY | O_CREAT | O_APPEND, 0655);
  }
  if(fd == FAIL)
  {
    perror("smash error: open failed");
    return;
  }
  SmallShell& shell = SmallShell::getInstance();
  shell.executeCommand((*short_command).c_str());

  //restore everything
  if(close(fd) == FAIL)
  {
    perror("smash error: close failed");
  }
  int dup_status = dup2(last_save_of_current_stdout, 1);
  if(dup_status == FAIL)
  {
    perror("smash error: dup2 failed");
  }
  if(close(last_save_of_current_stdout) == FAIL)
  {
    perror("smash error: close failed");
  }
  return;
}


////// Pipe Commands ////////

PipeCommand::PipeCommand(const char* cmd_line) : Command(cmd_line)
{
  std::string cmd_line_as_string = string(cmd_line);

  if((cmd_line_as_string).find("|&") != std::string::npos)
  {
    delimiter = "|&";
    second_command = cmd_line_as_string.substr(cmd_line_as_string.find(delimiter)+2,cmd_line_as_string.length());
  }
  else
  {
    delimiter = "|";
    second_command = cmd_line_as_string.substr(cmd_line_as_string.find(delimiter)+1,cmd_line_as_string.length());

  }
  first_command = cmd_line_as_string.substr(0,cmd_line_as_string.find(delimiter));
}

void PipeCommand::execute()
{
  SmallShell &shell = SmallShell::getInstance();
  int fds[2];
  if(pipe(fds) == FAIL)
  {
    perror("smash error: pipe failed");
    return;
  }

  __pid_t first_pid,second_pid;
  first_pid = fork();
  if(first_pid == FAIL)
  {
    perror("smash error: fork failed");
    if(close(fds[0]) == FAIL || close(fds[1]) == FAIL)
    {
      perror("smash error: close failed");
    }
    return;
  }

  if(first_pid == 0)
  {
    if(setpgrp() == FAIL)
    {
      perror("smash error: setpgrp failed");
      if(close(fds[0]) == FAIL || close(fds[1]) == FAIL)
      {
        perror("smash error: close failed");
      }
      return;
    }
    if (delimiter.compare("|") == 0)
    {
      if(dup2(fds[1],1) == FAIL)
      {
        perror("smash error: dup2 failed");
        if(close(fds[0]) == FAIL || close(fds[1]) == FAIL)
        {
          perror("smash error :close failed");
        }
        return;
      }
    }
    else
    {
      if(dup2(fds[1],2) == FAIL)
      {
        perror("smash error: dup2 failed");
        if(close(fds[0]) == FAIL || close(fds[1]) == FAIL)
        {
          perror("smash error :close failed");
        }
        return;
      }
    }
    if(close(fds[0]) == FAIL || close(fds[1]) == FAIL)
      {
       perror("smash error :close failed");
      }

    shell.cur_pipe = true;
    shell.executeCommand(first_command.c_str());
    exit(0);
  }
  second_pid = fork();
  if(second_pid == FAIL)
  {
    perror("smash error: fork failed");
    if(close(fds[0]) == FAIL || close(fds[1]) == FAIL)
    {
      perror("smash error: close failed");
    }
    return;
  }

  if(second_pid == 0)
  {
    if(setpgrp() == FAIL)
    {
      perror("smash error: setpgrp failed");
      if(close(fds[0]) == FAIL || close(fds[1]) == FAIL)
      {
        perror("smash error: close failed");
      }
      return;
    }
    if(dup2(fds[0],0) == FAIL)
      {
      perror("smash error: dup2 failed");
      if(close(fds[0]) == FAIL || close(fds[1]) == FAIL)
        {
          perror("smash error :close failed");
        }
        return;
      }
    
    if(close(fds[0]) == FAIL || close(fds[1]) == FAIL)
      {
       perror("smash error :close failed");
      }

    shell.cur_pipe = true;
    shell.executeCommand(second_command.c_str());
    exit(0);
  }

  if(close(fds[0]) == FAIL || close(fds[1]) == FAIL)
  {
    perror("smash error :close failed");
  }

  if(waitpid(first_pid,nullptr,WUNTRACED) == FAIL)
  {
    perror("smash error :waitpid failed");
    return;
  }
  if(waitpid(second_pid,nullptr,WUNTRACED) == FAIL)
  {
    perror("smash error :waitpid failed");
    return;
  }
}



////// Fare Command //////

FareCommand::FareCommand(const char* cmd_line) : BuiltInCommand(cmd_line), words_replaced(0) {}

void FareCommand::execute()
{
  int args_num = 0;
  char** args = PrepareArgs(cmd_line, &args_num);
  //TODO : add input check
  int original_file_descriptor;
  int temp_file_descriptor;
  string word;
  string file_name = (string)args[1];
  string file_format = file_name.substr(file_name.find_first_of('.'), file_name.size());
  string source = (string)args[2];
  string dest = (string)args[3];
  string temp_file_name = "t_temp_try_7" + file_format;
  int replaces_counter = 0;
  original_file_descriptor = open(file_name.c_str(), O_RDONLY);
  if(original_file_descriptor == FAIL)
  {
    perror("smash error: open failed");
    FreeArgs(args, args_num);
    return;
  }
  temp_file_descriptor = open(temp_file_name.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
  if(temp_file_descriptor == FAIL){
    perror("smash error: open failed");
    FreeArgs(args, args_num);
    return;
  }
  while(1)
  {
    char* buffer = (char*) malloc(sizeof(char) * BUFFER_SIZE);
    int read_res = read(original_file_descriptor, buffer, 2 * source.size());
    if(read_res == FAIL)
    {
      perror("smash error: read failed");
      FreeArgs(args, args_num);
      free(buffer);
      return;
    }
    else if(read_res == 0)
    {
      break;
    }
    word = (string)buffer;
    int curr_position = 0;
    while((curr_position = word.find(source)) != std::string::npos)
    {
      word.replace(curr_position, source.size(), dest);
      curr_position += dest.size();
      replaces_counter += 1;
    }
    int write_res = write(temp_file_descriptor, word.c_str(), word.size());
    if(write_res == FAIL)
    {
      perror("smash error: write failed");
      FreeArgs(args, args_num);
      free(buffer);
      return;
    }
    free(buffer);
  }
  //If everything is ok
  int close_original_res = close(original_file_descriptor);
  if(close_original_res == FAIL)
  {
    perror("smash error: write failed");
    FreeArgs(args, args_num);
    return;
  }
  int close_temp_res = close(temp_file_descriptor);
  if(close_temp_res == FAIL)
  {
    perror("smash error: write failed");
    FreeArgs(args, args_num);
    return;
  }
  //Delete old file
  int remove_res = remove(file_name.c_str());
  if(remove_res == FAIL)
  {
    perror("smash error: remove failed");
    FreeArgs(args, args_num);
    return;
  }
  
  //Rename new file
  int rename_res = rename(temp_file_name.c_str(), file_name.c_str());
  if(rename_res == FAIL)
  {
    perror("smash error: rename failed");
    FreeArgs(args, args_num);
    return;   
  }
  cout << "replaced 2 instances of the string " << '"' << source << '"' << endl;
  FreeArgs(args, args_num);
}



///// Set Core Command ////
SetcoreCommand::SetcoreCommand(const char* cmd_line, JobsList* jobs_list) : BuiltInCommand(cmd_line) ,jobs_list(jobs_list){};

 void SetcoreCommand::execute ()
 {
  int args_num = 0;
  char** args = PrepareArgs(cmd_line, &args_num);
  auto max_core_num = std::thread::hardware_concurrency() - 1;
  if (!_is_number(args[1]) || !_is_number(args[2]))
  {
    perror("smash error: setcore: invalid arguments");
    FreeArgs(args,args_num);
    return;
  }
  auto req_job = jobs_list->getJobById(stoi(args[1]));
  if(!req_job)
  {
    cerr << "smash error: setcore: job-id " << stoi(args[1]) << " does not exist" << endl;
    FreeArgs(args,args_num);
    return;
  }
  int req_core = stoi(args[2]);
  if(req_core < 0 || req_core>max_core_num)
  {
    perror("smash error: setcore: invalid core number");
    FreeArgs(args,args_num);
    return;    
  }

  cpu_set_t mask;
  CPU_ZERO(&mask);
  CPU_SET(req_core,&mask);
  kill(req_job->job_pid, SIGTSTP);
  if(sched_setaffinity(req_job->job_pid,sizeof(mask),&mask) == FAIL)
  {
    perror("smash error: sched_setaffinity failed");
    FreeArgs(args,args_num);
    return; 
  }
  kill(req_job->job_pid, SIGCONT);
  FreeArgs(args,args_num);
 }



//Alarms and Timeout Command ///

AlarmsList::AlarmsList() : alarms_list({}) {}

AlarmsList::AlarmEntry::AlarmEntry(time_t entered_list_time, time_t duration, std::string* cmd_line, __pid_t pid) :
                                  entered_list_time(entered_list_time), duration(duration),max_time(entered_list_time + duration), cmd_line(cmd_line), pid(pid) {}

void AlarmsList::addAlarm(const char* cmd_line, __pid_t pid, time_t duration)
{
  string* cmd_line_as_string = new string(cmd_line);
  alarms_list.push_back(AlarmEntry(time(nullptr), duration, cmd_line_as_string, pid));
  time_t current_time = time(nullptr);
  time_t next_alarm = alarms_list[0].max_time - current_time;
  for(int i = 0; i < alarms_list.size(); i++)
  {
    if(alarms_list[i].max_time - current_time < next_alarm)
    {
      next_alarm = alarms_list[i].max_time - current_time;
    }
  }
  alarm(next_alarm);
}

void AlarmsList::removeAlarms()
{
  for( auto current_alarm = alarms_list.begin(); current_alarm != alarms_list.end(); current_alarm++)
  {
    if(time(nullptr) >= current_alarm->max_time)
    {
      cout << "smash: timeout " << current_alarm->duration << " " << *(current_alarm->cmd_line) << " timed out!" << endl;
      if(current_alarm->cmd_line != nullptr)
      {
        delete current_alarm->cmd_line;
      }
      kill(current_alarm->pid, SIGKILL);
      alarms_list.erase(current_alarm);
      current_alarm--;
    }
  }
  if(alarms_list.size() == 0)
  {
    return;
  }
  time_t current_time = time(nullptr);
  time_t next_alarm = alarms_list[0].max_time - current_time;
  for(int i = 0; i < alarms_list.size(); i++)
  {
    if(alarms_list[i].max_time - current_time < next_alarm)
    {
      next_alarm = alarms_list[i].max_time - current_time;
    }
  }
  alarm(next_alarm);
}

TimeoutCommand::TimeoutCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void TimeoutCommand::execute()
{
  SmallShell& smash = SmallShell::getInstance();
  int arg_num = 0;
  char** args = PrepareArgs(cmd_line, &arg_num);
  if(!args)
  {
    perror("smash error: malloc failed");
    return;
  }
  time_t duration = stoi(args[1]);
  //Extract command
  string command;
  for(int i = 2; i < arg_num; i++)
  {
    command = command + (string)args[i] + " ";
  }
  command.pop_back();
  //alarm(duration);
  smash.current_duration = duration;
  *(smash.last_alarm) = (string)cmd_line;
  if(!_isBackgroundComamnd(cmd_line))
  {
    smash.is_fg_timed_command = true;
  }
  smash.executeCommand(command.c_str(), true);
  FreeArgs(args, arg_num);
}


////Kill command //////

KillCommand::KillCommand(const char* cmd_line, JobsList* job_list) : BuiltInCommand(cmd_line), jobs_list(job_list) {}


void KillCommand::execute()
{
  int arg_num = 0;
  char** args = PrepareArgs(cmd_line, &arg_num);

  if(arg_num != 3)
  {
    cerr << "smash error: kill: invalid arguments" << endl;
    FreeArgs(args, arg_num);
    return;
  }
  string second_argument = (string)args[1];
  string third_argument = (string)args[2];
  if(second_argument.find("-") != 0)
  {
    cerr << "smash error: kill: invalid arguments" << endl;
    FreeArgs(args, arg_num);
    return;
  }
  else if(second_argument.find_first_not_of("-") != 1)
  {
    cerr << "smash error: kill: invalid arguments" << endl;
    FreeArgs(args, arg_num); 
    return; 
  }  
  else if(!_is_number(args[1] + 1) || !_is_number(args[2]))
  {
    cerr << "smash error: kill: invalid arguments" << endl;
    FreeArgs(args, arg_num);   
    return;  
  }
  else if (stoi(args[1] + 1) > 64 || stoi(args[1] + 1) < 1)
  {
    cerr << "smash error: kill: invalid arguments" << endl;
    FreeArgs(args, arg_num);   
    return; 
  }

  auto req_job = jobs_list->getJobById(stoi(args[2]));
  if(!req_job)
  {
    cerr << "smash error: kill: job-id " << stoi(args[2]) << " does not exist" << endl;
    FreeArgs(args,arg_num);
    return;
  }

  pid_t req_job_pid = req_job->job_pid;
  auto signal = stoi(args[1]+1);
  if(kill(req_job_pid,signal)==FAIL)
  {
    perror("smash error : kill failed");
    FreeArgs(args,arg_num);
    return;
  }
  cout << "signal number " << args[1] + 1 << " was sent to pid " << req_job->job_pid << endl;
  if(signal == SIGCONT)
  {
    req_job->is_stopped = false;
  }
  if(signal == SIGTSTP)
  {
    req_job->is_stopped = true;
  }
  if(signal == SIGKILL || signal == SIGTERM)
  {
    jobs_list->removeJobById(req_job->job_id);
  }
  FreeArgs(args,arg_num);
}
