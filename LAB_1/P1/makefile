all: clean proc_shell

proc_shell:
	gcc -c proc_shell.c
	gcc -o proc_shell proc_shell.c -pthread

clean:
	rm -f *.o proc_shell

exe:
	./proc_shell

val:
	valgrind --leak-check=full ./proc_shell