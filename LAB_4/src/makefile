all: clean miner.o trabajador.o block.o exe

miner.o:
	gcc -g -c miner.c -lpthread

trabajador.o:
	gcc -g -c trabajador.c

block.o:
	gcc -g -c block.c

exe:
	gcc -g miner.o trabajador.o -o miner -lpthread

clean:
	rm -f *.o miner

