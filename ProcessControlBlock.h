//
// Created by AKIVA on 12/04/2020.
//

#ifndef OS_HW1_PROCESSCONTROLBLOCK_H
#define OS_HW1_PROCESSCONTROLBLOCK_H

#include <stdbool.h>
#include <string>
#include <ostream>

typedef unsigned int job_id_t;

class ProcessControlBlock {
private:
    //process data
    const job_id_t jobId;
    pid_t processId;
    bool running = true;
    const std::string creatingCommand;
    time_t startTime;

    std::unique_ptr<std::unique_ptr<char>> args;
    unsigned short numArgs = 0;
public:
    time_t getStartTime() const;

    pid_t getProcessId() const;

    void setProcessId(pid_t processId);

    bool operator<(const ProcessControlBlock &rhs) const;

    bool operator>(const ProcessControlBlock &rhs) const;

    bool operator<=(const ProcessControlBlock &rhs) const;

    bool operator>=(const ProcessControlBlock &rhs) const;

public:
    bool operator==(const ProcessControlBlock &rhs) const;

    bool operator!=(const ProcessControlBlock &rhs) const;

public:
    void setRunning(bool running);

    void setYoungestSon(ProcessControlBlock &youngestSon);

    void setYoungerBrother(ProcessControlBlock &youngerBrother);

    void setOlderBrother(ProcessControlBlock &olderBrother);

public:
    ProcessControlBlock* getYoungestSon() const;

    ProcessControlBlock* getYoungerBrother() const;

    ProcessControlBlock* getOlderBrother() const;

    ProcessControlBlock &getFather() const;

public:
    const job_id_t getJobId() const;

    bool isRunning() const;

    const std::string &getCreatingCommand() const;

private:

    //family
    ProcessControlBlock* youngestSon = nullptr;
    ProcessControlBlock* youngerBrother = nullptr;
    ProcessControlBlock* olderBrother = nullptr;
    ProcessControlBlock& father;

public:
    ProcessControlBlock(const job_id_t jobId,
                        const pid_t processId,
                        const std::string &creatingCommand,
                        char** args,
                        unsigned short numArgs,
                        ProcessControlBlock &father,
                        ProcessControlBlock* youngerBrother,
                        ProcessControlBlock* olderBrother);
};

std::ostream& operator<<(std::ostream& outstream, ProcessControlBlock& pcb);

#endif //OS_HW1_PROCESSCONTROLBLOCK_H
