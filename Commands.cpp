#include <unistd.h>
#include <string.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <memory>
#include <sys/wait.h>
#include <sys/types.h>
#include <iomanip>
#include <algorithm>
#include <limits.h>
#include <type_traits>
#include <stdio.h>
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

//debug

#ifndef NDEBUG
#define DEBUG_PRINT(err_msg) cerr << "DEBUG: " << err_msg << endl
#else
#define DEBUG_PRINT(err_msg)
#endif

#define DIGITS "1234567890"
#define BuiltInID -2
const job_id_t UNINITIALIZED_JOB_ID=-1;
const job_id_t FG_JOB_ID = 0;
const int NO_OPTIONS = 0;

/// USE THIS WHEN SENDING ORDERS TO PROCESSES THAT SHOULD AFFECT PROCESS'S CHILDREN!
/// \param pcb process control block representing process to send signal to
/// \param sig_num signal number to send
/// \param errCodeReturned where to return error code to [optional]
/// \return true if succeeded, false if failed to send signal

bool sendSignal(const ProcessControlBlock& pcb, signal_t sig_num, errno_t* errCodeReturned) {
    bool result = (killpg(pcb.getProcessGroupId(), sig_num) >= 0);
    DEBUG_PRINT("sendSignal sending signal "<<sig_num<<" to process "<<pcb.getCreatingCommand());
    if (errCodeReturned) *errCodeReturned = errno;
    return result;
}


template<>
ProcessControlBlock* Heap<ProcessControlBlock*>::getMax() {
    if (empty()) throw SmashExceptions::NoStoppedJobsException();

    ProcessControlBlock *result = this->front();
    for (ProcessControlBlock *elem : *this) if (*elem > *result) result = elem;
    return result;
}

template<class T>
void Heap<T>::insert(T &newElement) {
    std::vector<T>::push_back(newElement);
    push_heap(this->begin(), this->end());
}

template<class T>
bool Heap<T>::empty() {
    return std::vector<T>::empty();
}

template<class T>
void Heap<T>::erase(const T &target) {
    std::vector<ProcessControlBlock *>::iterator position =
            find(this->begin(), this->end(), target);
    if (position != this->end()) std::vector<T>::erase(position);

    make_heap(this->begin(), this->end());
}


///strip spaces from beginning
///implemented for trim
string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

///strip spaces from end
///implemented for _trim
string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

