all: clean proc_shell

proc_shell:
	gcc -g -c proc_shell.c
	gcc -g -o proc_shell proc_shell.c -pthread

clean:
	rm -f *.o proc_shell

exe: clean proc_shell
	./proc_shell

val: clean proc_shell
	valgrind --leak-check=full ./proc_shell