#include <stdio.h>
#include <stdlib.h>

// The POSIX standard header for using pthread library
#include <pthread.h>

// To use getpid()
#include <unistd.h>

// This function contains the logic which should be run
// as the body of a separate thread
void* thread_body(void* arg) {
    // Obtaing process id of actual thread
    pid_t pid = getpid();

    // Obtain actual thread id executed
    pthread_t tid = pthread_self();

    printf("Hello from first thread!\n");

    // pid_t it's int type
    printf("PID: %d\n", pid);

    // pthread_t can be intern type (not always int)
    // This ensures portability
    printf("TID: %lu\n", (unsigned long)tid);

    return NULL;
}

int main(int argc, char** argv) {
    // The thread handler
    pthread_t thread;

    // Create a new thread
    int result = pthread_create(&thread, NULL, thread_body, NULL);

    // If the thread creation did not succeed
    if (result) {
        printf("Thread could not be created. Error number: %d\n", result);
        exit(1);
    }

    // Wait for the created thread to finish
    result = pthread_join(thread, NULL);

    // If joining the thread did not succeed
    if (result) {
        printf("The thread could not be joined. Error number: %d\n", result);
        exit(2);
    }

    return 0;
}
