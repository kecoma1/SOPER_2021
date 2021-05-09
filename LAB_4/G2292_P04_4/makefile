all: clean miner.o trabajador.o block.o net.o sems.o monitor.o miner monitor

miner.o:
	gcc -g -c miner.c -lpthread

trabajador.o:
	gcc -g -c trabajador.c

block.o:
	gcc -g -c block.c 

net.o:
	gcc -g -c net.c

sems.o:
	gcc -g -c sems.c

monitor.o:
	gcc -g -c monitor.c

miner:
	gcc -g miner.o trabajador.o block.o net.o sems.o -o miner -lpthread -lrt

monitor:
	gcc -g trabajador.o block.o net.o sems.o monitor.o -o monitor -lpthread -lrt

clean:
	rm -f *.o miner monitor

valgrind:
	valgrind --leak-check=full --show-leak-kinds=all ./miner 1 4

