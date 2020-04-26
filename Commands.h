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

typedef unsigned int job_id_t;
typedef unsigned int signal_t;

class Command;
class SmallShell;

using std::string;
using std::unique_ptr;

class JobsManager {
private:
    //Dictionary mapping job_id to process
    std::map<job_id_t, ProcessControlBlock> processes;

    //std::list<ProcessControlBlock*> runQueue;
    std::vector<ProcessControlBlock*> waitingHeap;

    SmallShell& smash;
    job_id_t maxIndex = 1;

    job_id_t resetMaxIndex();
public:
    JobsManager(SmallShell& smash);
    ~JobsManager() = default;
    void addJob(const Command& cmd, pid_t pid);
    void addJob(const ProcessControlBlock& pcb);
    void printJobsList();
    void killAllJobs(); //FIXME: doesn't do anything
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

    std::string smashPrompt = "smash> ";

    std::string lastPwd = "";

    const ProcessControlBlock* foregroundProcess = nullptr;
public:
    const ProcessControlBlock *getForegroundProcess() const;

    void setForegroundProcess(const ProcessControlBlock *foregroundProcess);

public:
    ProcessControlBlock* getLateProcess(){ //FIXME: implement
        return nullptr;
    }
    const std::string &getLastPwd() const;
    void setLastPwd(const std::string &lastPwd);
    bool sendSignal(signal_t signum, job_id_t jobId);

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
    bool backgroundRequest = false;
    pid_t pid;

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

class PipeCommand : public BackgroundableCommand { //TODO: test PipeCommand, RedirectionCommand, and CopyCommand sent to background
private:
    bool errPipe = false;
    int pidFrom=-1, pidTo=-1;

protected:
    unique_ptr<Command> commandFrom= nullptr, commandTo=nullptr;

public:
    PipeCommand(std::string cmd_line, SmallShell* smash);
    PipeCommand(unique_ptr<Command> commandFrom, unique_ptr<Command> commandTo, SmallShell *smash);
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

protected:
    bool backgroundRequest = false;
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
	  // TODO: Add your data members
  };
 // TODO: Add your data members
 public:
  CommandsHistory();
  ~CommandsHistory() = default;
  void addRecord(const char* cmd_line);
  void printHistory();
};

class HistoryCommand : public BuiltInCommand {
 // TODO: Add your data members
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

// TODO: add timeoutCommand class


namespace SmashExceptions{
    class Exception : public std::exception {};
    class SignalSendException : public Exception {};
    class NoStoppedJobsException : public Exception {};
    class FileOpenException : public Exception{};
    class InvalidArgumentsException;
}

class SmashExceptions::InvalidArgumentsException : public SmashExceptions::Exception{
    string errMsg;
public:
    InvalidArgumentsException(string sender) : errMsg("smash error: "+sender+": invalid arguments"){}
    const char* what(){
        return errMsg.c_str();
    }
};

#endif //SMASH_COMMAND_H_
