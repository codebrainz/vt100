# Root Makefile for vt100 project (clean, deduplicated, and modern)

.PHONY: all clean test docs dist valgrind-test install uninstall format

BUILD_DIR := build

all: $(BUILD_DIR)
	$(MAKE) -C src
	$(MAKE) -C tests

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tests clean
	rm -rf $(BUILD_DIR)/*
	rm -rf docs/reference/*

format:
	find src tests -name '*.c' -o -name '*.h' | xargs clang-format -i

test: all
	$(MAKE) -C tests test

docs:
	doxygen docs/Doxyfile

dist:
	@name=vt100-$(shell date +%Y%m%d); \
	tar --exclude-vcs --exclude='*.o' --exclude='*.a' --exclude='*.swp' --exclude='*.pyc' --exclude='*.ipynb' -cJf $$name.tar.xz \
		README.md LICENSE.md Makefile src/ tests/ docs/ PLAN.md; \
	echo "Created: $$name.tar.xz"

valgrind-test: all
	cd $(BUILD_DIR) && valgrind ./vt100_test

install:
	echo "Install not yet implemented."

uninstall:
	echo "Uninstall not yet implemented."
