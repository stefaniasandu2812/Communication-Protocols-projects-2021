#Sandu Stefania-Cristina 324CA

CC=gcc
CFLAGS=-I.

client: client.c requests.c parson.c helpers.c buffer.c
	$(CC) -o client client.c requests.c parson.c helpers.c buffer.c -Wall

run: client
	./client

clean:
	rm -f *.o client
