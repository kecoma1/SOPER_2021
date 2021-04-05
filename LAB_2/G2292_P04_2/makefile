all: clean conc_cycle sig_kill

conc_cycle:
	gcc -g -c conc_cycle.c
	gcc -g -o conc_cycle conc_cycle.c -lpthread
	
sig_kill:
	gcc -g -c sig_kill.c
	gcc -g -o sig_kill sig_kill.c -lpthread

clean:
	rm -f *.o conc_cycle sig_kill

test: clean conc_cycle
	./conc_cycle 5

val: clean conc_cycle
	valgrind --leak-check=full ./conc_cycle 5
