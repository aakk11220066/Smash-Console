#include <unistd.h>
#include <string.h>
#include <iostream>
#include <string.h>
#include <vector>
#include <sstream>
#include <memory>
#include <sys/wait.h>
#include <sys/types.h>
#include <iomanip>
#include <algorithm>
//#include <linux/limits.h>
#include <limits.h>
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

#define DEBUG_PRINT(err_msg) cerr << "DEBUG: " << err_msg << endl
#define DIGITS "1234567890"


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
      if (cmd_line=="" || idx == string::npos) {
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



SmallShell::SmallShell() : jobs(*this), smashPid(getpid()) {}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
std::unique_ptr<Command> SmallShell::CreateCommand(string cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    string opcode = cmd_s.substr(0,cmd_s.find_first_of(WHITESPACE));

    //Special commands
    if (cmd_s.find('|') != string::npos) return std::unique_ptr<Command>(new PipeCommand(cmd_line, this));
    else if (cmd_s.find('>') != string::npos)
        return std::unique_ptr<Command>(new RedirectionCommand(cmd_line, this));
    else if (("cp") == opcode) return std::unique_ptr<Command>(new CopyCommand(cmd_line, this));
    else if (("timeout") == opcode) return std::unique_ptr<Command>(new TimeoutCommand(cmd_line, this)); //DEBUG

    //Ordinary commands
    else if (("chprompt") == opcode) return std::unique_ptr<Command>(new ChpromptCommand(cmd_line, this));
    else if (("showpid") == opcode) return std::unique_ptr<Command>(new ShowPidCommand(cmd_line, this));
    else if (("pwd") == opcode) return std::unique_ptr<Command>(new GetCurrDirCommand(cmd_line, this));
    else if (("cd") == opcode) return std::unique_ptr<Command>(new ChangeDirCommand(cmd_line, this));
    else if (("jobs") == opcode) return std::unique_ptr<Command>(new JobsCommand(cmd_line, this));
    else if (("kill") == opcode) return std::unique_ptr<Command>(new KillCommand(cmd_line, this));
    else if (("bg") == opcode) return std::unique_ptr<Command>(new BackgroundCommand(cmd_line, this));
    else if (("fg") == opcode) return std::unique_ptr<Command>(new ForegroundCommand(cmd_line, this));
    else if (("quit") == opcode) return std::unique_ptr<Command>(new QuitCommand(cmd_line, this));
    else return std::unique_ptr<Command>(new ExternalCommand(cmd_line, this));
}

//ROI

ProcessControlBlock* SmallShell::getLateProcess() //ROI
{
    for (ProcessControlBlock *pcb: jobs.timed_processes){
        if (difftime(time(nullptr), pcb->getStartTime()) == pcb->duration){
            //erase timed process from job list
            jobs.removeJobById(pcb->getJobId());
            return pcb;
        }

    }
    return nullptr;
}

void SmallShell::RemoveLateProcess(pid_t pid){
    ProcessControlBlock* target = jobs.getJobById(pid);
    assert(target);
// erase from timed_processes list
    std::list<ProcessControlBlock*>::iterator position =
            find(jobs.timed_processes.begin(), jobs.timed_processes.end(), target);
    if (jobs.timed_processes.end() != position) jobs.timed_processes.erase(position);
// erase from map
    jobs.removeJobById(pid);
}

void SmallShell::executeCommand(string cmd_line) {
    try {
        std::unique_ptr<Command> cmd = this->CreateCommand(cmd_line);
        if (!cmd || !cmd->invalid) cmd->execute();
    } catch (SmashExceptions::Exception& error){
        cerr << error.what() << endl;
    }
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

//ROI
ProcessControlBlock *SmallShell::getForegroundProcess1() const {
    return const_cast<ProcessControlBlock*>(foregroundProcess);
}

void SmallShell::setForegroundProcess(const ProcessControlBlock *foregroundProcess) {
    SmallShell::foregroundProcess = foregroundProcess;
}

SmallShell::~SmallShell() {
    if (foregroundProcess){
        if (kill(foregroundProcess->getProcessId(), SIGKILL)<0) std::cerr <<"smash error: kill failed"<<endl;
    }
    executeCommand("quit");
}

bool SmallShell::getIsForgroundTimed() const {
    return isForgroundTimed;
}

void SmallShell::setIsForgroundTimed(bool value) {
    isForgroundTimed = value;
}


std::vector<std::string> initArgs(string cmd_line){
    const unsigned int MAX_ARGS = 20;
    char** argsArray = (char**) malloc(MAX_ARGS*sizeof(char*));
    const unsigned short numArgs = _parseCommandLine(_removeBackgroundSign(cmd_line), argsArray);
    std::vector<std::string> args = vector<std::string> (argsArray, argsArray+numArgs);

    for (unsigned short i=0; i<numArgs; ++i) free(argsArray[i]);
    free(argsArray);

    return args;
}
Command::Command(string cmd_line, SmallShell* smash) : smash(smash), cmd_line(cmd_line) {
    args = initArgs(cmd_line);
}

void ChpromptCommand::execute() {
    smash->setSmashPrompt(newPrompt + "> ");
}

ChpromptCommand::ChpromptCommand(string cmd_line, SmallShell* smash) :
    BuiltInCommand(cmd_line, smash), newPrompt((args.size()-1 >= 1)? args[1] : "smash") {}

ShowPidCommand::ShowPidCommand(string cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line, smash) {}

void ShowPidCommand::execute() {
    cout << "smash pid is " << smash->smashPid << endl;
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
    string finalResult = result;
    free(buf);
    return finalResult;
}
void GetCurrDirCommand::execute() {
    string currDir = getCurrDir();
    if (currDir != "") cout << currDir << endl;
}

ChangeDirCommand::ChangeDirCommand(string cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line, smash) {} //FIXME: cd ...* throws bad_alloc

void ChangeDirCommand::execute() {
    if (args.size() -1 > 1) throw SmashExceptions::TooManyArgumentsException("cd");

    //TODO: verify that this is the required exception message
    if (args.size() -1 < 1) throw SmashExceptions::TooFewArgumentsException("cd");

    assert(smash);
    if (args[1] == "-" && smash->getLastPwd() == "") {
        throw SmashExceptions::Exception("cd","OLDPWD not set");
    }

    string oldPath = GetCurrDirCommand::getCurrDir();

    string targetPath = (args[1]=="-")? smash->getLastPwd() : args[1];
    if (!chdir(targetPath.c_str())) {
        smash->setLastPwd(oldPath);
        return;
    }

    throw SmashExceptions::SyscallException("chdir");
}

JobsCommand::JobsCommand(string cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line, smash) {}

void JobsCommand::execute() {
    smash->jobs.printJobsList();
}

void JobsManager::removeFinishedJobs() {
    std::list<job_id_t> targets;
    for (pair<job_id_t, ProcessControlBlock> job : processes){
        int waitPidStatus = waitpid(job.second.getProcessId(), nullptr, WNOHANG);
        int killStatus = kill(job.second.getProcessId(), 0);
        if (killStatus<0 && errno==3) {
            targets.push_back(job.first);
        }
        else if (killStatus < 0) {
            DEBUG_PRINT("removeFinishedJobs kill failed with errno="<<errno);
            throw SmashExceptions::SyscallException("kill");
        }
    }
    for (job_id_t jobId : targets){
        ProcessControlBlock* targetPCB = &processes.at(jobId);
        std::vector<ProcessControlBlock*>::iterator position =
                find(waitingHeap.begin(), waitingHeap.end(), targetPCB);
        if (position != waitingHeap.end()) waitingHeap.erase(position);
        processes.erase(jobId);
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
    cout << "smash: sending SIGKILL to " << processes.size() << " jobs:"<<endl;
    for (pair<job_id_t, ProcessControlBlock> pcbPair : processes){
        cout << pcbPair.second << endl;
        bool signalStatus = smash.sendSignal(SIGKILL, pcbPair.first);
        assert (signalStatus>=0);
    }
}

void JobsManager::addJob(const Command &cmd, pid_t pid, int duration) {
    addJob(ProcessControlBlock(maxIndex, pid, cmd.cmd_line, duration));
}
void JobsManager::addJob(const ProcessControlBlock& pcb){
    removeFinishedJobs();

    resetMaxIndex();
    const_cast<ProcessControlBlock&>(pcb).resetStartTime();

    const_cast<ProcessControlBlock&>(pcb).setJobId(++maxIndex);
    processes.insert(pair<job_id_t,
            ProcessControlBlock>(maxIndex, pcb));

    //if process is stopped, handle it as such
    if (!pcb.isRunning()){
        pauseJob(pcb.getJobId());
    }
}

job_id_t JobsManager::resetMaxIndex() {
    DEBUG_PRINT("resetMaxIndex called, currently maxIndex="<<maxIndex);
    while (maxIndex>0 && !getJobById(maxIndex)){
        --maxIndex;
    }
    return maxIndex;
}

KillCommand::KillCommand(string cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line, smash){
    try{
        if ((args.size() -1 != 2) || (args[1][0] != '-')) throw std::invalid_argument("Bad args");
        signum = -stoi(args[1]);
        jobId = stoi(args[2]);
    } catch (std::invalid_argument e) {
        throw SmashExceptions::InvalidArgumentsException("kill");
    }
}

void KillCommand::execute() {
    ProcessControlBlock* pcbPtr = smash->jobs.getJobById(jobId);
    if (nullptr == pcbPtr) {
        throw SmashExceptions::Exception("kill","job-id "+to_string(jobId)+" does not exist");
    }

    int signalSendStatus = kill(pcbPtr->getProcessId(), signum);
    if (signalSendStatus) {
        if (!verbose) throw SmashExceptions::SignalSendException();
        DEBUG_PRINT("Kill command failed");
        throw SmashExceptions::SyscallException("kill");
    }

    if (signum == SIGSTOP) smash->jobs.pauseJob(jobId);

    if (verbose) cout << "signal number " << signum << " was sent to pid " << pcbPtr->getProcessId() << endl;
}

BuiltInCommand::BuiltInCommand(string cmd_line, SmallShell* smash) : Command(_removeBackgroundSign(cmd_line), smash) {}

//TODO: test fg with no arguments (check that maximal jobId selected)
//TODO: test fg with stopped process
//TODO: test to make sure fg removes job from jobs list
ForegroundCommand::ForegroundCommand(string cmd_line, SmallShell* smash) : BuiltInCommand(cmd_line, smash) {
    try{
        if (args.size() - 1 > 1) throw std::invalid_argument("Too many args");
        if (args.size() - 1 != 0) jobId = stoi(args[1]);
    } catch (std::invalid_argument e) {
        throw SmashExceptions::InvalidArgumentsException("fg");
    }
    if (args.size() - 1 == 0){
        if (smash->jobs.isEmpty()) throw SmashExceptions::Exception("fg", "jobs list is empty");
        jobId = smash->jobs.getLastJob()->getJobId();
    }
    pcb = smash->jobs.getJobById(jobId);
    if (!pcb) throw SmashExceptions::Exception("fg", "job-id " + to_string(jobId) + " does not exist");
}

void ForegroundCommand::execute() {
    cout << *pcb << endl;

    smash->sendSignal(SIGCONT, jobId);
    pid_t pid = pcb->getProcessId();

    smash->setForegroundProcess(pcb);
    const int waitStatus = waitpid(pid, nullptr, NO_SETTINGS);
    if (waitStatus < 0) throw SmashExceptions::SyscallException("waitpid");
    smash->setForegroundProcess(nullptr);

    smash->jobs.removeJobById(jobId);
    // ROI - loop to remove timed process in case it ended before the timeout
    for (auto it = smash->jobs.timed_processes.begin(); it != smash->jobs.timed_processes.end(); ++it){
        if (!*it) continue; //AKIVA - to prevent segmentation faults
        if (jobId == (*it)->getJobId()) smash->jobs.timed_processes.erase(it);
    }
}

//TODO: make bg return "there is no stopped jobs to resume" when all of background is running and no arguments given (management of waitingHeap faulty) - lines that add/remove from the heap are 325, 367, 377
BackgroundCommand::BackgroundCommand(string cmd_line, SmallShell *smash) : BuiltInCommand(cmd_line, smash) {
    //sanitize inputs
    //set jobId = args[0] or lastStoppedCommand if none specified
    try{
        if (args.size() - 1 > 1) throw std::invalid_argument("Too many arguments to bg.");
        if (args.size() - 1 == 1) jobId = stoi(args[1]);
        else jobId = smash->jobs.getLastStoppedJob()->getJobId();
    } catch(std::invalid_argument e) {
        throw SmashExceptions::InvalidArgumentsException("bg");
    } catch (SmashExceptions::NoStoppedJobsException e) {
        throw SmashExceptions::Exception("bg", "there is no stopped jobs to resume");
    }

    //find pcb
    pcb = smash->jobs.getJobById(jobId);
    if (!pcb) throw SmashExceptions::Exception("bg","job-id " + to_string(jobId) + " does not exist");

    //if pcb is already running, print to stderr already running
    if (pcb->isRunning()) {
        throw SmashExceptions::Exception("bg","job-id " + to_string(jobId) + " is already running in the background");
    }
}

void BackgroundCommand::execute() {
    //send SIGCONT
    bool signalSendStatus = smash->sendSignal(SIGCONT, jobId);
    assert(signalSendStatus);

    pcb->setRunning(true);
}

QuitCommand::QuitCommand(string cmd_line, SmallShell *smash) : BuiltInCommand(cmd_line, smash) {
    if (args.size() - 1 ==1 && args[1] == "kill") killRequest = true;
}

void QuitCommand::execute() {
    //kill processes if requested
    if (killRequest) smash->jobs.killAllJobs();

    exit(0);
}

BackgroundableCommand::BackgroundableCommand(string cmd_line, SmallShell *smash) : Command(cmd_line, smash),
       backgroundRequest(_isBackgroundComamnd(cmd_line)) {}

void BackgroundableCommand::execute() {
    //fork a son
    pid = fork();
    if (pid < 0) throw SmashExceptions::SyscallException("fork");
    if (pid == 0){
        DEBUG_PRINT("Forked a son for backgroundablecommand "<<cmd_line<<" with pid="<<getpid());
        if (setpgrp() < 0) throw SmashExceptions::SyscallException("setpgrp");

        executeBackgroundable();
        exit(0);
    }

    else{
        //if !backgroundRequest then wait for son, inform smash that a foreground program is running
        if (!backgroundRequest){
            const job_id_t FG_JOB_ID = 0;
            ProcessControlBlock foregroundPcb = ProcessControlBlock(FG_JOB_ID, pid, cmd_line);
            smash->setForegroundProcess(&foregroundPcb);
            const int waitStatus = waitpid(pid, nullptr, WUNTRACED);
            if (waitStatus < 0) {
                DEBUG_PRINT("backgroundablecommand waitpid failed with pid="<<pid<<", waitStatus="<<waitStatus<<", and errno="<<errno);
                throw SmashExceptions::SyscallException("waitpid");
            }
            smash->setForegroundProcess(nullptr);
        }
        //else add to jobs
        else{
            smash->jobs.addJob(*this, pid);
            ProcessControlBlock* target = smash->jobs.getJobById(pid);
            smash->jobs.timed_processes.push_back(target);

        }
    }
}


//TODO: pipe command should redirect signals to children
PipeCommand::PipeCommand(std::string cmd_line, SmallShell *smash) : BackgroundableCommand(cmd_line, smash) {
    //TODO: verify that if not given 2 commands then need to invalidate
    //TODO: verify that if one of the commands fails need to return error of that command
    unsigned short pipeIndex = cmd_line.find_first_of('|');
    //sanitize inputs
    if (!(cmd_line.size() > pipeIndex+1)) throw SmashExceptions::InvalidArgumentsException("pipe");

    if (cmd_line[pipeIndex+1] == '&') errPipe = true;

    const string cmd_lineFrom = cmd_line.substr(0, pipeIndex);
    const string cmd_lineTo = cmd_line.substr(pipeIndex+1+(errPipe? 1:0));

    /*bool backgroundCommand = _isBackgroundComamnd(cmd_line);

    commandFrom = smash->CreateCommand(cmd_lineFrom + (backgroundCommand? "&" : ""));
    commandTo = smash->CreateCommand(cmd_lineTo + (backgroundCommand? "&" : ""));*/

    commandFrom = smash->CreateCommand(cmd_lineFrom);
    commandTo = smash->CreateCommand(cmd_lineTo);
    assert(commandTo && commandFrom);
    invalid = commandFrom->invalid && commandTo->invalid;
}

PipeCommand::PipeCommand(std::unique_ptr<Command> commandFrom, std::unique_ptr<Command> commandTo, SmallShell* smash) :
        BackgroundableCommand(cmd_line, smash), commandFrom(std::move(commandFrom)), commandTo(std::move(commandTo)){}

void PipeCommand::executeBackgroundable() {
    int pipeSides[2];
    if (pipe(pipeSides)) throw SmashExceptions::SyscallException("pipe");

    if ((pidFrom = fork()) < 0) throw SmashExceptions::SyscallException("fork");
    if (!pidFrom){
        //close pipe read side
        if (close(pipeSides[0])) throw SmashExceptions::SyscallException("close");
        //replace stdout with pipe write side
        if (dup2(pipeSides[1], errPipe? STDERR_FILENO : STDOUT_FILENO)<0) throw SmashExceptions::SyscallException("dup2");
        //execute commandFrom //TODO: test if command is showpid, needs to print original smash pid
        commandFrom->execute();
        exit(0);
    }
    else{
        if ((pidTo = fork()) < 0) throw SmashExceptions::SyscallException("fork");
        if (!pidTo){
            //close pipe write side
            if (close(pipeSides[1])) throw SmashExceptions::SyscallException("close");
            //replace stdin with pipe read side
            if (dup2(pipeSides[0], STDIN_FILENO)<0) throw SmashExceptions::SyscallException("dup2");
            //execute commandTo
            commandTo->execute();
            exit(0);
        }
        else{
            //parent
            if (close(pipeSides[0]) || close(pipeSides[1])) throw SmashExceptions::SyscallException("close");

            wait(nullptr);
            wait(nullptr);
            pidFrom = pidTo = -1;
        }
    }
}

PipeCommand::~PipeCommand() {
    if (pidFrom != -1) {
        if (kill(pidFrom, SIGKILL) < 0) {
            DEBUG_PRINT("pidFrom = "<<pidFrom<<" and kill failed with errno = "<<errno);
            cerr << "smash error: kill failed" << endl;
        }
    }
    if (pidTo != -1) {
        if (kill(pidTo, SIGKILL) < 0) {
            DEBUG_PRINT("pidTo = "<<pidFrom<<" and kill failed with errno = "<<errno);
            cerr << "smash error: kill failed" << endl;        }
    }
}

unsigned short indicator(bool condition){
    return condition? 1 : 0;
}

//TODO: RedirectionCommand should not fork first command if first command is builtin?
RedirectionCommand::RedirectionCommand(unique_ptr<Command> commandFrom, string filename, bool append, SmallShell *smash) try :
    PipeCommand(std::move(commandFrom), unique_ptr<Command>(new WriteCommand(filename, append, smash)), smash) {}
    catch (SmashExceptions::FileOpenException& e){
        DEBUG_PRINT("Failed to open file "<<filename<<".  errno = "<<errno);
        throw SmashExceptions::InvalidArgumentsException("redirection");
    }

#define OPERATOR_POSITION (cmd_line.find_first_of('>'))
RedirectionCommand::RedirectionCommand(std::string cmd_line, SmallShell *smash) :
        RedirectionCommand(smash->CreateCommand(cmd_line.substr(0,OPERATOR_POSITION)),
                           _trim(cmd_line.substr(OPERATOR_POSITION+1
                                +indicator(cmd_line.at(1+OPERATOR_POSITION) == '>'))),
                           cmd_line.at(1+OPERATOR_POSITION) == '>',
                    sanitizeInputs(smash)) {}
#undef OPERATOR_POSITION


void RedirectionCommand::execute() {
    PipeCommand::execute();
}

SmallShell *RedirectionCommand::sanitizeInputs(SmallShell *smash) {
    operatorPosition = cmd_line.find_first_of('>');
    DEBUG_PRINT("operatorPosition="<<operatorPosition);
    //ensure a string representing a filename was specified (validation that it is a filename in redirected ctor)
    if (cmd_line.size() > operatorPosition+1) {
        DEBUG_PRINT("cmd_line.size() > operatorPosition+1 was not fulfilled because cmd_line.size()="<<cmd_line.size()<<" and operatorPosition+1="<<operatorPosition+1);
        throw SmashExceptions::InvalidArgumentsException("redirection");
    }
    return smash;
}

RedirectionCommand::WriteCommand::WriteCommand(string fileName, bool append, SmallShell* smash) :
    Command("write_into "+fileName, smash){

    sink.open(fileName, append? std::ofstream::app : std::ofstream::trunc);
    if (sink.failbit & sink.rdstate()) throw SmashExceptions::FileOpenException();
}

void RedirectionCommand::WriteCommand::execute() {
    char buf;
    int readStatus;
    while ((readStatus=read(STDIN_FILENO, &buf, 1)) > 0) {
        sink.write(&buf, 1);
        sink.flush();
    }
}

RedirectionCommand::WriteCommand::~WriteCommand() {
    sink.close();
}

CopyCommand::CopyCommand(string cmd_line, SmallShell *smash) try :
    RedirectionCommand(unique_ptr<Command>(
        new ReadCommand(initArgs(cmd_line).at(1), smash)),
        initArgs(cmd_line).at(2),
        false,
        smash) {

        this->cmd_line = cmd_line;
        args = initArgs(cmd_line);
        //TODO: verify that if copy does not receive 2 arguments need to throw invalid arguments
        if (args.size() -1 != 2) throw SmashExceptions::InvalidArgumentsException("cp");
        backgroundRequest = _isBackgroundComamnd(cmd_line);
    } //TODO: verify that if input is not 2 filepaths then need to invalidate
    catch (SmashExceptions::FileOpenException& e){
        throw SmashExceptions::InvalidArgumentsException("cp");
    }


void CopyCommand::execute() {
    RedirectionCommand::execute();
}

CopyCommand::ReadCommand::ReadCommand(string fileName, SmallShell *smash) :
    Command("read_from "+fileName, smash) {

    source.open(fileName);
    if (source.failbit & source.rdstate()) throw SmashExceptions::FileOpenException();
}


void CopyCommand::ReadCommand::execute() {
    for (char nextLetter = source.get(); source.good() && nextLetter != EOF; nextLetter=source.get()){
        cout<<nextLetter;
    }
    cout.flush();
}

CopyCommand::ReadCommand::~ReadCommand() {
    source.close();
}
// ROI - timeout command

// to check - do we need to handle a scenario (error) where the inner command is a built-in command ??
TimeoutCommand::TimeoutCommand(string cmd_line, SmallShell *smash) : Command(cmd_line, smash),
backgroundRequest(_isBackgroundComamnd(cmd_line)) {
    // trimming 1st word, saving number and saving the rest of the command
    string trimmed_cmd = _trim(cmd_line);
    unsigned short timeout_index = trimmed_cmd.find_first_of("timeout");
    if (timeout_index != 0) throw SmashExceptions::InvalidArgumentsException("timeout");
    string no_timeout_cmd = trimmed_cmd.substr(7, trimmed_cmd.length()+1);
    //string trimmed_no_timeout_cmd = _trim(no_timeout_cmd);
    unsigned short digits_index = _trim(no_timeout_cmd).find_first_of(DIGITS);
    if (digits_index != 0) throw SmashExceptions::InvalidArgumentsException("timeout");
    unsigned short digits_end_index = _trim(no_timeout_cmd).find_first_of(' ');
    string str_number = _trim(no_timeout_cmd).substr(digits_index, digits_end_index);
    if (str_number.find_first_not_of(DIGITS) != std::string::npos) throw SmashExceptions::InvalidArgumentsException("timeout");
    inner_cmd_line = trimmed_cmd.substr(trimmed_cmd.find_first_of(str_number) + str_number.length()+1, trimmed_cmd.length()+1);


    //innerCommand = smash->CreateCommand(inner_cmd_line);
    cout << inner_cmd_line << endl; //DEBUG
    waitNumber = stoi(str_number);

}

void TimeoutCommand::execute() {
    //fork a son
    pid_t pid = fork();
    if (pid < 0) throw SmashExceptions::SyscallException("fork");
    if (pid == 0){
        if (setpgrp() < 0) throw SmashExceptions::SyscallException("setpgrp");
        execl("/bin/bash", "/bin/bash", "-c", _removeBackgroundSign(inner_cmd_line).c_str(), NULL);
        exit(0);
    }

    else{
        // set the alarm signal
        alarm(waitNumber);
        //if !backgroundRequest then wait for son, inform smash that a foreground program is running
        if (!backgroundRequest){
            const job_id_t FG_JOB_ID = 0;
            ProcessControlBlock foregroundPcb = ProcessControlBlock(FG_JOB_ID, pid, cmd_line, waitNumber);
            smash->setForegroundProcess(&foregroundPcb);
            ProcessControlBlock* timed_pcb = smash->getForegroundProcess1();
            smash->setIsForgroundTimed(true);
            //smash->jobs.timed_processes.push_back(timed_pcb);
            const int waitStatus = waitpid(pid, nullptr, WUNTRACED);
            if (waitStatus < 0) throw SmashExceptions::SyscallException("waitpid");
            smash->setIsForgroundTimed(false);
            smash->setForegroundProcess(nullptr);
        }
            //else add to jobs
        else{
            smash->jobs.addJob(*this, pid);
        }
    }
}

ExternalCommand::ExternalCommand(string cmd_line, SmallShell *smash) : BackgroundableCommand(cmd_line, smash) {}

void ExternalCommand::executeBackgroundable() {
    execl("/bin/bash", "/bin/bash", "-c", _removeBackgroundSign(cmd_line).c_str(), NULL);
}

