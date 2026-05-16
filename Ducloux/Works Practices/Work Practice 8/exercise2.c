#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>

int main(int argc, char** argv) {
    // Pipe son --> father
    int pipe_child_parent[2];

    // Pipe father --> son
    int pipe_parent_child[2];

    // Create both pipes
    pipe(pipe_child_parent);
    pipe(pipe_parent_child);

    int childpid = fork();
    if(childpid == -1) {
        fprintf(stderr, "fork error!\n");
        exit(1);
    }

    if(childpid == 0) {
        // SON
        // Close extremes not used
        close(pipe_child_parent[0]); // No read
        close(pipe_parent_child[1]); // No write

        char msg_to_parent[] = "Hello Daddy!";
        char buffer[64];

        fprintf(stdout, "CHILD: Waiting 2 seconds...\n");
        sleep(2);

        // Write to the father
        fprintf(stdout, "CHILD: Sending message to parent...\n");
        write(pipe_child_parent[1], msg_to_parent, strlen(msg_to_parent) + 1);

        // Read response of father
        fprintf(stdout, "CHILD: Waiting response from parent...\n");

        read(pipe_parent_child[0], buffer, sizeof(buffer));

        fprintf(stdout, "CHILD: Received from parent: %s\n", buffer);
    } else {
        // FATHER
        // Close extremes not used
        close(pipe_child_parent[1]); // No write
        close(pipe_parent_child[0]); // No read

        char buffer[64];
        char msg_to_child[] = "Hello Son!";

        // Read message of son
        fprintf(stdout, "PARENT: Waiting message from child...\n");

        read(pipe_child_parent[0], buffer, sizeof(buffer));

        fprintf(stdout, "PARENT: Received from child: %s\n", buffer);

        // Response to the son
        fprintf(stdout, "PARENT: Sending response to child...\n");

        write(pipe_parent_child[1], msg_to_child, strlen(msg_to_child) + 1);
    }

    return 0;
}