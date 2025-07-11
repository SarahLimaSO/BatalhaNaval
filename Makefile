
CC = gcc
CFLAGS = -Wall

all: battleserver battleclient battleclient_auto

battleserver: server/battleserver.c common/protocol.h 
	$(CC) $(CFLAGS) -o server/battleserver server/battleserver.c 

battleclient: client/battleclient.c common/protocol.h
	$(CC) $(CFLAGS) -o client/battleclient client/battleclient.c

battleclient_auto: client/battleclient_auto.c common/protocol.h
	$(CC) $(CFLAGS) -o client/battleclient_auto client/battleclient_auto.c -pthread

clean:
	rm -f server/battleserver client/battleclient client/battleclient_auto
