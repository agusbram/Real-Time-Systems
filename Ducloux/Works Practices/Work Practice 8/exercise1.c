#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>

int main(int argc, char* argv[]) {
    // Verify if the PID was passed to the command line in the shell
    if(argc != 2) {
        fprintf(stderr, "Usage: %s <PID>\n", argv[0]);
        return 1;
    }

    // Convert argument to integer
    pid_t pid = atoi(argv[1]);

    // Send SIGINT to the program ejemplo1.c
    if(kill(pid, SIGINT) == -1) {
        perror("Error sending SIGINT");
        return 1;
    }

    printf("SIGINT sent to process %d\n", pid);

    return 0;
}