//strip spaces from beginning and end
///implemented for parseCommandLine
string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(string cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
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
    size_t idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    //if (cmd_line == "" || idx == string::npos) {
    if (cmd_line == "" || idx == string::npos) {
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



SmallShell::SmallShell() : smashProcessGroup(getpgrp()), smashPid(getpid()), jobs(*this) {}

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
std::unique_ptr<Command> SmallShell::CreateCommand(string cmd_line) {
    string cmd_s = _trim(string(cmd_line));
    string opcode = _trim(_removeBackgroundSign(cmd_s.substr(0, cmd_s.find_first_of(WHITESPACE))));

    //Special commands
    if ((cmd_s.find('|') != string::npos) && (("chprompt") != opcode))
        return std::unique_ptr<Command>(new PipeCommand(cmd_line, this));
    else if ((cmd_s.find('>') != string::npos) && (("chprompt") != opcode))
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

TimedProcessControlBlock *SmallShell::getLateProcess() //ROI
{

    for (TimedProcessControlBlock& timed_pcb: jobs.timed_processes) {

        if (difftime(time(nullptr), timed_pcb.getAbortTime()) == 0) {
            pid_t pid = timed_pcb.getProcessId();
            // in case of built-in command
            if (pid == BuiltInID) return &timed_pcb;

            // in case background command already finished
            if (waitpid(timed_pcb.getProcessId(), nullptr, WNOHANG)<0) {
                throw SmashExceptions::SyscallException("waitpid");
            }
            int killStatus = kill(pid, 0);
            if (killStatus < 0 && errno == ESRCH) {
                return nullptr;
                }
            }
        //background command un-finished
            return &timed_pcb;
        }
    return nullptr;
}

void SmallShell::RemoveLateProcesses() {
    //remove from list all jobs who have just expired
    for (TimedProcessControlBlock timed_pcb: jobs.timed_processes) {
        //cout << pid << endl << pcb->getProcessId() << endl;
        if (difftime(time(nullptr), timed_pcb.getAbortTime()) == 0) {
            jobs.timed_processes.remove(timed_pcb);
            // erase from map
            jobs.removeJobById(timed_pcb.getJobId());
        }
    }
    //sort list after deletion for the next alarm signal
    jobs.timed_processes.sort();
}


void SmallShell::executeCommand(string cmd_line) {
    try {
        std::unique_ptr<Command> cmd = this->CreateCommand(cmd_line);
        if (!cmd || !cmd->invalid) cmd->execute();
    }
    catch (SmashExceptions::SameFileException& e) {}
    catch (SmashExceptions::SyscallException& error){
        std::perror(error.what());
        fflush(stderr);
    }
    catch (SmashExceptions::Exception &error) {
        cerr << error.what() << endl;
        cerr.flush();
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
    std::unique_ptr<KillCommand> killCmd = std::unique_ptr<KillCommand>(new KillCommand(killCmdText, this));
    killCmd->verbose = false;
    if (!killCmd->invalid) {
        try {
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
    return const_cast<ProcessControlBlock *>(foregroundProcess);
}

void SmallShell::setForegroundProcess(const ProcessControlBlock *foregroundProcess) {
    SmallShell::foregroundProcess = foregroundProcess;
}

SmallShell::~SmallShell() {
    if (foregroundProcess) {
        if (!::sendSignal(*foregroundProcess, SIGKILL)) std::cerr << "smash error: kill failed" << endl;
    }
    executeCommand("quit");
}
/*
bool SmallShell::getIsForgroundTimed() const {
    return isForgroundTimed;
}

void SmallShell::setIsForgroundTimed(bool value) {
    isForgroundTimed = value;
}

bool SmallShell::getHasProcessTimedOut() const{
    return hasProcessTimedOut;
}

void SmallShell::setHasProcessTimedOut(bool value){
    hasProcessTimedOut = value;
}
*/
signal_t SmallShell::escapeSmashProcessGroup() {
    if (getpgrp() == smashProcessGroup){ //only escape smash process group
        if (setpgrp() < 0) throw SmashExceptions::SyscallException("setpgrp");
    }
    return getpgrp();
}


std::vector<std::string> initArgs(string cmd_line) {
    const unsigned int MAX_ARGS = 20;
    char **argsArray = (char **) malloc(MAX_ARGS * sizeof(char *));
    const unsigned short numArgs = _parseCommandLine(_removeBackgroundSign(cmd_line), argsArray);
    std::vector<std::string> args = vector<std::string>(argsArray, argsArray + numArgs);

    for (unsigned short i = 0; i < numArgs; ++i) free(argsArray[i]);
    free(argsArray);

    return args;
}

Command::Command(string cmd_line, SmallShell *smash) : smash(smash), cmd_line(cmd_line) {
    args = initArgs(cmd_line);
}

void ChpromptCommand::execute() {
    smash->setSmashPrompt(newPrompt + "> ");
}

ChpromptCommand::ChpromptCommand(string cmd_line, SmallShell *smash) :
        BuiltInCommand(cmd_line, smash), newPrompt((args.size() - 1 >= 1) ? args[1] : "smash") {}

ShowPidCommand::ShowPidCommand(string cmd_line, SmallShell *smash) : BuiltInCommand(cmd_line, smash) {}

void ShowPidCommand::execute() {
    cout << "smash pid is " << smash->smashPid << endl;
}

GetCurrDirCommand::GetCurrDirCommand(string cmd_line, SmallShell *smash) : BuiltInCommand(cmd_line, smash) {}

string GetCurrDirCommand::getCurrDir() {
    char *buf = (char *) malloc(sizeof(char) * (1+PATH_MAX));
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

ChangeDirCommand::ChangeDirCommand(string cmd_line, SmallShell *smash) : BuiltInCommand(cmd_line,
                                                                                        smash) {}

void ChangeDirCommand::execute() {
    if (args.size() - 1 > 1) throw SmashExceptions::TooManyArgumentsException("cd");

    if (args.size() - 1 < 1) return;

    assert(smash);
    if (args[1] == "-" && smash->getLastPwd() == "") {
        throw SmashExceptions::Exception("cd", "OLDPWD not set");
    }

    string oldPath = GetCurrDirCommand::getCurrDir();

    string targetPath = (args[1] == "-") ? smash->getLastPwd() : args[1];
    if (!chdir(targetPath.c_str())) {
        smash->setLastPwd(oldPath);
        return;
    }

    throw SmashExceptions::SyscallException("chdir");
}

JobsCommand::JobsCommand(string cmd_line, SmallShell *smash) : BuiltInCommand(cmd_line, smash) {}

void JobsCommand::execute() {
    smash->jobs.printJobsList();
}

void JobsManager::removeFinishedJobs() {
    std::list<job_id_t> targets;
    for (pair<job_id_t, ProcessControlBlock> job : processes) {
        if (waitpid(job.second.getProcessId(), nullptr, WNOHANG | WUNTRACED)<0) throw SmashExceptions::SyscallException("waitpid");
        int killStatus = kill(job.second.getProcessId(), 0);
        if (killStatus < 0 && errno == 3) {
            targets.push_back(job.first);
        } else if (killStatus < 0) {
            throw SmashExceptions::SyscallException("kill");
        }
    }
    for (job_id_t jobId : targets) {
        waitingHeap.erase(&processes.at(jobId));
        processes.erase(jobId);
    }
}

void JobsManager::printJobsList() {
    removeFinishedJobs();

    for (pair<job_id_t, ProcessControlBlock> job : processes) {
        ProcessControlBlock &pcb = job.second;
        cout << "[" << pcb.getJobId() << "] " << pcb
             << " " << difftime(time(nullptr), pcb.getStartTime()) << " secs"
             << ((pcb.isRunning()) ? "" : " (stopped)") << endl;
    }
}

ProcessControlBlock *JobsManager::getJobById(job_id_t jobId) {
    try {
        return &processes.at(jobId);
    } catch (std::out_of_range) {
        return nullptr;
    }
}

bool JobsManager::isEmpty() {
    return processes.empty();
}

ProcessControlBlock *JobsManager::getLastJob() {
    return &((--processes.end())->second);
}

void JobsManager::removeJobById(job_id_t jobId) {
    //remove from waiting list
    waitingHeap.erase(&processes.at(jobId));
    //remove from map
    processes.erase(jobId);

    /*
    // remove from timed_processes list
    for (ProcessControlBlock* pcb : smash.jobs.timed_processes){
        if (pcb->getJobId() == jobId) timed_processes.remove(pcb);
    }
     */

}

void JobsManager::pauseJob(job_id_t jobId) {
    ProcessControlBlock *pcb = getJobById(jobId);
    assert(pcb);
    pcb->setRunning(false);
    waitingHeap.insert(pcb);
}

void JobsManager::unpauseJob(job_id_t jobId) {
    //send SIGCONT
    bool signalSendStatus = smash.sendSignal(SIGCONT, jobId);
    assert(signalSendStatus);

    registerUnpauseJob(jobId);
}
void JobsManager::registerUnpauseJob(job_id_t jobId) {
    ProcessControlBlock *pcb = getJobById(jobId);
    assert(pcb);
    //inform process manager that it is running now
    pcb->setRunning(true);

    //remove from waiting list
    waitingHeap.erase(pcb);
}

JobsManager::JobsManager(SmallShell &smash) : smash(smash) {}

ProcessControlBlock *JobsManager::getLastStoppedJob() {
    if (waitingHeap.empty()) throw SmashExceptions::NoStoppedJobsException();
    ProcessControlBlock *result = waitingHeap.getMax();
    assert(result);
    return result;
}

void JobsManager::killAllJobs() {
    cout << "smash: sending SIGKILL signal to " << processes.size() << " jobs:" << endl;
    for (pair<job_id_t, ProcessControlBlock> pcbPair : processes) {
        cout << pcbPair.second.getProcessId() << ": " << pcbPair.second.getCreatingCommand() << endl;
        bool signalStatus = smash.sendSignal(SIGKILL, pcbPair.first);
        assert (signalStatus);
    }
    // ROI erase also all timed processes
    smash.jobs.timed_processes.clear();
}

void JobsManager::addJob(const Command &cmd, pid_t pid) {
    addJob(ProcessControlBlock(UNINITIALIZED_JOB_ID, pid, cmd.cmd_line));
}

void JobsManager::addJob(const ProcessControlBlock &pcb) {
    removeFinishedJobs();

    resetMaxIndex();
    const_cast<ProcessControlBlock &>(pcb).resetStartTime();

    job_id_t newJobId = pcb.getJobId();
    if (newJobId == UNINITIALIZED_JOB_ID || newJobId==FG_JOB_ID) newJobId = ++maxIndex;
    const_cast<ProcessControlBlock &>(pcb).setJobId(newJobId);
    processes.erase(newJobId); //new element should overwrite old element
    processes.insert(pair<job_id_t,
            ProcessControlBlock>(newJobId, pcb));

    //if process is stopped, handle it as such
    if (!pcb.isRunning()) {
        pauseJob(pcb.getJobId());
    }
}

job_id_t JobsManager::resetMaxIndex() {
    maxIndex += 2; //for safety
    while (maxIndex > 0 && !getJobById(maxIndex)) {
        --maxIndex;
    }
    return maxIndex;
}

void JobsManager::addTimedProcess(const job_id_t jobId,
                                  const pid_t processId,
                                  const std::string& creatingCommand, int futureSeconds){
    TimedProcessControlBlock timed_pcb = TimedProcessControlBlock(jobId, processId, creatingCommand, futureSeconds);
    timed_processes.push_front(timed_pcb);
    //sort processes by futureTime
    timed_processes.sort();
}

void JobsManager::setAlarmSignal() const{
    assert(!timed_processes.empty());
    int alarmNumber = (int)difftime(timed_processes.begin()->getAbortTime(),time(nullptr));
    assert(alarmNumber > 0);
    alarm(alarmNumber);
}

KillCommand::KillCommand(string cmd_line, SmallShell *smash) : BuiltInCommand(cmd_line, smash) {
    try {
        if ((args.size() - 1 != 2) || (args[1][0] != '-')) throw std::invalid_argument("Bad args");
        signum = -stoi(args[1]);
        jobId = stoi(args[2]);
        if (signum>31) throw SmashExceptions::InvalidArgumentsException("kill");
    } catch (std::invalid_argument e) {
        throw SmashExceptions::InvalidArgumentsException("kill");
    }
}

void KillCommand::execute() {
    ProcessControlBlock *pcbPtr = smash->jobs.getJobById(jobId);
    if (nullptr == pcbPtr) {
        throw SmashExceptions::Exception("kill", "job-id " + to_string(jobId) + " does not exist");
    }

    bool signalSendStatus = ::sendSignal(*pcbPtr, signum);
    if (!signalSendStatus) {
        if (!verbose) throw SmashExceptions::SignalSendException();
        throw SmashExceptions::SyscallException("kill");
    }

    //if this is wait/continue signal, update JobsManager as well
    bool stopSignal = (signum==SIGSTOP || signum==SIGTSTP || signum==SIGTTIN || signum==SIGTTOU);
    bool contSignal = (signum==SIGCONT);
    if (stopSignal) smash->jobs.pauseJob(jobId);
    if (contSignal) smash->jobs.registerUnpauseJob(jobId);

    if (verbose) cout << "signal number " << signum << " was sent to pid " << pcbPtr->getProcessId() << endl;
}

BuiltInCommand::BuiltInCommand(string cmd_line, SmallShell *smash) : Command(_removeBackgroundSign(cmd_line), smash){
    isBuiltIn = true;
}

ForegroundCommand::ForegroundCommand(string cmd_line, SmallShell *smash) : BuiltInCommand(cmd_line, smash) {
    try {
        if (args.size() - 1 > 1) throw std::invalid_argument("Too many args");
        if (args.size() - 1 != 0) jobId = stoi(args[1]);
    } catch (std::invalid_argument& e) {
        throw SmashExceptions::InvalidArgumentsException("fg");
    }
    if (args.size() - 1 == 0) {
        if (smash->jobs.isEmpty()) throw SmashExceptions::Exception("fg", "jobs list is empty");
        jobId = smash->jobs.getLastJob()->getJobId();
    }
    pcb = smash->jobs.getJobById(jobId);
    if (!pcb) throw SmashExceptions::Exception("fg", "job-id " + to_string(jobId) + " does not exist");
}

void ForegroundCommand::execute() {
    cout << *pcb << endl;

    ::sendSignal(*pcb, SIGCONT);
    pid_t pid = pcb->getProcessId();

    //make a copy to prevent foregroundProcess from becoming a dangling pointer after job removal
    ProcessControlBlock reservePcb = ProcessControlBlock(*pcb);

    smash->setForegroundProcess(&reservePcb);
    smash->jobs.removeJobById(jobId);
    int jobData=0;//debug
    const int waitStatus = waitpid(pid, &jobData, WUNTRACED);
    if (waitStatus < 0) throw SmashExceptions::SyscallException("waitpid");
    smash->setForegroundProcess(nullptr);

    // ROI - loop to remove timed process in case it ended before the timeout
    if (!(smash->jobs.timed_processes.size() < 1)) {

        for (TimedProcessControlBlock &timed_pcb: smash->jobs.timed_processes) {
            if (jobId == timed_pcb.getJobId()) {
                smash->jobs.timed_processes.remove(timed_pcb);
                smash->jobs.timed_processes.sort();
                //set new signal to next item in list
                smash->jobs.setAlarmSignal();
            }
        }
    }
}
    /*
        for (auto it = smash->jobs.timed_processes.begin(); it != smash->jobs.timed_processes.end(); ++it) {
        if (!*it) continue; //AKIVA - to prevent segmentation faults
        if (jobId == (*it)->getJobId()) smash->jobs.timed_processes.erase(it);
    }
     */

BackgroundCommand::BackgroundCommand(string cmd_line, SmallShell *smash) : BuiltInCommand(cmd_line, smash) {
    //sanitize inputs
    //set jobId = args[0] or lastStoppedCommand if none specified
    try {
        if (args.size() - 1 > 1) throw std::invalid_argument("Too many arguments to bg.");
        if (args.size() - 1 == 1) jobId = stoi(args[1]);
        else jobId = smash->jobs.getLastStoppedJob()->getJobId();
    } catch (std::invalid_argument e) {
        throw SmashExceptions::InvalidArgumentsException("bg");
    } catch (SmashExceptions::NoStoppedJobsException e) {
        throw SmashExceptions::Exception("bg", "there is no stopped jobs to resume");
    }

    //find pcb
    pcb = smash->jobs.getJobById(jobId);
    if (!pcb) throw SmashExceptions::Exception("bg", "job-id " + to_string(jobId) + " does not exist");

    //if pcb is already running, print to stderr already running
    if (pcb->isRunning()) {
        throw SmashExceptions::Exception("bg", "job-id " + to_string(jobId) + " is already running in the background");
    }
}

void BackgroundCommand::execute() {
    cout << *(smash->jobs.getJobById(jobId)) << endl;
    smash->jobs.unpauseJob(jobId);
}

QuitCommand::QuitCommand(string cmd_line, SmallShell *smash) : BuiltInCommand(cmd_line, smash) {
    if (args.size() - 1 == 1 && args[1] == "kill") killRequest = true;
}

void QuitCommand::execute() {
    //kill processes if requested
    if (killRequest) smash->jobs.killAllJobs();

    exit(0);
}

BackgroundableCommand::BackgroundableCommand(string cmd_line, SmallShell *smash) :
    Command(cmd_line, smash), backgroundRequest(_isBackgroundComamnd(cmd_line)) {}

void BackgroundableCommand::execute() {
    //fork a son
    pid = fork();
    if (pid < 0) throw SmashExceptions::SyscallException("fork");
    if (pid == 0) {
        //DEBUG_PRINT("process "<<getppid()<<" forked a son for backgroundablecommand "<<cmd_line<<" with pid="<<getpid());
        smash->escapeSmashProcessGroup();

        if (!isRedirectionBuiltinForegroundCommand) executeBackgroundable();
        exit(0);
    } else {
        //if !backgroundRequest then wait for son, inform smash that a foreground program is running
        if (!backgroundRequest) {

            ProcessControlBlock foregroundPcb = ProcessControlBlock(FG_JOB_ID, pid, cmd_line);
            smash->setForegroundProcess(&foregroundPcb);

            if (isTimeOut) {
                //ROI - timeout handling
                smash->jobs.addTimedProcess(foregroundPcb.getJobId(), pid, cmd_line, waitNumber);
                smash->jobs.setAlarmSignal();
            }

            int waitStatus = 0;
            bool isHelperProcess = (getpgrp()==getppid());
            if (!isHelperProcess) waitStatus = waitpid(pid, nullptr, WUNTRACED);
            else waitStatus = waitpid(pid, nullptr, NO_OPTIONS);
            if (waitStatus < 0) {
                throw SmashExceptions::SyscallException("waitpid");
            }
            smash->setForegroundProcess(nullptr);
            if (isRedirectionBuiltinForegroundCommand) executeBackgroundable(); //run from smash process
        }
        //else add to jobs
        else {
            smash->jobs.addJob(*this, pid);
            // ROI - timeout handling
            if (isTimeOut) {
                smash->jobs.addTimedProcess(smash->jobs.getLastJob()->getJobId(), pid, cmd_line, waitNumber);
                smash->jobs.setAlarmSignal();
            }
        }
    }
}


PipeCommand::PipeCommand(std::string cmd_line, SmallShell *smash) : BackgroundableCommand(cmd_line, smash) {
    unsigned int pipeIndex = cmd_line.find_first_of('|');
    //sanitize inputs
    if (!(cmd_line.size() > pipeIndex + 1)) throw SmashExceptions::InvalidArgumentsException("pipe");

    if (cmd_line[pipeIndex + 1] == '&') errPipe = true;

    const string cmd_lineFrom = _removeBackgroundSign(cmd_line.substr(0, pipeIndex));
    const string cmd_lineTo = _removeBackgroundSign(cmd_line.substr(pipeIndex + 1 + (errPipe ? 1 : 0)));

    /*bool backgroundCommand = _isBackgroundComamnd(cmd_line);

    commandFrom = smash->CreateCommand(cmd_lineFrom + (backgroundCommand? "&" : ""));
    commandTo = smash->CreateCommand(cmd_lineTo + (backgroundCommand? "&" : ""));*/

    commandFrom = smash->CreateCommand(cmd_lineFrom);
    commandTo = smash->CreateCommand(cmd_lineTo);
    assert(commandTo && commandFrom);
    invalid = commandFrom->invalid && commandTo->invalid;

    //create pipe
    if (pipe(pipeSides)<0) throw SmashExceptions::SyscallException("pipe");
}


PipeCommand::PipeCommand(std::unique_ptr<Command> commandFrom, std::unique_ptr<Command> commandTo, SmallShell *smash) :
        BackgroundableCommand(cmd_line, smash), commandFrom(std::move(commandFrom)),
        commandTo(std::move(commandTo)) {

    //create pipe
    if (pipe(pipeSides)<0) throw SmashExceptions::SyscallException("pipe");
}

void PipeCommand::commandFromNonBuiltinExecution() {
    //run commandFrom fork
    if ((pidFrom = fork()) < 0) throw SmashExceptions::SyscallException("fork");
    if (!pidFrom) {
        *processGroupFromPtr = smash->escapeSmashProcessGroup();
        if (signal(SIGCONT, SIG_DFL) == SIG_ERR) throw SmashExceptions::SyscallException("signal");
        //DEBUG_PRINT("process "<<getppid()<<" forked a son for pipe commandFrom "<<commandFrom->cmd_line<<" with pid="<<getpid());
        //close pipe read side
        if (close(pipeSides[0])) throw SmashExceptions::SyscallException("close");
        //replace stdout with pipe write side
        if (dup2(pipeSides[1], errPipe ? STDERR_FILENO : STDOUT_FILENO) < 0)
            throw SmashExceptions::SyscallException("dup2");
        //execute commandFrom
        commandFrom->execute();
        exit(0);
    }
}

void PipeCommand::commandFromBuiltinExecution() {
    typedef int fd_number_t;
    fd_number_t outputAddress = errPipe ? STDERR_FILENO : STDOUT_FILENO;

    DEBUG_PRINT("errPipe"<<((errPipe==STDERR_FILENO)? "=" : "!")<<"=STDERR_FILENO");
    //duplicate stdout/stderr
    fd_number_t stdoutCopy = dup(outputAddress);

    //replace stdout/err with pipe write side
    if (dup2(pipeSides[1], outputAddress) < 0)
        throw SmashExceptions::SyscallException("dup2");

    DEBUG_PRINT("this was written to stderr during redirection");
    cout<<"this was written to stdout during redirection"<<endl; //debug
    //execute commandFrom
    commandFrom->execute();

    //restore stdout/err
    if (dup2(stdoutCopy, outputAddress)<0) throw SmashExceptions::SyscallException("dup2");
    //close duplicate
    if (close(stdoutCopy)<0) throw SmashExceptions::SyscallException("close");

    DEBUG_PRINT("this was written to stderr after restore");
    cout<<"this was written to stdout after restore"<<endl; //debug

    //run commandFrom fork (pointless, for structure)
    if ((pidFrom = fork()) < 0) throw SmashExceptions::SyscallException("fork");
    if (!pidFrom){
        //DEBUG_PRINT("CommandFrom pointlessly started, exiting");
        //close pipe read side
        if (close(pipeSides[0])) throw SmashExceptions::SyscallException("close");
        exit(0);
    }
}

void PipeCommand::commandFromExecution() {
    if (!(commandFrom->isBuiltIn) || !isRedirectionCommand) commandFromNonBuiltinExecution();
    else commandFromBuiltinExecution();
}

void PipeCommand::commandToExecution() {
    //run commandTo fork
    if ((pidTo = fork()) < 0) throw SmashExceptions::SyscallException("fork");
    if (!pidTo) {
        *processGroupToPtr = smash->escapeSmashProcessGroup();
        if (signal(SIGCONT, SIG_DFL) == SIG_ERR) throw SmashExceptions::SyscallException("signal");
        //DEBUG_PRINT("process "<<getppid()<<" forked a son for pipe commandTo "<<commandTo->cmd_line<<" with pid="<<getpid());
        //close pipe write side
        if (close(pipeSides[1])) throw SmashExceptions::SyscallException("close");
        //replace stdin with pipe read side
        if (dup2(pipeSides[0], STDIN_FILENO) < 0) throw SmashExceptions::SyscallException("dup2");
        //execute commandTo
        commandTo->execute();
        exit(0);
    }
}

void PipeCommand::executeBackgroundable() {
    //create pipe
    if (pipe(pipeSides)) throw SmashExceptions::SyscallException("pipe");

    PipeCommand::commandFromExecution();
    if (pidFrom) {
        commandToExecution();
        if (pidTo){
            //parent
            if (close(pipeSides[0]) || close(pipeSides[1])) throw SmashExceptions::SyscallException("close");

            //wait for commandTo fork and for commandFrom fork to finish
            if (waitpid(pidFrom, nullptr, NO_OPTIONS)<0) throw SmashExceptions::SyscallException("wait");
            //DEBUG_PRINT("Finished waiting for commandFrom.  Sending signal to it at "<<pidFrom<<((kill(pidFrom, 0)<0)? " failed, as it should":" succeeded (uh oh)"));
            if (waitpid(pidTo,nullptr, NO_OPTIONS)<0) throw SmashExceptions::SyscallException("wait");
            //DEBUG_PRINT("Finished waiting for commandTo.  Sending signal to it at "<<pidTo<<((kill(pidTo, 0)<0)? " failed, as it should":" succeeded (uh oh)"));

            pidFrom = pidTo = -1;
        }
    }
}


PipeCommand::~PipeCommand() {
    if (pidFrom != -1) {
        DEBUG_PRINT("sending SIGKILL to commandFrom");
        if (killpg(processGroupFrom, SIGKILL) < 0) {
            DEBUG_PRINT("pipe command: pidFrom = " << pidFrom << " and kill failed with errno = " << errno);
            perror("killpg failed");
        }
    }
    if (pidTo != -1) {
        DEBUG_PRINT("sending SIGKILL to commandTo");
        if (killpg(processGroupTo, SIGKILL) < 0) {
            DEBUG_PRINT("pipe command: pidTo = " << pidFrom << " and kill failed with errno = " << errno);
            perror("killpg failed");
        }
    }
}

unsigned short indicator(bool condition) {
    return condition ? 1 : 0;
}

RedirectionCommand::RedirectionCommand(unique_ptr<Command> commandFrom, string filename, bool append, SmallShell *smash) :
        PipeCommand(std::move(commandFrom),
                unique_ptr<Command>(new WriteCommand(filename, append, smash)), smash) {

    isRedirectionCommand = true;
}


#define OPERATOR_POSITION (cmd_line.find_first_of('>'))
RedirectionCommand::RedirectionCommand(std::string cmd_line, SmallShell *smash) :
        RedirectionCommand(smash->CreateCommand(cmd_line.substr(0, OPERATOR_POSITION)),
                           _trim(_removeBackgroundSign(cmd_line).substr(OPERATOR_POSITION + 1
                                                                        + indicator(
                                   cmd_line.at(1 + OPERATOR_POSITION) == '>'))),
                           cmd_line.at(1 + OPERATOR_POSITION) == '>',
                           smash) {}
#undef OPERATOR_POSITION


void RedirectionCommand::execute() {
    isRedirectionBuiltinForegroundCommand = commandFrom->isBuiltIn && !BackgroundableCommand::backgroundRequest;
    PipeCommand::execute();
}

RedirectionCommand::WriteCommand::WriteCommand(string fileName, bool append, SmallShell *smash) :
        Command("write_into " + fileName, smash) {

    sink.open(fileName, append ? std::ofstream::app : std::ofstream::trunc);
    if (sink.failbit & sink.rdstate()) throw SmashExceptions::SyscallException("open");
}

void RedirectionCommand::WriteCommand::execute() {
    char buf;
    int readStatus;
    while ((readStatus = read(STDIN_FILENO, &buf, 1)) > 0) {
        sink.write(&buf, 1);
        sink.flush();
    }
}

RedirectionCommand::WriteCommand::~WriteCommand() {
    sink.close();
    cout<<closingMessage;
}

void RedirectionCommand::WriteCommand::setClosingMessage(const string &closingMessage) {
    WriteCommand::closingMessage = closingMessage;
}

CopyCommand::CopyCommand(string cmd_line, SmallShell *smash) try :
        RedirectionCommand(
                unique_ptr<Command>(
                        new ReadCommand(getSourceFile((initArgs(cmd_line))), smash)),
                getTargetFile(initArgs(cmd_line)),
                false,
                smash) {

    this->cmd_line = cmd_line;
    args = initArgs(cmd_line);
    backgroundRequest = _isBackgroundComamnd(cmd_line);
    dynamic_cast<RedirectionCommand::WriteCommand*>(commandTo.get())->
        setClosingMessage("smash: "+args.at(1)+" was copied to "+args.at(2)+"\n");
} catch (std::out_of_range& e) {
    throw SmashExceptions::InvalidArgumentsException("cp");
}


void CopyCommand::execute() {
    RedirectionCommand::execute();
}

bool CopyCommand::isSameFile(string fileFrom, string fileTo) {
    char* fullPathFrom = (char*)malloc(sizeof(char)*(1+PATH_MAX));
    char* fullPathTo = (char*)malloc(sizeof(char)*(1+PATH_MAX));
    if (!fullPathFrom || !fullPathTo){
        if (fullPathFrom) free(fullPathFrom);
        if (fullPathTo) free(fullPathTo);
        throw SmashExceptions::SyscallException("malloc");
    }

    realpath(fileFrom.c_str(), fullPathFrom);
    realpath(fileTo.c_str(), fullPathTo);
    bool notResult = strcmp(fullPathFrom, fullPathTo);
    free(fullPathFrom);
    free(fullPathTo);
    return !notResult;
}

string CopyCommand::getTargetFile(std::vector<std::string> args) {
    if (isSameFile(args.at(1), args.at(2))) throw SmashExceptions::SameFileException();
    return args.at(2);
}

string CopyCommand::getSourceFile(std::vector<std::string> args) {
    if (args.size()-1 < 2) throw SmashExceptions::InvalidArgumentsException("cp");
    return args.at(1);
}

CopyCommand::ReadCommand::ReadCommand(string fileName, SmallShell *smash) :
        Command("read_from " + fileName, smash) {

    source.open(fileName);
    if (source.failbit & source.rdstate()) throw SmashExceptions::SyscallException("open");
}


void CopyCommand::ReadCommand::execute() {
    for (char nextLetter = source.get(); source.good() && nextLetter != EOF; nextLetter = source.get()) {
        cout << nextLetter;
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
    string no_timeout_cmd = trimmed_cmd.substr(7, trimmed_cmd.length() + 1);
    //string trimmed_no_timeout_cmd = _trim(no_timeout_cmd);
    unsigned short digits_index = _trim(no_timeout_cmd).find_first_of(DIGITS);
    if (digits_index != 0) throw SmashExceptions::InvalidArgumentsException("timeout");
    unsigned short digits_end_index = _trim(no_timeout_cmd).find_first_of(' ');
    string str_number = _trim(no_timeout_cmd).substr(digits_index, digits_end_index);
    if (str_number.find_first_not_of(DIGITS) != std::string::npos)
        throw SmashExceptions::InvalidArgumentsException("timeout");
    inner_cmd_line = trimmed_cmd.substr(trimmed_cmd.find_first_of(str_number) + str_number.length() + 1,
                                        trimmed_cmd.length() + 1);

    /*
    //check to avoid timeout loop in command, if it exists i distort it so that CreatingCommand fails regularly, but removing the 1st letter
    string opcode = inner_cmd_line.substr(0, inner_cmd_line.find_first_of(WHITESPACE));
    if (("timeout") == opcode) inner_cmd_line.erase(0, 1);
    if ((("chprompt") == opcode) || (("showpid") == opcode) || (("pwd") == opcode) || (("cd") == opcode) ||
        (("jobs") == opcode) || (("kill") == opcode) || (("bg") == opcode) || (("fg") == opcode) ||
        (("quit") == opcode)) {
        isBuiltIn = true;
    }
    */ //AKIVA - set BuiltInCommand constructor to set isBuiltIn=true

    innerCommand = smash->CreateCommand(inner_cmd_line);
    innerCommand->isTimeOut = true;
    innerCommand->waitNumber = waitNumber;
    //set cmd_line for inner command to include 'timeout' in string
    innerCommand->cmd_line = cmd_line;
    cout << inner_cmd_line << endl; //DEBUG
    waitNumber = stoi(str_number);

}

void TimeoutCommand::execute() {

    //in case of built-in command
    if (isBuiltIn) {
        innerCommand->execute();
        smash->jobs.addTimedProcess(BuiltInID, BuiltInID, " ", waitNumber);
        smash->jobs.setAlarmSignal();
        return;
    }
    //in case of external command
    innerCommand->execute();
}

//ROI old execute
/*
void TimeoutCommand::execute() {

    //in case of built-in command
    if (isBuiltIn) {
        innerCommand->execute();
        smash->jobs.addTimedProcess(BuiltInID, BuiltInID, " ", waitNumber);
        smash->jobs.setAlarmSignal();
        return;
    }

    //fork a son
    pid_t pid = fork();
    if (pid < 0) throw SmashExceptions::SyscallException("fork");
    if (pid == 0){
        if (setpgrp() < 0) throw SmashExceptions::SyscallException("setpgrp");
        execl("/bin/bash", "/bin/bash", "-c", _removeBackgroundSign(inner_cmd_line).c_str(), NULL);
        exit(0);
    }

    else{
        //if !backgroundRequest then wait for son, inform smash that a foreground program is running
        if (!backgroundRequest){
            const job_id_t FG_JOB_ID = 0;
            ProcessControlBlock foregroundPcb = ProcessControlBlock(FG_JOB_ID, pid, cmd_line);
            smash->setForegroundProcess(&foregroundPcb);

            //ROI
            smash->jobs.addTimedProcess(foregroundPcb.getJobId(), pid, cmd_line, waitNumber);
            smash->jobs.setAlarmSignal();

            //ProcessControlBlock* timed_pcb = smash->getForegroundProcess1();
            //smash->setIsForgroundTimed(true);
            //smash->djobs.timed_processes.push_back(timed_pcb);
            const int waitStatus = waitpid(pid, nullptr, WUNTRACED);
            if (waitStatus < 0) throw SmashExceptions::SyscallException("waitpid");
            //smash->setIsForgroundTimed(false);
            smash->setForegroundProcess(nullptr);
        }
            //else add to jobs
        else{
            smash->jobs.addJob(*this, pid);



            ProcessControlBlock *foregroundPcb2 = smash->jobs.getLastJob();
            foregroundPcb2->duration = waitNumber;
            foregroundPcb2->setStartTime((time(nullptr)));
            smash->jobs.timed_processes.push_back(foregroundPcb2);

            smash->jobs.addTimedProcess(smash->jobs.getLastJob()->getJobId(), pid, cmd_line, waitNumber);
            smash->jobs.setAlarmSignal();
        }
    }
}
*/

/*
void TimeoutCommand::execute() {
    //in case of built-in command
    if (isBuiltIn) {
        innerCommand->execute();
        alarm(waitNumber);
        return;
    }
    //in case of special command

}
 */

ExternalCommand::ExternalCommand(string cmd_line, SmallShell *smash) : BackgroundableCommand(cmd_line, smash) {}

void ExternalCommand::executeBackgroundable() {
    execl("/bin/bash", "/bin/bash", "-c", _removeBackgroundSign(cmd_line).c_str(), NULL);
}

