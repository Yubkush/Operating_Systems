#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"
#include <limits.h>
#include <sys/types.h>
#include <time.h>
#include <algorithm>

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

void _parseCommandLine(const char* cmd_line, std::vector<std::string>& args) {
  FUNC_ENTRY()
  std::istringstream iss(_trim(string(cmd_line)).c_str());
  for(std::string s; iss >> s; ) {
    args.push_back(s);
  }

  FUNC_EXIT()
}

bool _isBackgroundComamnd(const char* cmd_line) {
  const string str(cmd_line);
  return str[str.find_last_not_of(WHITESPACE)] == '&';
}

void _removeBackgroundSign(string& cmd_line) {
  // find last character other than spaces
  unsigned int idx = cmd_line.find_last_not_of(WHITESPACE);
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
  cmd_line.erase(cmd_line.find_last_not_of(WHITESPACE, idx)+1,1);
}

// TODO: Add your implementation for classes in Commands.h 
Command::Command(const char* cmd_line): line(cmd_line), is_bg(false) {
  if(_isBackgroundComamnd(cmd_line)) {
    is_bg = true;
  }
  _removeBackgroundSign(line);
  _parseCommandLine(line.c_str(), this->args);
}

BuiltInCommand::BuiltInCommand(const char* cmd_line) : Command(cmd_line) {}

ChangePromptCommand::ChangePromptCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}
void ChangePromptCommand::execute() {
  if (this->args.size() > 1) {
    SmallShell::getInstance().setPrompt(this->args[1]);
  }
  else {
    SmallShell::getInstance().setPrompt("smash");
  }
}

ShowPidCommand::ShowPidCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void ShowPidCommand::execute() {
  cout << "smash pid is " << getpid() << endl;
}

GetCurrDirCommand::GetCurrDirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {}

void GetCurrDirCommand::execute() {
  char cwd[COMMAND_ARGS_MAX_LENGTH];
  if (getcwd(cwd, sizeof(cwd)) != NULL) {
    cout << cwd << endl;
  }
  else {
    perror("smash error: getcwd failed");
  }
}

ChangeDirCommand::ChangeDirCommand(const char* cmd_line, std::string& last_pwd) : BuiltInCommand(cmd_line), last_pwd(last_pwd){}

void ChangeDirCommand::execute () {
  if(this->args.size() > 2){
    perror("smash error: cd: too many arguments");
    return;
  }
  if(this->args.size() == 1){
    return;
  }
  char cwd[PATH_MAX+1];
  if (getcwd(cwd, sizeof(cwd)) == NULL) {
    perror("smash error: getcwd failed");
    return;
  }
  // if we get input - move to last dir
  if ((this->args[1]).compare("-") == 0) {
    if((this->last_pwd).compare("") == 0){
      perror("smash error: cd: OLDPWD not set");
    }
    int res = chdir((this->last_pwd).c_str());
    if(res == -1){
      perror("smash error: chdir failed");
      return;
    }
    last_pwd = std::string(cwd);
  }
  else{
    int res = chdir(args[1].c_str());
    if(res == -1){
      perror("smash error: chdir failed");
      return;
    }
    last_pwd = std::string(cwd);
  }
}

ExternalCommand::ExternalCommand(const char* cmd_line): Command(cmd_line) {}

void ExternalCommand::execute(){
  pid_t pid = fork();
  if(pid == 0) {
    if(setpgrp() == -1){
      perror("smash error: setpgrp failed");
      return;
    }
    // simple external
    if(line.find('*') == std::string::npos && line.find('?') == std::string::npos) {
      std::vector<const char*> command_args;
      for(std::string& arg : this->args) {
        command_args.push_back(arg.c_str());
      }
      command_args.push_back(nullptr);
      
      execv(command_args[0], const_cast<char* const*>(command_args.data()));
      perror("smash error: execv failed");
      exit(EXIT_FAILURE);
    }
    // Complex external
    else {
      char arr[line.size()+1];
      strcpy(arr, line.c_str());
      char file[] = "/bin/bash";
      char c[] = "-c";
      execl("/bin/bash", file, c, arr, nullptr);
      perror("smash error: execl failed");
      exit(EXIT_FAILURE);
    }
  }
  else if(pid == -1){
    perror("smash error: fork failed");
  }
  else {
    if(this->is_bg){
      SmallShell::getInstance().getJobsList().addJob(this, pid);
    }
    else{
      SmallShell::getInstance().setForegroundProcess(this);
      SmallShell::getInstance().setFgPid(pid);
      if(waitpid(pid, nullptr, WUNTRACED) == -1){
        perror("smash error: waitpid failed");
      }
      SmallShell::getInstance().setFgPid(NO_FOREGROUND);
      SmallShell::getInstance().setForegroundProcess(nullptr);
    }
    
  }
}

