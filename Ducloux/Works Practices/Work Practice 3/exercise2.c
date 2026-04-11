#include <stdio.h>
#include <stdlib.h>

// The POSIX standard header for using pthread library
#include <pthread.h>

// To use getpid()
#include <unistd.h>

// This function contains the logic which should be run
// as the body of a separate thread
void* thread_body(void* arg) {
    // Obtain thread number, casted previously
    int thread_num = *(int*)arg;

    // Obtaing process id of actual thread
    pid_t pid = getpid();

    // Obtain actual thread id executed
    pthread_t tid = pthread_self();

    printf("Hello from thread number %d!\n", thread_num);

    // pid_t it's int type
    printf("PID: %d\n", pid);

    // pthread_t can be intern type (not always int)
    // This ensures portability
    printf("TID: %lu\n", (unsigned long)tid);

    return NULL;
}

int main(int argc, char** argv) {
    // The thread handler
    pthread_t threads[10];

    // Store IDs of arrays
    int thread_ids[10];

    // Create an array of threads
    for(int i = 0; i < 10; i++) {
        // Create ids of threads to print it in console
        thread_ids[i] = i + 1;
        
        int result = pthread_create(&threads[i], NULL, thread_body, &thread_ids[i]);

        // If the thread creation did not succeed
        if (result) {
            printf("Thread could not be created. Error number: %d\n", result);
            exit(1);
        }
    }

    // Wait for all threads to finish
    for (int i = 0; i < 10; i++) {
        int result = pthread_join(threads[i], NULL);

        // If joining the thread did not succeed
        if (result) {
            printf("The thread could not be joined. Error number: %d\n", result);
            exit(2);
        }
    }

    return 0;
}
