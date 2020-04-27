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
            if (kill(foregroundProcess->getProcessId(), SIGSTOP) < 0) {
                cerr << "smash error: kill failed" << endl;
                return;
            }
            cout << "smash: process " << foregroundProcess->getProcessId() << " was stopped" << endl;

            //log that process is stopped
            const_cast<ProcessControlBlock *>(foregroundProcess)->setRunning(false);

            //add foreground command to jobs
            shell->jobs.addJob(*foregroundProcess);

            /*
            //DEBUG
            if (foregroundProcess) {
                if (kill(foregroundProcess->getProcessId(), SIGKILL) < 0) {
                    cerr << "smash error: kill failed" << endl;
                    return;
                }
                cout << "smash: process " << foregroundProcess->getProcessId() << " was stopped" << endl;
            }*/
            //DEBUG
        }
    }


    void ctrlCHandler(int sig_num) {
        cout << "smash: got ctrl-C" << endl;

        //send SIGKILL to foreground process
        const ProcessControlBlock *foregroundProcess = shell->getForegroundProcess();
        if (foregroundProcess) {
            if (kill(foregroundProcess->getProcessId(), SIGKILL) < 0) {
                cerr << "smash error: kill failed" << endl;
                return;
            }
            cout << "smash: process " << foregroundProcess->getProcessId() << " was killed" << endl;
        }
    }

    void alarmHandler(int sig_num) {
        cout << "smash: got an alarm" << endl;

        //find command that cause alarm
        ProcessControlBlock* lateProcess = shell->getLateProcess();

        //send SIGKILL
        if (kill(lateProcess->getProcessId(), SIGKILL) < 0) {
          cerr << "smash error: kill failed" << endl;
          return;
          }

        cout << "smash: " << lateProcess->getCreatingCommand() << " timed out!" << endl;
        shell->RemoveLateProcess(lateProcess->getProcessId());

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


        SmallShell &smash = SmallShell::getInstance();
        shell = &smash;
        while (true) {
            std::cout << smash.getSmashPrompt();
            std::string cmd_line;
            std::getline(std::cin, cmd_line);
            smash.jobs.removeFinishedJobs();
            smash.executeCommand(cmd_line);
        }
        return 0;
    }