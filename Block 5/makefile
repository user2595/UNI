CC=gcc
ARGSPEER1= 127.0.0.1 8000 1
ARGSPEER2= 127.0.0.1 8001 100 127.0.0.1 8000
ARGSPEER3= 127.0.0.1 8002 50 127.0.0.1 8000

all: peer

runpeer1: peer
	./peer $(ARGSPEER1)

runpeer2: peer
	./peer $(ARGSPEER2)

runpeer3: peer
	./peer $(ARGSPEER3)


peer: peer.c
	$(CC) -o peer peer.c


clean:
	rm peer