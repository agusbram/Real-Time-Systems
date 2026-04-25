#define _GNU_SOURCE
#include <stdio.h> // Standard Input-Output 
#include <stdlib.h> // General system functions
#include <unistd.h> // POSIX Interface for system calls
#include <sys/resource.h> // Prioritie's handler of NICE type
#include <sched.h> // Scheduler process
#include <sys/wait.h> // Finalization control of sons processes

#define SONS_NUM 3

void print_info() {
    // Initialize policy of threads
    int policy;

    // Initialize structure to define schedulity
    struct sched_param sp;

    // Initialize NICE points
    int nice;

    // Obtain actual scheduler of thread that calls this function
    policy = sched_getscheduler(0);

    // Obtain parameter of scheduling structure of thread that calls this function
    sched_getparam(0, &sp);

    // Obtain priority of actual thread that calls this function
    nice = getpriority(PRIO_PROCESS, 0);

    // Obtain actual process ID 
    printf("PID: %d | ", getpid());

    // Prints actual policy of this thread that calls this function
    switch(policy){
        case SCHED_OTHER: printf("Policy: SCHED_OTHER | "); break;
        case SCHED_FIFO:  printf("Policy: SCHED_FIFO | "); break;
        case SCHED_RR:    printf("Policy: SCHED_RR | "); break;
        default:          printf("Policy: UNKNOWN | "); break;
    }

    // Prints actual priority & NICE of this thread that calls this function
    printf("Priority: %d | Nice: %d\n", sp.sched_priority, nice);
}

int main() {
    // Initalize process type of sons threads
    pid_t sons[SONS_NUM];

    // Create sons threads
    for (int i = 0; i < SONS_NUM; i++) {
        sons[i] = fork();

        // This executes the son process
        if (sons[i] == 0) {
            while (1) {
                print_info();
                sleep(1);
            }
            exit(0);
        }
    }

    // Father waits 10 seconds
    sleep(10);

    printf("\nFather changing priorities...\n\n");

    // Change scheduling policy and priority of each child
    for (int i = 0; i < SONS_NUM; i++) {
        struct sched_param sp;

        // Assign different real-time priorities
        sp.sched_priority = 10 + i * 20; // 10, 30, 50

        // Set scheduling policy to FIFO (real-time)
        if (sched_setscheduler(sons[i], SCHED_FIFO, &sp) == -1) {
            perror("Error sched_setscheduler (need sudo)");
        } else {
            printf("Son PID %d new RT priority: %d (SCHED_FIFO)\n",
                   sons[i], sp.sched_priority);
        }

        // Also change nice value for comparison
        int new_nice = 5 + i * 5;

        if (setpriority(PRIO_PROCESS, sons[i], new_nice) == -1) {
            perror("Error setpriority");
        } else {
            printf("Son PID %d new nice: %d\n", sons[i], new_nice);
        }
    }

    // Wait (optional)
    wait(NULL);
    return 0;
}