#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define NUM_PROC 3

int main(void) {
	int i;
	pid_t pid;

	for (i = 0; i < NUM_PROC; i++) {
		pid = fork();
		if (pid <  0) {
			perror("fork");
			exit(EXIT_FAILURE);
		}
		else if (pid == 0) {
			printf("Hijo %d\n", i);
			exit(EXIT_SUCCESS);
		}
		else if (pid > 0) {
			printf("Padre %d\n", i);
		}
	}
	wait(NULL);
	exit(EXIT_SUCCESS);
}