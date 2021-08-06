# Sandu Stefania-Cristina 324CA
# Pcom - Tema 2
# Makefile

CFLAGS = -Wall -g
CC = g++
all: server subscriber

# Compileaza server.c
server: server.cpp
	$(CC) $(CFLAGS) server.cpp -o server

# Compileaza subscriber.c
subscriber: subscriber.cpp
	$(CC) $(CFLAGS) subscriber.cpp -o subscriber

clean:
	rm -f server subscriber
