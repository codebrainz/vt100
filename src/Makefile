# Makefile for vt100 library

CC = gcc
AR = ar
ARFLAGS = rcs

BUILD_DIR ?= ../build
LIB = ../build/libvt100.a
OBJS = ../build/vt100.o

# Allow SANITIZE=address,undefined, etc.
ifeq ($(SANITIZE),)
  CFLAGS = -Wall -Wextra -std=c99 -O2 -fPIC
else
  CFLAGS = -Wall -Wextra -std=c99 -g -fPIC -fsanitize=$(SANITIZE)
endif


all: $(LIB)

../build:
	mkdir -p ../build

$(LIB): $(OBJS) | ../build
	$(AR) $(ARFLAGS) $@ $^

../build/vt100.o: vt100.c vt100.h | ../build
	$(CC) $(CFLAGS) -c vt100.c -o $@

clean:
	rm -f ../build/vt100.o ../build/libvt100.a

.PHONY: all clean
