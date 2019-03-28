CC=gcc

all: client clientudp clienthttp

client: client.c
	$(CC) -o client client.c

clientudp: clientudp.c
	$(CC) -o clientudp clientudp.c

clienthttp: clienthttp.c
		$(CC) -o clienthttp clienthttp.c

clean:
	rm client clientudp clienthttp