JobsCommand::JobsCommand(const char* cmd_line, JobsList& jobs): BuiltInCommand(cmd_line), jobs(jobs){}

void JobsCommand::execute() {
  this->jobs.printJobsList();
}

ForegroundCommand::ForegroundCommand(const char* cmd_line, JobsList& jobs): BuiltInCommand(cmd_line), jobs(jobs) {}

static bool is_number(const std::string& s)
{
    std::string::const_iterator it = s.begin();
    while (it != s.end() && std::isdigit(*it)) ++it;
    return !s.empty() && it == s.end();
}

void ForegroundCommand::execute() {
  if(this->args.size() > 2 || (this->args.size() == 2 && !is_number(this->args[1]))){
    perror("smash error: fg: invalid arguments");
    return;
  }
  try{
    // Initalize 
    jid job_id = 0; 
    JobsList::JobEntry job(nullptr, 0, 0);
    if(this->args.size() == 1){
      job = jobs.getLastJob(&job_id);
    }
    else{
      job_id = std::stoi(this->args[1]); 
      job = jobs.getJobById(job_id);
    }
    pid_t pid = job.getJobPid();
    if(kill(pid, SIGCONT) == -1){
      perror("smash error: kill failed");
    }
    jobs.removeJobById(job_id); // remove fg process from job list
    SmallShell::getInstance().setForegroundProcess(this); // update small shell fg fields
    SmallShell::getInstance().setFgPid(pid);
    std::cout << job.getCommand()->getLine(); // print command
    if(job.getCommand()->getIsBg()) {
      std::cout << "&";
    }
    std::cout << " : " << job.getJobPid() << endl;
    this->setLine(job.getCommand()->getLine());
    if(job.getCommand()->getIsBg()){
      this->line.push_back('&');
    }
    if(waitpid(pid, nullptr, WUNTRACED) == -1){ // brings pid process back to fg
      perror("smash error: waitpid failed");
    }
    SmallShell::getInstance().setFgPid(NO_FOREGROUND); // update small shell fg fields
    SmallShell::getInstance().setForegroundProcess(nullptr);
  }
  catch(JobsList::EmptyList& e){
    perror("smash error: fg: jobs list is empty");
  }
  catch(JobsList::JobIdMissing& e) {
    fprintf(stderr, "smash error: fg: job-id %d does not exist\n", std::stoi(this->args[1]));
  }
}

BackgroundCommand::BackgroundCommand(const char* cmd_line, JobsList& jobs): BuiltInCommand(cmd_line), jobs(jobs) {}

void BackgroundCommand::execute() {
  if(this->args.size() > 2 || (this->args.size() == 2 && !is_number(this->args[1]))){
    perror("smash error: bg: invalid arguments");
    return;
  }
  try{
    // Initalize 
    jid job_id = 0; 
    JobsList::JobEntry job(nullptr, 0, 0);
    if(this->args.size() == 1){
      job = jobs.getLastStoppedJob(&job_id);
    }
    else{
      job_id = std::stoi(this->args[1]); 
      job = jobs.getJobById(job_id);
    }
    if(!job.getIsStopped()){
      fprintf(stderr, "smash error: bg: job-id %d is already running in the background\n", job_id);
    }
    pid_t pid = job.getJobPid();
    std::cout << job.getCommand()->getLine(); // print command
    if(job.getCommand()->getIsBg()) {
      std::cout << "&";
    }
    std::cout << " : " << job.getJobPid() << endl;
    jobs.getJobById(job_id).setStopped(false);  // mark command as running
    if(kill(pid, SIGCONT) == -1){
      perror("smash error: kill failed");
    }
  }
  catch(const JobsList::NoStoppedJob& e){
    perror("smash error: bg: there is no stopped jobs to resume");
  }
  catch(const JobsList::JobIdMissing& e) {
    fprintf(stderr, "smash error: bg: job-id %d does not exist\n", std::stoi(this->args[1]));
  }
}



////////////////////////////////************************** jobs implementation

JobsList::JobsList(): job_map() {}

void JobsList::removeFinishedJobs() {
  if(this->job_map.empty()){
    return;
  }
  for(auto it = this->job_map.begin(); it != this->job_map.end(); ) {
    pid_t result = waitpid(it->second.getJobPid(), nullptr, WNOHANG);
    if(result > 0 || result == -1) {
      it = this->job_map.erase(it);
    }
    else {
      ++it;
    }
  }
}

