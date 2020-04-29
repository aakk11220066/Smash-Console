//
// Created by AKIVA on 12/04/2020.
//

#include "ProcessControlBlock.h"
#include <signal.h>
#include <iostream>
#include <cstring>

#define DEBUG_PRINT(err_msg) std::cerr << "DEBUG: " << err_msg << std::endl //debug

ProcessControlBlock::ProcessControlBlock(const job_id_t jobId,
                                         const pid_t processId,
                                         const std::string& creatingCommand,
                                         const int duration) :

                                         jobId(jobId),
                                         creatingCommand(creatingCommand),
                                         startTime(time(nullptr)),
                                         processId(processId),
                                         //ROI field for timed processes
                                         duration(duration){

}

const job_id_t ProcessControlBlock::getJobId() const {
    return jobId;
}

bool ProcessControlBlock::isRunning() const {
    return running;
}

const std::string &ProcessControlBlock::getCreatingCommand() const {
    return creatingCommand;
}

void ProcessControlBlock::setRunning(bool running) {
    ProcessControlBlock::running = running;
}

bool ProcessControlBlock::operator==(const ProcessControlBlock &rhs) const {
    return jobId == rhs.jobId;
}

bool ProcessControlBlock::operator!=(const ProcessControlBlock &rhs) const {
    return !(rhs == *this);
}

bool ProcessControlBlock::operator<(const ProcessControlBlock &rhs) const {
    return jobId < rhs.jobId;
}

bool ProcessControlBlock::operator>(const ProcessControlBlock &rhs) const {
    return rhs < *this;
}

bool ProcessControlBlock::operator<=(const ProcessControlBlock &rhs) const {
    return !(rhs < *this);
}

bool ProcessControlBlock::operator>=(const ProcessControlBlock &rhs) const {
    return !(*this < rhs);
}

pid_t ProcessControlBlock::getProcessId() const {
    return processId;
}

void ProcessControlBlock::setProcessId(pid_t processId) {
    ProcessControlBlock::processId = processId;
}

time_t ProcessControlBlock::getStartTime() const {
    return startTime;
}
//ROI
void ProcessControlBlock::setStartTime(time_t new_time){
    startTime = new_time;
}

void ProcessControlBlock::setJobId(job_id_t jobId) {
    ProcessControlBlock::jobId = jobId;
}
/*
//ROI
void ProcessControlBlock::setCreatingCommand(std::string command) {
    ProcessControlBlock::creatingCommand = command;

}
 */

std::ostream& operator<<(std::ostream &outstream, ProcessControlBlock &pcb);
ProcessControlBlock::~ProcessControlBlock() {
    /*DEBUG_PRINT("Abruptly killing process "<<*this<<" with process id "<<getProcessId()<<"...");
    if (kill(getProcessId(), SIGKILL) < 0 && errno!=3) {
        DEBUG_PRINT("pid = " + std::to_string(getProcessId()) + " and kill failed with errno = "+std::to_string(errno) + " which is "+std::strerror(errno));
        std::cerr << "smash error: kill failed" << std::endl;
    }
    DEBUG_PRINT("...killed process with id "<<getProcessId()<<".");*/
}

void ProcessControlBlock::resetStartTime() {
    startTime = time(nullptr);
}

std::ostream& operator<<(std::ostream &outstream, ProcessControlBlock &pcb) {
    return outstream << pcb.getCreatingCommand() << " : " << pcb.getProcessId();
}
