CC = gcc -Wall -g
LDFLAGS = -lpthread

all: client server

server: server/server.h server/server.c server/semaphore.h server/semaphore.c
	$(CC) -c server/server.c -o server/server.o
	$(CC) -c server/semaphore.c -o server/semaphore.o
	$(CC) -o server/server server/server.o server/semaphore.o

client: client/client.c client/client.h
	$(CC) -c client/client.c -o client/client.o
	$(CC) -o client/mychannel client/client.o

clean:
	rm -f server/*.o client/*.o server/server client/mychannel
