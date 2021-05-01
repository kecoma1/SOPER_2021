all: clean miner.o trabajador.o block.o exe

miner.o:
	gcc -g -c miner.c -lpthread

trabajador.o:
	gcc -g -c trabajador.c

block.o:
	gcc -g -c block.c 

exe:
	gcc -g miner.o trabajador.o block.o -o miner -pthread -lrt

clean:
	rm -f *.o miner

valgrind:
	valgrind --leak-check=full ./miner 10 3

