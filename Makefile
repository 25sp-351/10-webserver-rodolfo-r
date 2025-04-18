CC = gcc
CFLAGS = -Wall -Wextra
LDFLAGS = -lpthread

all: echo_server

# Multi-threaded version
echo_server: echo_server.c
	$(CC) $(CFLAGS) -o echo_server echo_server.c $(LDFLAGS)

clean:
	rm -f echo_server

.PHONY: all clean