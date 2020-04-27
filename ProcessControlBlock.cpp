//
// Created by AKIVA on 12/04/2020.
//

#include "ProcessControlBlock.h"

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

ProcessControlBlock* ProcessControlBlock::getYoungestSon() const {
    return youngestSon;
}

ProcessControlBlock* ProcessControlBlock::getYoungerBrother() const {
    return youngerBrother;
}

ProcessControlBlock* ProcessControlBlock::getOlderBrother() const {
    return olderBrother;
}

ProcessControlBlock* ProcessControlBlock::getFather() const {
    return father;
}

void ProcessControlBlock::setRunning(bool running) {
    ProcessControlBlock::running = running;
}

void ProcessControlBlock::setYoungestSon(ProcessControlBlock& youngestSon) {
    ProcessControlBlock::youngestSon = &youngestSon;
}

void ProcessControlBlock::setYoungerBrother(ProcessControlBlock &youngerBrother) {
    ProcessControlBlock::youngerBrother = &youngerBrother;
}

void ProcessControlBlock::setOlderBrother(ProcessControlBlock &olderBrother) {
    ProcessControlBlock::olderBrother = &olderBrother;
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

std::ostream& operator<<(std::ostream &outstream, ProcessControlBlock &pcb) {
    return outstream << pcb.getCreatingCommand() << " : " << pcb.getProcessId();
}
