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
    job_id_t jobId;
    pid_t processId;
    bool running = true;
    const std::string creatingCommand;
    time_t startTime;

public:
    void setJobId(job_id_t jobId);
    //ROI - field for timed process
    int duration;

    time_t getStartTime() const;

    pid_t getProcessId() const;

    void setProcessId(pid_t processId);

    bool operator<(const ProcessControlBlock &rhs) const;

    bool operator>(const ProcessControlBlock &rhs) const;

    bool operator<=(const ProcessControlBlock &rhs) const;

    bool operator>=(const ProcessControlBlock &rhs) const;

    bool operator==(const ProcessControlBlock &rhs) const;

    bool operator!=(const ProcessControlBlock &rhs) const;

    void setRunning(bool running);

    const job_id_t getJobId() const;

    bool isRunning() const;

    const std::string &getCreatingCommand() const;

    ProcessControlBlock(const job_id_t jobId,
                        const pid_t processId,
                        const std::string &creatingCommand, const int duration = -1);

    virtual ~ProcessControlBlock();
};

std::ostream& operator<<(std::ostream& outstream, ProcessControlBlock& pcb);


#endif //OS_HW1_PROCESSCONTROLBLOCK_H
