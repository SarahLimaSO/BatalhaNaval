
CC = gcc
CFLAGS = -Wall

all: battleserver battleclient

battleserver: server/battleserver.c common/protocol.h common/gameFeatures.c
	$(CC) $(CFLAGS) -o server/battleserver server/battleserver.c common/gameFeatures.c -lpthread

battleclient: client/battleclient.c common/gameFeatures.c common/protocol.h common/gameFeatures.h
	$(CC) $(CFLAGS) -o client/battleclient client/battleclient.c common/gameFeatures.c

clean:
	rm -f server/battleserver client/battleclient
