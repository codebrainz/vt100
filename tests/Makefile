# Makefile for vt100 unit tests

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -I../src
LDFLAGS = -L../src -lvt100

SRCS = vt100_test.c
OBJS = $(SRCS:.c=.o)
TARGET = vt100_test

all: $(TARGET)

$(TARGET): $(OBJS) ../src/libvt100.a
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

.PHONY: all clean
