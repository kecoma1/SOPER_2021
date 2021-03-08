#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main(void) {
	pid_t pid;
	FILE *pf = fopen("archivo.txt", "w");

	fprintf(pf, "Yo soy tu padre\n");

	pid = fork();
	if (pid <  0) {
		perror("fork");
		exit(EXIT_FAILURE);
	}
	else if (pid == 0) {
		fprintf(pf,"Noooooo");
		fclose(pf);
		exit(EXIT_SUCCESS);
	}

	wait(NULL);
	fclose(pf);
	exit(EXIT_SUCCESS);
}
