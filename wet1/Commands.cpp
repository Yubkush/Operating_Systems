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
    // args[i] = (char*)malloc(s.length()+1);
    // memset(args[i], 0, s.length()+1);
    // strcpy(args[i], s.c_str());
    // args[++i] = NULL;
  }

  FUNC_EXIT()
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

// TODO: Add your implementation for classes in Commands.h 
Command::Command(const char* cmd_line) {
  _parseCommandLine(cmd_line, this->args);
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

  std::string line("");
  for(int i = 0; i < this->args.size(); i++) {
    line += (this->args)[i] + " ";
  }

  pid_t pid = fork();
  if(pid == 0) {
    if(setpgrp() == -1){
      perror("smash error: setpgrp failed");
      return;
    }
    // simple external
    if(line.find('*') == std::string::npos && line.find('?') == std::string::npos) {
      
      this->args[0] = "/bin/" + this->args[0];

      std::vector<const char*> command_args;
      for(std::string& arg : this->args) {
        command_args.push_back(arg.c_str());
      }
      command_args.push_back(nullptr);
      
      execv(command_args[0], const_cast<char* const*>(command_args.data()));
      perror("smash error: execv failed");
    }
    // Complex external
    else {
      char arr[line.size()+1];
      strcpy(arr, line.c_str());
      char file[] = "/bin/bash";
      char c[] = "-c";
      char* complex_args[4] = {file, c, arr, nullptr};
      execv("/bin/bash", complex_args);
      perror("smash error: execv failed");
    }
  }
  else if(pid == -1){
    perror("smash error: fork failed");
  }
  else {
    if(waitpid(pid, nullptr, 0) == -1){
      perror("smash error: waitpid failed");
    }
  }
}

SmallShell::SmallShell(): prompt("smash"), last_pwd("") {
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
  // else if(firstWord.compare("jobs") == 0) {
  //   return new JobsCommand(cmd_line);
  // }
  // else if(firstWord.compare("kill") == 0) {
  //   return new KillCommand(cmd_line);
  // }
  // else if(firstWord.compare("fg") == 0) {
  //   return new ForegroundCommand(cmd_line);
  // }
  // else if(firstWord.compare("bg") == 0) {
  //   return new BackgroundCommand(cmd_line);
  // }
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
  Command* cmd = CreateCommand(cmd_line);
  cmd->execute();
  // Please note that you must fork smash process for some commands (e.g., external commands....)
}