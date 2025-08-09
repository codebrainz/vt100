# Root Makefile for vt100 project

.PHONY: all clean test docs

all:
	$(MAKE) -C src
	$(MAKE) -C tests


clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
	rm -rf docs/reference/*

# Format all .c and .h files in src and tests
format:
	find src tests -name '*.c' -o -name '*.h' | xargs clang-format -i

# Run unit tests
test: all
	$(MAKE) -C tests
	cd tests && ./vt100_test

# Documentation (Doxygen)
docs:
	doxygen docs/Doxyfile