void JobsList::addJob(Command* cmd, pid_t pid, bool isStopped) {
  removeFinishedJobs();
  if(this->job_map.empty()){
    this->job_map.insert(std::make_pair(1, JobsList::JobEntry(cmd, pid, isStopped)));
  }
  else {
    this->job_map.insert(std::make_pair(
      (this->job_map.rbegin()->first)+1,
      JobsList::JobEntry(cmd, pid, isStopped)
    ));
  }
}

void JobsList::printJobsList() {
  removeFinishedJobs();
  for(auto& job: this->job_map) {
    std::cout << "[" << job.first << "]" 
        << job.second.getCommand()->getLine();
    if(job.second.getCommand()->getIsBg()) {
      std::cout << "&";
    }
    std::cout << " : " << job.second.getJobPid() << " "
    << difftime(time(nullptr), job.second.getTimeCreated()) << " secs ";

    if(job.second.getIsStopped()){
      std::cout << "(stopped)";
    }
    std::cout << endl;
  }
}

void JobsList::killAllJobs() {
  removeFinishedJobs();
  for(auto& job: this->job_map) {
    std::cout << job.second.getJobPid() << ": " << job.second.getCommand()->getLine() << endl;
    kill(job.second.getJobPid(), SIGKILL);
  }
}

JobsList::JobEntry& JobsList::getJobById(jid jobId) {
  auto it = this->job_map.find(jobId);
  if(it == this->job_map.end()){
    throw JobIdMissing();
  }
  return it->second;
}

void JobsList::removeJobById(jid jobId) {
  auto it = this->job_map.find(jobId);
  if(it == this->job_map.end()){
    throw(JobIdMissing());
  }
  this->job_map.erase(this->job_map.find(jobId));
}

JobsList::JobEntry& JobsList::getLastJob(jid* lastJobId) {
  if(this->job_map.empty()) {
    throw EmptyList();
  }
  *lastJobId = this->job_map.rbegin()->first;
  return this->job_map.rbegin()->second;
}

JobsList::JobEntry& JobsList::getLastStoppedJob(jid *jobId) {
  jid max_stopped_jid = -1;
  for(auto& job: this->job_map) {
    if(job.first > max_stopped_jid && job.second.getIsStopped()){
      max_stopped_jid = job.first;
    }
  }
  if(max_stopped_jid == -1){throw NoStoppedJob();}
  *jobId = max_stopped_jid;
  return getJobById(max_stopped_jid);
}


////////////////////////////////************************** smash implementation

SmallShell::SmallShell(): prompt("smash"), last_pwd(""), fg_pid(NO_FOREGROUND), jobs_list(), foreground_process(nullptr)
{
// TODO: add your implementation
}

SmallShell::~SmallShell() {
// TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command * SmallShell::CreateCommand(const char* cmd_line) {
  string cmd_s = _trim(string(cmd_line));
  string firstWord = cmd_s.substr(0, cmd_s.find_first_of(" \n"));
  _removeBackgroundSign(firstWord);

  if (firstWord.compare("chprompt") == 0) {
    return new ChangePromptCommand(cmd_line);
  }
  else if(firstWord.compare("showpid") == 0) {
    return new ShowPidCommand(cmd_line);
  }
  else if(firstWord.compare("pwd") == 0) {
    return new GetCurrDirCommand(cmd_line);
  }
  else if(firstWord.compare("cd") == 0) {
    return new ChangeDirCommand(cmd_line, this->last_pwd);
  }
  else if(firstWord.compare("jobs") == 0) {
    return new JobsCommand(cmd_line, SmallShell::getInstance().getJobsList());
  }
  // else if(firstWord.compare("kill") == 0) {
  //   return new KillCommand(cmd_line);
  // }
  else if(firstWord.compare("fg") == 0) {
    return new ForegroundCommand(cmd_line, SmallShell::getInstance().getJobsList());
  }
  else if(firstWord.compare("bg") == 0) {
    return new BackgroundCommand(cmd_line, SmallShell::getInstance().getJobsList());
  }
  // else if(firstWord.compare("quit") == 0) {
  //   return new QuitCommand(cmd_line);
  // }
  else {
    return new ExternalCommand(cmd_line);
  }
  return nullptr;
}

void SmallShell::executeCommand(const char *cmd_line) {
  // TODO: Add your implementation here
  // for example:
  this->jobs_list.removeFinishedJobs();
  Command* cmd = CreateCommand(cmd_line);
  cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}