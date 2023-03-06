CC=gcc
CFLAGS=-Wall -Wextra -std=c99
DEBUG=-g

build: main Hashtable LinkedList load_balancer server
	$(CC) $(CFLAGS) $(DEBUG) main.o Hashtable.o LinkedList.o load_balancer.o server.o -o tema2

Hashtable: Hashtable.h Hashtable.c
	$(CC) $(CFLAGS) $(DEBUG) Hashtable.c -c -o Hashtable.o

LinkedList: LinkedList.h LinkedList.c
	$(CC) $(CFLAGS) $(DEBUG) LinkedList.c -c -o LinkedList.o

load_balancer: load_balancer.h load_balancer.c
	$(CC) $(CFLAGS) $(DEBUG) load_balancer.c -c -o load_balancer.o

server: server.h server.c
	$(CC) $(CFLAGS) $(DEBUG) server.c -c -o server.o

main: main.c
	$(CC) $(CFLAGS) $(DEBUG) main.c -c -o main.o

run:
	./tema2

clean:
	rm *.o
	rm tema2