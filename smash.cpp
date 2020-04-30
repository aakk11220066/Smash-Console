#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <sys/wait.h>
#include <signal.h>
#include "Commands.h"

#define DEBUG_PRINT(err_msg) cerr << "DEBUG: " << err_msg //debug

using namespace std;

SmallShell* shell = nullptr;

namespace SignalHandlers {
    void ctrlZHandler(int sig_num) {
        cout << "smash: got ctrl-Z" << endl;

        //send SIGSTOP to foreground process
        const ProcessControlBlock *foregroundProcess = shell->getForegroundProcess();
        if (foregroundProcess) {

            //stop process
            try{
                ::sendSignal(*foregroundProcess, SIGSTOP);
            } catch (SmashExceptions::SyscallException& error){
                cerr << error.what() << endl;
            }

            cout << "smash: process " << foregroundProcess->getProcessId() << " was stopped" << endl;

            //log that process is stopped
            const_cast<ProcessControlBlock*>(foregroundProcess)->setRunning(false);

            //add foreground command to jobs
            shell->jobs.addJob(*foregroundProcess);
        }
    }

    void ctrlCHandler(int sig_num) {
        cout << "smash: got ctrl-C" << endl;

        //send SIGKILL to foreground process
        const ProcessControlBlock *foregroundProcess = shell->getForegroundProcess();
        if (foregroundProcess) {
            try{
                ::sendSignal(*foregroundProcess, SIGKILL);
            } catch (SmashExceptions::SyscallException& error){
                cerr << error.what() << endl;
            }
            cout << "smash: process " << foregroundProcess->getProcessId() << " was killed" << endl;
        }
    }

    void alarmHandler(int sig_num) {

        // do we need to print we got an alarm anyway? - we do
        cout << "smash: got an alarm" << endl;
        TimedProcessControlBlock *lateProcess = shell->getLateProcess();

        //send SIGKILL
        // i marked built-in command with " " as their command line
        if (lateProcess && lateProcess->getCreatingCommand() != " ") {
            //AKIVA: use ::sendSignal instead of kill to send actionable signals
            try{
                ::sendSignal(*lateProcess, SIGKILL);
            } catch (SmashExceptions::SyscallException& error){
                cerr << error.what() << endl;
            }
            cout << "smash: " << lateProcess->getCreatingCommand() << " timed out!" << endl;
        }
            // general remove from job list
            shell->RemoveLateProcesses();
            //set signal alarm for next process in the list
            shell->jobs.setAlarmSignal();
        }
}

    int main(int argc, char *argv[]) {
        //set sigaction struct

        struct sigaction action;
        action.sa_handler = SignalHandlers::alarmHandler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_RESTART;


        DEBUG_PRINT("this pid is " << getpid() << endl);
        if (signal(SIGTSTP, SignalHandlers::ctrlZHandler) == SIG_ERR) {
            perror("smash error: failed to set ctrl-Z handler");
        }
        if (signal(SIGINT, SignalHandlers::ctrlCHandler) == SIG_ERR) {
            perror("smash error: failed to set ctrl-C handler");
        }

        if(sigaction(SIGALRM , &action, nullptr) == -1) {
            perror("smash error: failed to set alarm signal handler");
        }

    SmallShell& smash = SmallShell::getInstance();
    shell = &smash;
    while(true) {
        std::cout << smash.getSmashPrompt();
        std::string cmd_line;
        std::getline(std::cin, cmd_line);
        smash.jobs.removeFinishedJobs();
        smash.executeCommand(cmd_line);
    }
    return 0;
}