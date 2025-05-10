CC = gcc
CFLAGS = -Wall -Wextra -pthread
OBJS = webserver.o webserver_functions.o

all: webserver

webserver: $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

webserver.o: webserver.c webserver.h
	$(CC) $(CFLAGS) -c $

webserver_functions.o: webserver_functions.c webserver.h
	$(CC) $(CFLAGS) -c $

clean:
	rm -f webserver $(OBJS)

.PHONY: all clean