CC = gcc -std=c99
# -D_GNU_SOURCE -D_POSIX_C_SOURCE=199309L
# uncomment when compiling for linux
CC-DEBUG = $(CC) -Wall -ggdb -DDEBUG
CC-RELEASE = $(CC) -g -fsanitize=address #-O3
LDFLAGS = -lpthread

all: client server
debug: client-debug server-debug

server: server/server.c shared/semaphore.c shared/protocol.c
	$(CC-RELEASE) -c server/server.c    -o server/server.o
	$(CC-RELEASE) -c shared/semaphore.c -o shared/semaphore.o
	$(CC-RELEASE) -c shared/protocol.c  -o shared/protocol.o
	$(CC-RELEASE) -c shared/log.c       -o shared/log.o
	$(CC-RELEASE) -o server/server server/server.o shared/semaphore.o shared/log.o shared/protocol.o $(LDFLAGS)

client: client/client.c shared/protocol.c
	$(CC-RELEASE) -c client/client.c -o client/client.o
	$(CC-RELEASE) -c shared/protocol.c -o shared/protocol.o
	$(CC-RELEASE) -c shared/log.c       -o shared/log.o
	$(CC-RELEASE) -o client/mychannel client/client.o shared/protocol.o shared/log.o $(LDFLAGS)

server-debug: server/server.c shared/semaphore.c shared/protocol.c
	$(CC-DEBUG) -c server/server.c    -o server/server.o
	$(CC-DEBUG) -c shared/semaphore.c -o shared/semaphore.o
	$(CC-DEBUG) -c shared/protocol.c  -o shared/protocol.o
	$(CC-DEBUG) -c shared/log.c       -o shared/log.o
	$(CC-DEBUG) -o server/server server/server.o shared/semaphore.o shared/log.o shared/protocol.o $(LDFLAGS)

client-debug: client/client.c shared/protocol.c
	$(CC-DEBUG) -c client/client.c -o client/client.o
	$(CC-DEBUG) -c shared/protocol.c -o shared/protocol.o
	$(CC-DEBUG) -c shared/log.c       -o shared/log.o
	$(CC-DEBUG) -o client/mychannel client/client.o shared/protocol.o shared/log.o $(LDFLAGS)

.PHONY: clean
clean:
	rm -f server/*.o client/*.o shared/*.o server/server client/mychannel
