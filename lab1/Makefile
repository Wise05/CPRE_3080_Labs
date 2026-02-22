CC = gcc
CFLAGS = -Wall -g

lab1: lab1.o message.o
	$(CC) $(CFLAGS) -o lab1 lab1.o message.o

lab1.o: lab1.c message.h
	$(CC) $(CFLAGS) -c lab1.c

message.o: message.c message.h
	$(CC) $(CFLAGS) -c message.c

clean:
	rm -f lab1 *.o
