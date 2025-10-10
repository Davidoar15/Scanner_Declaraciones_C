CC = gcc
CFLAGS = -std=c11 -Wall

all: scanner

scanner: Scanner.o main.o
	$(CC) $(CFLAGS) -o scanner Scanner.o main.o

Scanner.o: Scanner.c Scanner.h
	$(CC) $(CFLAGS) -c Scanner.c

main.o: main.c Scanner.h
	$(CC) $(CFLAGS) -c main.c

clean:
	rm -f *.o scanner