//
// Created by AKIVA on 12/04/2020.
//

#ifndef OS_HW1_CONSOLE_H
#define OS_HW1_CONSOLE_H

#include <sys/types.h>
#include <unistd.h>
#include <string>
#include <signal.h>
#include <map>
#include <queue>

#include "ProcessControlBlock.h"

using std::map;
using std::queue;
using std::string;

typedef unsigned short job_id_t;
typedef unsigned short signal_t;

class Console {
private:
    //Dictionary mapping job_id to process
    map<job_id_t, ProcessControlBlock> processes;

    queue<ProcessControlBlock> runQueue;
    queue<ProcessControlBlock> waitingQueue;

    //Builtin commands ----------------------
public:
    void chprompt(const string newPrompt = "smash");

    void showpid();

    void pwd();

    void cd(const string newDir);

    void jobs();

    void kill(const signal_t signum, const job_id_t jobId);

    void fg(const job_id_t jobId);

    void bg(const job_id_t jobId);

    void quit(const string kill = "");

    void externalCommand(const string command);


    //Special commands ----------------------
public:
    //TODO: pipe
    //TODO: IO redirection

    void cp(const string oldFilePath, const string newFilePath);

    void timeout(const unsigned int duration, const string command);


    //Signal handling ----------------------
public:
    //TODO
};


#endif //OS_HW1_CONSOLE_H
