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
#define DEBUG_PRINT(err_msg) std::cerr << "DEBUG: " << err_msg << std::endl

typedef unsigned int job_id_t;
typedef unsigned int signal_t;

class Command;
class SmallShell;

using std::string;
using std::unique_ptr;

class JobsManager {
public:
    //ROI - list of timed processes
    std::list<ProcessControlBlock*> timed_processes;
private:
    //Dictionary mapping job_id to process
    std::map<job_id_t, ProcessControlBlock> processes;

    //std::list<ProcessControlBlock*> runQueue;
    std::vector<ProcessControlBlock*> waitingHeap;

    SmallShell& smash;
    job_id_t maxIndex = 0;

    job_id_t resetMaxIndex();
public:
    JobsManager(SmallShell& smash);
    ~JobsManager() = default;
    // ROI
    void addJob(const Command& cmd, pid_t pid, int duration = -1);
    void addJob(const ProcessControlBlock& pcb);
    void printJobsList();
    void killAllJobs();
    void removeFinishedJobs();
    ProcessControlBlock* getJobById(job_id_t jobId);
    void removeJobById(job_id_t jobId);
    ProcessControlBlock * getLastJob();
    ProcessControlBlock *getLastStoppedJob();
    void pauseJob(job_id_t jobId);
    bool isEmpty();
};

class SmallShell {
private:
    SmallShell();

    bool isForgroundTimed = false;
    std::string smashPrompt = "smash> ";

    std::string lastPwd = "";

    const ProcessControlBlock* foregroundProcess = nullptr;
public:
    const ProcessControlBlock *getForegroundProcess() const;
    ProcessControlBlock *getForegroundProcess1() const;


    void setForegroundProcess(const ProcessControlBlock *foregroundProcess);

    pid_t smashPid;
public:
    void RemoveLateProcess(const pid_t); //ROI
    ProcessControlBlock* getLateProcess(); //ROI
    void RemoveLateProcess(const job_id_t); //ROI
    ProcessControlBlock* getLateProcess(); //ROI
    const std::string &getLastPwd() const;
    void setLastPwd(const std::string &lastPwd);
    bool sendSignal(signal_t signum, job_id_t jobId);
    bool getIsForgroundTimed() const; //ROI
    void setIsForgroundTimed(bool value); //ROI

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
    pid_t pid;

protected:
    bool backgroundRequest = false;
public:
    BackgroundableCommand(string cmd_line, SmallShell* smash);
    virtual ~BackgroundableCommand() = default;
    void execute();
    virtual void executeBackgroundable() = 0;
};

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

protected:
    unique_ptr<Command> commandFrom= nullptr, commandTo=nullptr;

public:
    PipeCommand(std::string cmd_line, SmallShell* smash);
    PipeCommand(unique_ptr<Command> commandFrom, unique_ptr<Command> commandTo, SmallShell *smash);
    virtual ~PipeCommand();
    void executeBackgroundable() override
    ;
    virtual ~PipeCommand();
    void executeBackgroundable() override;
};

class RedirectionCommand : public PipeCommand {
private:
    bool append = false;
    short operatorPosition = -1;
    SmallShell* sanitizeInputs(SmallShell* smash);
    class WriteCommand : public Command{
        std::ofstream sink;
        void writeToSink();
    public:
        explicit WriteCommand(string fileName, bool append, SmallShell* smash);
        virtual ~WriteCommand();
        void execute() override;
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
        void execute() override;
    };
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
    class TooFewArgumentsException;
}

class SmashExceptions::Exception : public std::exception{
private:
    std::string errMsg;
    std::string sender;

public:
    explicit Exception(const std::string& sender, const std::string& errMsg) : sender(sender),
        errMsg(errMsg){}

    virtual const char* what(){
        return ("smash error: "+sender+": "+errMsg).c_str();
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
};
class SmashExceptions::InvalidArgumentsException : public SmashExceptions::Exception{
public:
    InvalidArgumentsException(const string& sender) : SmashExceptions::Exception(sender, "invalid arguments"){}
};
class SmashExceptions::SyscallException : public Exception{
    const string& _syscall;
public:
    SyscallException(const string& _syscall) :
        _syscall(_syscall),
        Exception(_syscall,_syscall+" failed"){
        DEBUG_PRINT("kill command exception thrown");
    };
    const char* what() override{
        return ("smash error: "+_syscall+" failed").c_str();
    };
};
class SmashExceptions::TooManyArgumentsException : public Exception{
public:
    TooManyArgumentsException(const string& sender) : Exception(sender, "too many arguments"){};
};
class SmashExceptions::TooFewArgumentsException : public Exception{
public:
    TooFewArgumentsException(const string& sender) : Exception(sender, "too few arguments"){};
};
#undef DEBUG_PRINT
#endif //SMASH_COMMAND_H_
