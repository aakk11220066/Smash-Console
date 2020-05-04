#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <list>
#include <string>
#include <map>
#include <memory>
#include <signal.h>
#include <fstream>
#include <memory>
#include <assert.h>
#include "ProcessControlBlock.h"

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)
#define DEBUG_PRINT(err_msg) /*std::cerr << "DEBUG: " << err_msg << std::endl*/



typedef int errno_t;
typedef int job_id_t;
typedef unsigned int signal_t;

class Command;
class SmallShell;

bool sendSignal(const ProcessControlBlock& pcb, signal_t sig_num, errno_t* errCodeReturned=nullptr);

using std::string;
using std::unique_ptr;

template<class T>
class Heap : private std::vector<T> {
public:
    T getMax();
    void insert(T& newElement);
    void erase (const T& target);
    bool empty();
};

class JobsManager {
public:
    //ROI - list of timed processes
    std::list<TimedProcessControlBlock> timed_processes;
private:
    //Dictionary mapping job_id to process
    std::map<job_id_t, ProcessControlBlock> processes;

    //std::list<ProcessControlBlock*> runQueue;
    Heap<ProcessControlBlock*> waitingHeap;

    SmallShell& smash;
    job_id_t maxIndex = 0;

    job_id_t resetMaxIndex();

public:
    JobsManager(SmallShell& smash);
    ~JobsManager() = default;
    void addJob(const Command& cmd, pid_t pid);
    void addJob(const ProcessControlBlock& pcb);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    ProcessControlBlock* getJobById(job_id_t jobId);
    void removeJobById(job_id_t jobId);
    ProcessControlBlock * getLastJob();
    ProcessControlBlock *getLastStoppedJob();
    void pauseJob(job_id_t jobId);
    void unpauseJob(job_id_t jobId);
    void registerUnpauseJob(job_id_t jobId); //administrative side of unpausing job
    bool isEmpty();


//ROI
// note that -2 in pid and jid implies a builtin command
//AKIVA: why not just use isBuiltIn field of Command class?
    void addTimedProcess(const job_id_t jobId,
                                      const pid_t processId,
                                      const std::string& creatingCommand, int futureSeconds);


    void setAlarmSignal();


};

class SmallShell {
private:
    SmallShell();

    //bool isForgroundTimed = false;
    //bool hasProcessTimedOut = false;
    std::string smashPrompt = "smash> ";

    std::string lastPwd = "";

    const ProcessControlBlock* foregroundProcess = nullptr;

    const pid_t smashProcessGroup;
public:
    const ProcessControlBlock *getForegroundProcess() const;
    ProcessControlBlock *getForegroundProcess1() const;

    void setForegroundProcess(const ProcessControlBlock *foregroundProcess);

    const pid_t smashPid;

    /// change process group to a different one from that of the smash
    /// \return new process group
    signal_t escapeSmashProcessGroup();
public:
    void RemoveLateProcess(const pid_t); //ROI
    TimedProcessControlBlock *getLateProcess(); //ROI
    void RemoveLateProcesses(); //ROI
    const std::string &getLastPwd() const;
    void setLastPwd(const std::string &lastPwd);
    bool sendSignal(signal_t signum, job_id_t jobId);
    /*
    bool getIsForgroundTimed() const; //ROI
    void setIsForgroundTimed(bool value); //ROI
    bool getHasProcessTimedOut() const; //ROI
    void setHasProcessTimedOut(bool value); //ROI
     */
    JobsManager jobs;

public:
    unique_ptr<Command> CreateCommand(std::string cmd_line);

    SmallShell(SmallShell const&)      = delete; // disable copy ctor

    void operator=(SmallShell const&)  = delete; // disable = operator

