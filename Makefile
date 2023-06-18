CC=gcc
CFLAGS=-g -static -std=c11 -Wall -pedantic -I. -lm -lafbeelding
LDFLAGS=-L/usr/lib
OBJS=wacker.o wad.o

all: wacker

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS)

wacker: $(OBJS)
	$(CC) -o $@ $(OBJS) $(CFLAGS)

.PHONY: clean

clean:
	$(RM) *.o wacker wacker.com
