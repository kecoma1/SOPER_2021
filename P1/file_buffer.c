#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
	pid_t pid;

	printf("Yo soy tu padre");

	pid = fork();
	if (pid <  0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	else if (pid == 0) {
		printf("Noooooo");
		exit(EXIT_SUCCESS);
	}

	wait(NULL);
	exit(EXIT_SUCCESS);
}
