all: clean miner.o trabajador.o exe

miner.o:
	gcc -g -c miner.c -lpthread

trabajador.o:
	gcc -g -c trabajador.c

exe:
	gcc miner.o trabajador.o -o miner -lpthread

clean:
	rm -f *.o miner

