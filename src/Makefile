# Makefile for vt100 library

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -fPIC
AR = ar
ARFLAGS = rcs

LIB = libvt100.a
OBJS = vt100.o

all: $(LIB)

$(LIB): $(OBJS)
	$(AR) $(ARFLAGS) $@ $^

vt100.o: vt100.c vt100.h
	$(CC) $(CFLAGS) -c vt100.c -o vt100.o

clean:
	rm -f $(OBJS) $(LIB)

.PHONY: all clean