    static SmallShell& getInstance() { // make SmallShell singleton
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    ~SmallShell();

    void executeCommand(std::string cmd_line);

    const std::string &getSmashPrompt() const;

    void setSmashPrompt(const std::string &smashPrompt);
};


class Command {
protected:
    std::vector<std::string> args;
    SmallShell* const smash;

public:
    bool verbose = true;
    bool invalid = false;
    std::string cmd_line;
    bool isBuiltIn = false;
    bool isTimeOut = false;
    int waitNumber = 0;

public:
    Command(std::string cmd_line, SmallShell* smash);
    virtual ~Command() = default;
    virtual void execute() = 0;
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(std::string cmd_line, SmallShell* smash);

    virtual ~BuiltInCommand() = default;
};

class BackgroundableCommand : public Command {
private:
    pid_t pid=0;

protected:
    bool backgroundRequest = false;
    bool isRedirectionBuiltinForegroundCommand = false;
public:
    BackgroundableCommand(string cmd_line, SmallShell* smash);
    virtual ~BackgroundableCommand() = default;
    void execute();
    virtual void executeBackgroundable() = 0;
};

class ExternalCommand : public BackgroundableCommand {
private:
    void runExec();
public:
    ExternalCommand(string cmd_line, SmallShell* smash);
    virtual ~ExternalCommand() = default;
    void executeBackgroundable() override;
};

class PipeCommand : public BackgroundableCommand {
private:
    bool errPipe = false;
    int pidFrom=-1, pidTo=-1;
    pid_t processGroupFrom = getpgrp();
    pid_t processGroupTo = processGroupFrom;
    pid_t *processGroupToPtr=&processGroupTo, *processGroupFromPtr=&processGroupFrom;
    int pipeSides[2] = {0,0};

    void commandFromBuiltinExecution();
    void commandFromNonBuiltinExecution();
    void commandFromExecution();
    void commandToExecution();

protected:
    unique_ptr<Command> commandFrom= nullptr, commandTo=nullptr;

    bool isRedirectionCommand = false;
public:
    PipeCommand(std::string cmd_line, SmallShell* smash);
    PipeCommand(unique_ptr<Command> commandFrom, unique_ptr<Command> commandTo, SmallShell *smash);
    virtual ~PipeCommand();
    void executeBackgroundable() override;
};

class RedirectionCommand : public PipeCommand {
private:
    bool append = false;
    class WriteCommand : public Command{
        std::ofstream sink;
        void writeToSink();
    public:
        explicit WriteCommand(string fileName, bool append, SmallShell* smash);
        virtual ~WriteCommand();
        virtual void execute() override;
    };

public:
    RedirectionCommand(std::string cmd_line, SmallShell* smash);
    RedirectionCommand(unique_ptr<Command> commandFrom, string filename, bool append,
            SmallShell* smash);
    virtual ~RedirectionCommand() = default;
    void execute() override;
};

class ChangeDirCommand : public BuiltInCommand {
public:
    ChangeDirCommand(std::string cmd_line, SmallShell* smash);
    virtual ~ChangeDirCommand() = default;
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(std::string cmd_line, SmallShell* smash);
    virtual ~GetCurrDirCommand() = default;
    void execute() override;

