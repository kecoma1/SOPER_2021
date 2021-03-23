all: clean conc_cycle

conc_cycle:
	gcc -g -c conc_cycle.c
	gcc -g -o conc_cycle conc_cycle.c -lpthread

clean:
	rm -f *.o conc_cycle

test: clean conc_cycle
	./conc_cycle 5

val: clean proc_shell
	valgrind --leak-check=full ./conc_cycle