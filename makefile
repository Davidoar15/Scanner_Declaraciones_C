CC = gcc
CFLAGS = -std=c11 -Wall

all: scanner parser

scanner: Scanner.o mainScanner.o
	$(CC) $(CFLAGS) -o scanner Scanner.o mainScanner.o

parser: Scanner.o Parser.o mainParser.o
	$(CC) $(CFLAGS) -o parser Scanner.o Parser.o mainParser.o


Scanner.o: Scanner.c Scanner.h
	$(CC) $(CFLAGS) -c Scanner.c -o Scanner.o

mainScanner.o: mainScanner.c Scanner.h
	$(CC) $(CFLAGS) -c mainScanner.c -o mainScanner.o

Parser.o: Parser.c Parser.h
	$(CC) $(CFLAGS) -c Parser.c -o Parser.o

mainParser.o: mainParser.c Parser.h Scanner.h
	$(CC) $(CFLAGS) -c mainParser.c -o mainParser.o

clean:
	rm -f *.o scanner parser

.PHONY: all clean