    static std::string getCurrDir();
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(string cmd_line, SmallShell* smash);
    virtual ~ShowPidCommand() = default;
    void execute() override;
};

class QuitCommand : public BuiltInCommand {
private:
    bool killRequest = false;
public:
    QuitCommand(string cmd_line, SmallShell* smash);
    virtual ~QuitCommand() = default;
    void execute() override;
};

/*
class CommandsHistory {
 protected:
  class CommandHistoryEntry {
  };
 public:
  CommandsHistory();
  ~CommandsHistory() = default;
  void addRecord(const char* cmd_line);
  void printHistory();
};

class HistoryCommand : public BuiltInCommand {
 public:
  HistoryCommand(const char* cmd_line, CommandsHistory* history);
  virtual ~HistoryCommand() = default;
  void execute() override;
};*/

class JobsCommand : public BuiltInCommand {
public:
    JobsCommand(string cmd_line, SmallShell* smash);
    virtual ~JobsCommand() = default;
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    signal_t signum = -1;
    job_id_t jobId = -1;
public:
    KillCommand(string cmd_line, SmallShell* smash);
    virtual ~KillCommand() = default;
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
private:
    job_id_t jobId;
    ProcessControlBlock* pcb = nullptr;

public:
    ForegroundCommand(string cmd_line, SmallShell* smash);
    virtual ~ForegroundCommand() = default;
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    job_id_t jobId;
    ProcessControlBlock* pcb = nullptr;

public:
    BackgroundCommand(string cmd_line, SmallShell* smash);
    virtual ~BackgroundCommand() = default;
    void execute() override;
};

class CopyCommand : public RedirectionCommand {
private:
    class ReadCommand : public Command{
        std::ifstream source;
        void read();
    public:
        explicit ReadCommand(string fileName, SmallShell* smash);
        virtual ~ReadCommand();
        void execute();
    };

    string getSourceFile(std::vector<std::string> args);
    string getTargetFile(std::vector<std::string> args);

    bool isSameFile(string fileFrom, string fileTo);

public:
    CopyCommand(string cmd_line, SmallShell* smash);
    virtual ~CopyCommand() = default;
    void execute() override;
};

class ChpromptCommand : public BuiltInCommand {
private:
    const std::string newPrompt;

public:
    ChpromptCommand(string cmd_line, SmallShell* smash);
    virtual ~ChpromptCommand() = default;
    void execute() override;
};


// ROI - timeout command

class TimeoutCommand : public Command {
    string inner_cmd_line;
    unique_ptr<Command> innerCommand = nullptr;

private:
    bool backgroundRequest = false;
    int waitNumber;
public:
    TimeoutCommand(string cmd_line, SmallShell* smash);
    virtual ~TimeoutCommand() = default;
    void execute() override;
};



namespace SmashExceptions{
    class Exception;
    class SignalSendException;
    class NoStoppedJobsException;
    class FileOpenException;
    class InvalidArgumentsException;
    class SyscallException;
    class TooManyArgumentsException;
    class SameFileException;
}

class SmashExceptions::Exception : public std::exception{
public:
    std::string sender;

protected:
    std::string errMsg;
    std::string whatMsg;
public:
    explicit Exception(const std::string& sender, const std::string& errMsg) : sender(sender),
        errMsg(errMsg), whatMsg("smash error: "+sender+": "+errMsg){}

    const char* what() const noexcept override{
        const char* result = whatMsg.c_str();
        return result;
    }
};

class SmashExceptions::SignalSendException : public SmashExceptions::Exception{
public:
    SignalSendException() : SmashExceptions::Exception("",""){};
};
class SmashExceptions::NoStoppedJobsException : public SmashExceptions::Exception{
public:
    NoStoppedJobsException() : Exception("",""){};
};
class SmashExceptions::FileOpenException : public SmashExceptions::Exception{
public:
    FileOpenException() : Exception("",""){};
} ;
class SmashExceptions::InvalidArgumentsException : public SmashExceptions::Exception{
public:
    InvalidArgumentsException(const string& sender) : SmashExceptions::Exception(sender, "invalid arguments"){}
};
class SmashExceptions::SyscallException : public Exception{
    string syscallErrMsg;
public:
    SyscallException(const string& _syscall) :
        Exception(_syscall,_syscall+" failed"), syscallErrMsg("smash error: "+errMsg){
    };
    const char* what() const noexcept override{
        return syscallErrMsg.c_str();
    };
};
class SmashExceptions::TooManyArgumentsException : public Exception{
public:
    TooManyArgumentsException(const string& sender) : Exception(sender, "too many arguments"){};
};
class SmashExceptions::SameFileException : public Exception{
public:
    SameFileException() : Exception("",""){}
};

#undef DEBUG_PRINT
#endif //SMASH_COMMAND_H_
