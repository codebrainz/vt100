# Makefile for vt100 tests (deduplicated and modern)

CC = gcc
BUILD_DIR = ../build
LIBS = -L../build -lvt100
TEST = ../build/vt100_test
OBJS = ../build/vt100_test.o

# Allow SANITIZE=address,undefined, etc.
ifeq ($(SANITIZE),)
  CFLAGS = -Wall -Wextra -std=c99 -O2 -I../src
else
  CFLAGS = -Wall -Wextra -std=c99 -g -fsanitize=$(SANITIZE) -I../src
endif

all: $(TEST)

../build:
	mkdir -p ../build

$(TEST): $(OBJS) ../build/libvt100.a | ../build
	$(MAKE) -C ../src
	$(CC) $(CFLAGS) -o $@ $^ -L../build -lvt100

../build/vt100_test.o: vt100_test.c | ../build
	$(CC) $(CFLAGS) -c vt100_test.c -o $@

test: $(TEST)
	$<

clean:
	rm -f ../build/vt100_test.o ../build/vt100_test

.PHONY: all clean test
