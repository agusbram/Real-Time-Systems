#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

int main(int argc, char** argv) {
	char *args[] = {"echo", "Hello", "World!", 0};

	execvp("echo", args);

	printf("execvp() failed. Error: %s\n", strerror(errno));

	return 0;
}
