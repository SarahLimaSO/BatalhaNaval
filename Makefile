
CC = gcc
CFLAGS = -Wall

all: battleserver battleclient gameFeatures

battleserver: server/battleserver.c common/protocol.h
	$(CC) $(CFLAGS) -o server/battleserver server/battleserver.c

battleclient: client/battleclient.c common/protocol.h
	$(CC) $(CFLAGS) -o client/battleclient client/battleclient.c

gameFeatures: common/gameFeatures.c common/gameFeatures.h
	$(CC) $(CFLAGS) -c common/gameFeatures.c -o common/gameFeatures.o
	
clean:
	rm -f server/battleserver client/battleclient
