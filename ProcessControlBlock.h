//
// Created by AKIVA on 12/04/2020.
//

#ifndef OS_HW1_PROCESSCONTROLBLOCK_H
#define OS_HW1_PROCESSCONTROLBLOCK_H

#include <stdbool.h>
#include <string>
#include <ostream>

typedef int job_id_t;

class ProcessControlBlock {
protected:
    //process data
    job_id_t jobId;
    pid_t processId;
    pid_t processGroupId;
    bool running = true;
    const std::string creatingCommand;
    time_t startTime;

public:
    void setJobId(job_id_t jobId);

    time_t getStartTime() const;

    void setStartTime(time_t new_time);

    void resetStartTime();

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
                        const std::string &creatingCommand);

    pid_t getProcessGroupId() const;

    // ROI - added default to avoid error in build
    virtual ~ProcessControlBlock() = default;
};


class TimedProcessControlBlock : public ProcessControlBlock {
private:
    //ROI - field for timed process
    time_t abortTime;

public:
    TimedProcessControlBlock(const job_id_t jobId,
                             const pid_t processId,
                             const std::string &creatingCommand, int futureSeconds);

    time_t getAbortTime() const;

    bool operator<(const TimedProcessControlBlock &rhs) const;

    bool operator>(const TimedProcessControlBlock &rhs) const;

    bool operator<=(const TimedProcessControlBlock &rhs) const;

    bool operator>=(const TimedProcessControlBlock &rhs) const;

    bool operator==(const TimedProcessControlBlock &rhs) const;

    bool operator!=(const TimedProcessControlBlock &rhs) const;

    virtual ~TimedProcessControlBlock() = default;
};


std::ostream& operator<<(std::ostream& outstream, ProcessControlBlock& pcb);


#endif //OS_HW1_PROCESSCONTROLBLOCK_H
