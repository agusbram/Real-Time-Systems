#define _GNU_SOURCE
#include <stdio.h>      // Standard Input-Output
#include <stdlib.h>     // General system functions
#include <unistd.h>     // POSIX API (sleep, getpid)
#include <pthread.h>    // Threads (pthreads)
#include <sys/resource.h> // NICE priority handling
#include <sched.h>      // Scheduler
#include <sys/syscall.h> // syscall to get thread ID

#define THREADS_NUM 3

// Global TIDS of threads
pid_t tids[THREADS_NUM];

// Function to get thread ID (TID in Linux)
pid_t gettid() {
    return syscall(SYS_gettid);
}

// Function that prints scheduling info
void print_info() {
    int policy;
    struct sched_param sp;
    int nice;

    // Get scheduling policy of calling thread
    policy = sched_getscheduler(0);

    // Get scheduling parameters of calling thread
    sched_getparam(0, &sp);

    // Get nice value (process-level in Linux)
    nice = getpriority(PRIO_PROCESS, 0);

    // Print process ID and thread ID
    printf("PID: %d | TID: %d | ", getpid(), gettid());

    // Print scheduling policy
    switch(policy){
        case SCHED_OTHER: printf("Policy: SCHED_OTHER | "); break;
        case SCHED_FIFO:  printf("Policy: SCHED_FIFO | "); break;
        case SCHED_RR:    printf("Policy: SCHED_RR | "); break;
        default:          printf("Policy: UNKNOWN | "); break;
    }

    // Print priority and nice value
    printf("Priority: %d | Nice: %d\n", sp.sched_priority, nice);
}

/**
 * Prints priority, NICE & schedule of actual thread
 */
void* thread_function(void* arg) {
    // Obtain index of main process transfered through pthread_create() function
    int index = *(int*)arg;

    // Obtaing actual thread ID & save it into global variable to later use it into setpriority() function
    tids[index] = gettid();

    while (1) {
        print_info();
        sleep(1);
    }
}

int main() {
    // Initialize 3 threads to create
    pthread_t threads[THREADS_NUM];

    // To pass it through pthread_create() function in thread_function()
    // and then know actual thread index
    int indexes[THREADS_NUM];

    // Create threads & use print_info() function to print useless information about them
    for (int i = 0; i < THREADS_NUM; i++) {
        indexes[i] = i;
        if (pthread_create(&threads[i], NULL, thread_function, &indexes[i]) != 0) {
            perror("Error creating thread");
            exit(EXIT_FAILURE);
        }
    }

    // Main thread waits 10 seconds
    sleep(10);

    printf("\nMain thread changing scheduling policies and nice...\n\n");

    // Iterates through the three threads
    for (int i = 0; i < THREADS_NUM; i++) {
        struct sched_param sp;
        int new_nice = 5 + i * 5; // 5, 10, 15

        // ---- SET NICE PER THREAD (using TID) ----
        if (setpriority(PRIO_PROCESS, tids[i], new_nice) == -1) {
            perror("Error setpriority");
        } else {
            printf("Thread %d (TID %d) → nice = %d\n", i, tids[i], new_nice);
        }

        // ---- SET SCHEDULING POLICY ----
        if (i == 0) {
            // Thread 0 ==> FIFO
            sp.sched_priority = 80;

            // Set SCHED_FIFO to thread 0
            if (sched_setscheduler(tids[i], SCHED_FIFO, &sp) == -1) {
                perror("Error setting SCHED_FIFO (need sudo)");
            } else {
                printf("Thread %d → SCHED_FIFO priority %d\n", i, sp.sched_priority);
            }

        } else if (i == 1) {
            // Thread 1 ==> RR
            sp.sched_priority = 60;

            // Set SCHED_FIFO to thread 1
            if (sched_setscheduler(tids[i], SCHED_RR, &sp) == -1) {
                perror("Error setting SCHED_RR (need sudo)");
            } else {
                printf("Thread %d → SCHED_RR priority %d\n", i, sp.sched_priority);
            }

        } else {
            // Thread 2 ==> OTHER
            sp.sched_priority = 0;

            // Set SCHED_FIFO to thread 2
            if (sched_setscheduler(tids[i], SCHED_OTHER, &sp) == -1) {
                perror("Error setting SCHED_OTHER");
            } else {
                printf("Thread %d → SCHED_OTHER\n", i);
            }
        }
    }

    // Join threads (never reached due to infinite loop)
    for (int i = 0; i < THREADS_NUM; i++) {
        pthread_join(threads[i], NULL);
    }

    return 0;
}