CC ?= gcc
CFLAGS = -Wall -Werror
LDFLAGS =

all: aesdsocket

aesdsocket: aesdsocket.c
	$(CC) $(CFLAGS) -o aesdsocket aesdsocket.c $(LDFLAGS)

clean:
	rm -f aesdsocket *.o