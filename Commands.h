#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>
#include <list>
#include <string>
#include <map>
#include <signal.h>
#include "ProcessControlBlock.h"

#define COMMAND_ARGS_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define HISTORY_MAX_RECORDS (50)

typedef unsigned int job_id_t;
typedef unsigned int signal_t;

using std::string;

class Command;
class SmallShell;

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
    void killAllJobs();
    void removeFinishedJobs();
    ProcessControlBlock* getJobById(job_id_t jobId);
    void removeJobById(job_id_t jobId);
    ProcessControlBlock * getLastJob();
    ProcessControlBlock *getLastStoppedJob();
    void pauseJob(job_id_t jobId);
    bool isEmpty();
    // TODO: Add extra methods or modify exisitng ones as needed
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
    ProcessControlBlock* getLateProcess(){ //FIXME: DEBUG
        return nullptr;
    }
    const std::string &getLastPwd() const;
    void setLastPwd(const std::string &lastPwd);
    bool sendSignal(signal_t signum, job_id_t jobId);

    JobsManager jobs;

public:
    std::unique_ptr<Command> CreateCommand(std::string cmd_line);

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

    // TODO: add extra methods as needed
};


class Command {
protected:
    std::vector<const std::string> args;
    SmallShell* const smash = nullptr;

public:
    bool invalid = false;
    std::string cmd_line;

public:
    Command(std::string cmd_line, SmallShell* smash);
    virtual ~Command() = default;
    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed
};

class BuiltInCommand : public Command {
public:
    BuiltInCommand(std::string cmd_line, SmallShell* smash);

    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
private:
    bool backgroundRequest = false;
    string commandText;

public:
    ExternalCommand(string cmd_line, SmallShell* smash);
    virtual ~ExternalCommand() {}
    void execute() override;
};

class PipeCommand : public Command {
  // TODO: Add your data members
 public:
  PipeCommand(std::string cmd_line, SmallShell* smash);
  virtual ~PipeCommand() {}
  void execute() override;
};

class RedirectionCommand : public Command {
 // TODO: Add your data members
 public:
  explicit RedirectionCommand(std::string cmd_line, SmallShell* smash);
  virtual ~RedirectionCommand() {}
  void execute() override;
  //void prepare() override;
  //void cleanup() override;
};

class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members
public:
    ChangeDirCommand(std::string cmd_line, SmallShell* smash);
    virtual ~ChangeDirCommand() {}
    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(std::string cmd_line, SmallShell* smash);
    virtual ~GetCurrDirCommand() {}
    void execute() override;

    static std::string getCurrDir();
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(string cmd_line, SmallShell* smash);
    virtual ~ShowPidCommand() {}
    void execute() override;
};

class QuitCommand : public BuiltInCommand {
private:
    bool killRequest = false;
public:
    QuitCommand(string cmd_line, SmallShell* smash);
    virtual ~QuitCommand() {}
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
  ~CommandsHistory() {}
  void addRecord(const char* cmd_line);
  void printHistory();
};

class HistoryCommand : public BuiltInCommand {
 // TODO: Add your data members
 public:
  HistoryCommand(const char* cmd_line, CommandsHistory* history);
  virtual ~HistoryCommand() {}
  void execute() override;
};*/

class JobsCommand : public BuiltInCommand {
public:
    JobsCommand(string cmd_line, SmallShell* smash);
    virtual ~JobsCommand() {}
    void execute() override;
};

class KillCommand : public BuiltInCommand {
    signal_t signum = -1;
    job_id_t jobId = -1;
public:
    bool verbose = true;
    KillCommand(string cmd_line, SmallShell* smash);
    virtual ~KillCommand() {}
    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
private:
    job_id_t jobId;
    ProcessControlBlock* pcb = nullptr;

public:
    ForegroundCommand(string cmd_line, SmallShell* smash);
    virtual ~ForegroundCommand() {}
    void execute() override;
};

class BackgroundCommand : public BuiltInCommand {
    job_id_t jobId;
    ProcessControlBlock* pcb = nullptr;

public:
    BackgroundCommand(string cmd_line, SmallShell* smash);
    virtual ~BackgroundCommand() {}
    void execute() override;
};


// TODO: should it really inherit from BuiltInCommand ?
class CopyCommand : public BuiltInCommand {
public:
    CopyCommand(string cmd_line, SmallShell* smash);
    virtual ~CopyCommand() {}
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

// TODO: add more classes if needed 
// maybe chprompt , timeout ?

namespace SmashExceptions{
    class Exception : public std::exception {};
    class SignalSendException : Exception {};
    class NoStoppedJobsException : Exception {};
}


#endif //SMASH_COMMAND_H_
