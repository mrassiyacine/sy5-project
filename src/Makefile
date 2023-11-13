CC=gcc
CFLAGS=-Wall -g
LDFLAGS=
EXEC=tsh

all: $(EXEC)

tsh: tsh.o cmd.o cmd_tar.o main.o
	$(CC) -o tsh tsh.o cmd.o cmd_tar.o main.o
	
	
main.o: main.c tsh.h cmd.h cmd_tar.h
	$(CC) -o main.o -c main.c $(CFLAGS)
	
clean:
	rm -rf *.o

mrproper: clean
	rm -rf $(EXEC)

