all: clean proc_shell

proc_shell:
	gcc -c proc_shell.c
	gcc -o proc_shell proc_shell.c -pthread

clean:
	rm -f *.o proc_shell

exe: clean proc_shell
	./proc_shell

val: clean proc_shell
	valgrind --leak-check=full ./proc_shell