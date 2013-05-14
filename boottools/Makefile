CC=gcc
CFLAGS=-Wall -Wextra -pedantic -std=c99 -D_BSD_SOURCE

.PHONY: clean

all: pack unpack

clean:
	rm -f *.o pack unpack

pack: pack.o
	$(CC) -o $@ $< $(CFLAGS)

unpack: unpack.o
	$(CC) -o $@ $< $(CFLAGS)
