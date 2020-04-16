#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <sys/wait.h>
#include <iomanip>
#include "Commands.h"

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#if 0
#define FUNC_ENTRY()  \
  cerr << __PRETTY_FUNCTION__ << " --> " << endl;

#define FUNC_EXIT()  \
  cerr << __PRETTY_FUNCTION__ << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

#define DEBUG_PRINT cerr << "DEBUG: "


#define EXEC(path, arg) \
    execvp((path), (arg));

#define INVALIDATE(errMsg) \
    do {cerr << errMsg << endl; \
    invalid = true; \
    return; \
    } while(0)

const int NO_SETTINGS = 0;

///strip spaces from beginning
///implemented for trim
string _ltrim(const std::string& s)
{
  size_t start = s.find_first_not_of(WHITESPACE);
  return (start == std::string::npos) ? "" : s.substr(start);
}

///strip spaces from end
///implemented for _trim
string _rtrim(const std::string& s)
{
  size_t end = s.find_last_not_of(WHITESPACE);
  return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

//strip spaces from beginning and end
///implemented for parseCommandLine
string _trim(const std::string& s)
{
  return _rtrim(_ltrim(s));
}

int _parseCommandLine(string cmd_line, char** args) {
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

bool _isBackgroundComamnd(basic_string<char, char_traits<char>, allocator<char>> cmd_line) {
      const string str(cmd_line);
      return str[str.find_last_not_of(WHITESPACE)] == '&';
}

string _removeBackgroundSign(string cmd_line) {
      const string str(cmd_line);
      // find last character other than spaces
      unsigned int idx = str.find_last_not_of(WHITESPACE);
      // if all characters are spaces then return
      if (idx == string::npos) {
        return cmd_line;
      }
      // if the command line does not end with & then return
      if (cmd_line[idx] != '&') {
        return cmd_line;
      }
      // replace the & (background sign) with space and then remove all tailing spaces.
      cmd_line[idx] = ' ';
      // truncate the command line string up to the last non-space character
      cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
      return cmd_line;
}

// TODO: Add your implementation for classes in Commands.h 

SmallShell::SmallShell() : jobs(*this) {
    // TODO: add your implementation
}

SmallShell::~SmallShell() {
    // TODO: add your implementation
}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
std::unique_ptr<Command> SmallShell::CreateCommand(string cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    //Special commands

    //Ordinary commands
    if (cmd_s.find("chprompt") == 0) return std::unique_ptr<Command>(new ChpromptCommand(cmd_line, this));
    else if (cmd_s.find("showpid") == 0) return std::unique_ptr<Command>(new ShowPidCommand(cmd_line, this));
    else if (cmd_s.find("pwd") == 0) return std::unique_ptr<Command>(new GetCurrDirCommand(cmd_line, this));
    else if (cmd_s.find("cd") == 0) return std::unique_ptr<Command>(new ChangeDirCommand(cmd_line, this));
    else if (cmd_s.find("jobs") == 0) return std::unique_ptr<Command>(new JobsCommand(cmd_line, this));
    else if (cmd_s.find("kill") == 0) return std::unique_ptr<Command>(new KillCommand(cmd_line, this));
    else if (cmd_s.find("fg") == 0) return std::unique_ptr<Command>(new ForegroundCommand(cmd_line, this));
    else return std::unique_ptr<Command>(new ExternalCommand(cmd_line, this));
}

void SmallShell::executeCommand(string cmd_line) {
    std::unique_ptr<Command> cmd = this -> CreateCommand(cmd_line);
    if (!cmd->invalid) cmd->execute();
    //Please note that you must fork smash process for some commands (e.g., external commands....)
}

const string &SmallShell::getSmashPrompt() const {
    return smashPrompt;
}

void SmallShell::setSmashPrompt(const string &smashPrompt) {
    SmallShell::smashPrompt = smashPrompt;
}

void SmallShell::setLastPwd(const string &lastPwd) {
    SmallShell::lastPwd = lastPwd;
}

const string &SmallShell::getLastPwd() const {
    return lastPwd;
}

/// \param signum 
/// \param jobId 
/// \return success=true, failuer=false
bool SmallShell::sendSignal(signal_t signum, job_id_t jobId) {
    string killCmdText = string("kill -") + std::to_string(signum) + " " + std::to_string(jobId);
    std::unique_ptr<KillCommand> killCmd  = std::unique_ptr<KillCommand>(new KillCommand(killCmdText, this));
    killCmd->verbose = false;
    if (!killCmd->invalid) {
        try{
            killCmd->execute();
        } catch (SmashExceptions::SignalSendException e) {
            return false;
        }
    }
    return true;
}

const ProcessControlBlock *SmallShell::getForegroundProcess() const {
    return foregroundProcess;
}

void SmallShell::setForegroundProcess(const ProcessControlBlock *foregroundProcess) {
    SmallShell::foregroundProcess = foregroundProcess;
}

Command::Command(string cmd_line, SmallShell* smash) : smash(smash), cmd_line(cmd_line) {
    const unsigned int MAX_ARGS = 20;
    char** argsArray = (char**) malloc(MAX_ARGS*sizeof(char*));
    const unsigned short numArgs = _parseCommandLine(_removeBackgroundSign(cmd_line), argsArray);
    args = vector<const std::string> (argsArray, argsArray+numArgs);

    for (unsigned short i=0; i<numArgs; ++i) free(argsArray[i]);
    free(argsArray);
}

void ChpromptCommand::execute() {
    smash->setSmashPrompt(newPrompt + "> ");
}

ChpromptCommand::ChpromptCommand(string cmd_line, SmallShell* smash) :
    BuiltInCommand(cmd_line, smash), newPrompt((args.size()-1 >= 1)? args[1] : "smash") {}

ShowPidCommand::ShowPidCommand(string cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line, smash) {}

void ShowPidCommand::execute() {
    cout << "smash pid is " << getpid() << endl;
}

GetCurrDirCommand::GetCurrDirCommand(string cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line, smash) {}

string GetCurrDirCommand::getCurrDir() {
    char *buf = (char *) malloc(sizeof(char) * PATH_MAX);
    if (!buf) {
        perror("smash error: malloc failed\n");
        return "";
    }
    char *result = getcwd(buf, PATH_MAX);
    if (!result) {
        free(buf);
        perror("smash error: getcwd failed\n");
        return "";
    }
    return result;
}
void GetCurrDirCommand::execute() {
    string currDir = getCurrDir();
    if (currDir != "") cout << currDir << endl;
}

ChangeDirCommand::ChangeDirCommand(string cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line, smash) {}

void ChangeDirCommand::execute() {
    if (args.size() -1 > 1) INVALIDATE("smash error: cd: too many arguments");

    if (args.size() -1 < 1) INVALIDATE("smash error: cd: too few arguments");

    assert(smash);
    if (args[1] == "-" && smash->getLastPwd() == "") INVALIDATE("smash error: cd: OLDPWD not set");

    string oldPath = GetCurrDirCommand::getCurrDir();

    string targetPath = (args[1]=="-")? smash->getLastPwd() : args[1];
    if (!chdir(targetPath.c_str())) {
        smash->setLastPwd(oldPath);
        return;
    }

    INVALIDATE("smash error: chdir failed");
}

JobsCommand::JobsCommand(string cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line, smash) {}

void JobsCommand::execute() {
    smash->jobs.printJobsList();
}

void JobsManager::removeFinishedJobs() {
    for (pair<job_id_t, ProcessControlBlock> job : processes){
        if (!smash.sendSignal(0, job.first)) {
            std::vector<ProcessControlBlock*>::iterator position =
                    find(waitingHeap.begin(), waitingHeap.end(), &job.second);
            if (position != waitingHeap.end()) waitingHeap.erase(position);
            processes.erase(job.first);
        }
    }

    make_heap(waitingHeap.begin(), waitingHeap.end());
}

void JobsManager::printJobsList() {
    removeFinishedJobs();

    for (pair<job_id_t, ProcessControlBlock> job : processes){
        ProcessControlBlock& pcb = job.second;
        cout << "[" << pcb.getJobId() << "] " << pcb
            << " " << difftime(time(nullptr), pcb.getStartTime()) << " secs"
            << ((pcb.isRunning())? "" : " (stopped)") << endl;
    }
}

ProcessControlBlock* JobsManager::getJobById(job_id_t jobId) {
    try{
        return &processes.at(jobId);
    } catch (std::out_of_range) {
        return nullptr;
    }
}

bool JobsManager::isEmpty() {
    return processes.empty();
}

ProcessControlBlock* JobsManager::getLastJob() {
    return &((--processes.end())->second);
}

void JobsManager::removeJobById(job_id_t jobId) {
    //find target job
    ProcessControlBlock* target = getJobById(jobId);
    assert(target);

    //remove from waiting list
    std::vector<ProcessControlBlock*>::iterator position =
            find(waitingHeap.begin(), waitingHeap.end(), target);
    if (waitingHeap.end() != position) waitingHeap.erase(position);
    make_heap(waitingHeap.begin(), waitingHeap.end());
    //remove from map
    processes.erase(jobId);
}

void JobsManager::pauseJob(job_id_t jobId) {
    ProcessControlBlock* pcb = getJobById(jobId);
    assert(pcb);
    pcb->setRunning(false);
    waitingHeap.push_back(pcb);
    push_heap(waitingHeap.begin(), waitingHeap.end());
}

JobsManager::JobsManager(SmallShell &smash) : smash(smash) {}

ProcessControlBlock *JobsManager::getLastStoppedJob() {
    if (waitingHeap.empty()) throw SmashExceptions::NoStoppedJobsException();
    ProcessControlBlock* result = waitingHeap.front();
    assert(result);
    return result;
}

void JobsManager::killAllJobs() {
    cout << "smash: sending SIGKILL to " << processes.size() << "jobs:";
    for (pair<job_id_t, ProcessControlBlock> pcbPair : processes){
        cout << pcbPair.second << endl;
        bool signalStatus = smash.sendSignal(SIGKILL, pcbPair.first);
        assert (signalStatus);
    }
}

void JobsManager::addJob(const Command &cmd, pid_t pid) {
    addJob(ProcessControlBlock(maxIndex, pid, cmd.cmd_line));
}
void JobsManager::addJob(const ProcessControlBlock& pcb){
    removeFinishedJobs();

    const_cast<ProcessControlBlock&>(pcb).setJobId(maxIndex);
    processes.insert(pair<job_id_t,
            ProcessControlBlock>(maxIndex++, pcb));

    //if process is stopped, handle it as such
    if (!pcb.isRunning()){
        pauseJob(pcb.getJobId());
    }
}

KillCommand::KillCommand(string cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line, smash){
    try{
        if ((args.size() -1 != 2) || (args[1][0] != '-')) throw std::invalid_argument("Bad args");
        signum = -stoi(args[1]);
        jobId = stoi(args[2]);
    } catch (std::invalid_argument e) {
        INVALIDATE("smash error: kill: invalid arguments");
    }
}

void KillCommand::execute() {
    ProcessControlBlock* pcbPtr = smash->jobs.getJobById(jobId);
    if (nullptr == pcbPtr) INVALIDATE("smash error: kill: job-id " << jobId << " does not exist");

    if (kill(pcbPtr->getProcessId(), signum)) {
        if (!verbose) throw SmashExceptions::SignalSendException();
        INVALIDATE("smash error: kill failed");
    }

    if (signum == SIGSTOP) smash->jobs.pauseJob(jobId);

    if (verbose) cout << "signal number " << signum << " was sent to pid " << pcbPtr->getProcessId() << endl;
    assert(errno != ESRCH);
}

BuiltInCommand::BuiltInCommand(string cmd_line, SmallShell* smash) : Command(_removeBackgroundSign(cmd_line), smash) {}

ForegroundCommand::ForegroundCommand(string cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line, smash) {
    try{
        if (args.size() - 1 > 1) throw std::invalid_argument("Too many args");
        if (args.size() - 1 != 0) jobId = stoi(args[1]);
    } catch (std::invalid_argument e) {
        INVALIDATE("smash error: fg: invalid arguments");
    }
    if (args.size() - 1 == 0){
        if (smash->jobs.isEmpty()) INVALIDATE("smash error: fg: jobs list is empty");
        jobId = smash->jobs.getLastJob()->getJobId();
    }
    pcb = smash->jobs.getJobById(jobId);
    if (!pcb) INVALIDATE("smash error: fg: job-id " << jobId << " does not exist");
}

void ForegroundCommand::execute() {
    cout << *pcb << endl;

    smash->sendSignal(SIGCONT, jobId);
    pid_t pid = pcb->getProcessId();

    smash->setForegroundProcess(pcb);
    const int waitStatus = waitpid(pid, nullptr, NO_SETTINGS);
    if (waitStatus < 0) INVALIDATE("smash error: waitpid failed");
    smash->setForegroundProcess(nullptr);

    smash->jobs.removeJobById(jobId);
}

BackgroundCommand::BackgroundCommand(string cmd_line, SmallShell *smash) : BuiltInCommand(cmd_line, smash) {
    //sanitize inputs
    //set jobId = args[0] or lastStoppedCommand if none specified
    try{
        if (args.size() - 1 > 1) throw std::invalid_argument("Too many arguments to bg.");
        if (args.size() - 1 == 1) jobId = stoi(args[1]);
        else jobId = smash->jobs.getLastStoppedJob()->getJobId();
    } catch(std::invalid_argument e) {
        INVALIDATE("smash error: bg: invalid arguments");
    } catch (SmashExceptions::NoStoppedJobsException e) {
        INVALIDATE("smash error: bg: there is no stopped jobs to resume");
    }

    //find pcb
    pcb = smash->jobs.getJobById(jobId);
    if (!pcb) INVALIDATE("smash error: bg: job-id " << jobId << " does not exist");

    //if pcb is already running, print to stderr already running
    if (pcb->isRunning()) {
        INVALIDATE("smash error: bg: job-id " << jobId << " is already running in the background");
    }
}

void BackgroundCommand::execute() {
    //send SIGCONT
    bool signalSendStatus = smash->sendSignal(SIGCONT, jobId);
    assert(signalSendStatus);

    pcb->setRunning(true);
}

QuitCommand::QuitCommand(const char *cmd_line, SmallShell *smash) : BuiltInCommand(cmd_line, smash) {
    if (args.size() - 1 ==1 && args[1] == "kill") killRequest = true;
}

void QuitCommand::execute() {
    //kill processes if requested
    if (killRequest) smash->jobs.killAllJobs();

    exit(0);
}

ExternalCommand::ExternalCommand(string cmd_line, SmallShell *smash) : Command(cmd_line, smash),
    backgroundRequest(_isBackgroundComamnd(cmd_line)) {}

void ExternalCommand::execute() {
    //fork a son
    pid_t pid = fork();
    if (pid < 0) INVALIDATE("smash error: fork failed");
    if (pid == 0){
        if (setpgrp() < 0) INVALIDATE("smash error: setpgrp failed");

        execl("/bin/bash", "/bin/bash", "-c", cmd_line.c_str(), NULL);
    }

    else{
        //if !backgroundRequest then wait for son, inform smash that a foreground program is running
        if (!backgroundRequest){
            const job_id_t FG_JOB_ID = 0;
            ProcessControlBlock foregroundPcb = ProcessControlBlock(FG_JOB_ID, pid, cmd_line);
            smash->setForegroundProcess(&foregroundPcb);
            const int waitStatus = waitpid(pid, nullptr, NO_SETTINGS);
            if (waitStatus < 0) INVALIDATE("smash error: waitpid failed");
            smash->setForegroundProcess(nullptr);
        }
        //else add to jobs
        else{
            smash->jobs.addJob(*this, pid);
        }
    }
}


#undef INVALIDATE