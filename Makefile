
# === Variables ===
SRC_DIR      := .
TESTS_DIR    := .
BUILD_DIR    := build
LIB          := $(BUILD_DIR)/libvt100.a
LIB_OBJS     := $(BUILD_DIR)/vt100.o
TEST         := $(BUILD_DIR)/vt100_test
TEST_OBJS    := $(BUILD_DIR)/vt100_test.o
PREFIX       ?= /usr/local
INCLUDEDIR   := $(PREFIX)/include
LIBDIR       := $(PREFIX)/lib

# Compiler flags (optionally enable sanitizers)
ifeq ($(SANITIZE),)
  CFLAGS      := -Wall -Wextra -std=c99 -O2 -fPIC
  TEST_CFLAGS := -Wall -Wextra -std=c99 -O2 -I$(SRC_DIR)
else
  CFLAGS      := -Wall -Wextra -std=c99 -g -fPIC -fsanitize=$(SANITIZE)
  TEST_CFLAGS := -Wall -Wextra -std=c99 -g -fsanitize=$(SANITIZE) -I$(SRC_DIR)
endif

# === Build Targets ===
all: $(LIB) $(TEST)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(LIB): $(LIB_OBJS) | $(BUILD_DIR)
	ar rcs $@ $^

$(BUILD_DIR)/vt100.o: vt100.c vt100.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c vt100.c -o $@

$(TEST): $(TEST_OBJS) $(LIB) | $(BUILD_DIR)
	$(CC) $(TEST_CFLAGS) -o $@ $^ -L$(BUILD_DIR) -lvt100

$(BUILD_DIR)/vt100_test.o: vt100_test.c | $(BUILD_DIR)
	$(CC) $(TEST_CFLAGS) -c vt100_test.c -o $@

# === Utility Targets ===

# Show usage information
help:
	@echo "Usage: make [target]"
	@echo "Targets:"
	@echo "  all            Build library and test binary (default)"
	@echo "  test           Build and run the test suite"
	@echo "  valgrind-test  Run tests under Valgrind (do not use with SANITIZE)"
	@echo "  docs           Build Doxygen HTML documentation in build/docs/reference/"
	@echo "  dist           Create a source tarball in build/"
	@echo "  format         Run clang-format on all source and header files"
	@echo "  clean          Remove build artifacts and generated docs"
	@echo "  install        Install library to $(PREFIX) (override with PREFIX=/path)"
	@echo "  uninstall      Remove installed library and header from $(PREFIX)"
	@echo "  help           Show this help message"

clean:
	rm -f $(BUILD_DIR)/*.o $(BUILD_DIR)/*.a $(BUILD_DIR)/vt100_test
	rm -rf $(BUILD_DIR)/docs/reference/*

format:
	find . -maxdepth 1 -name '*.c' -o -name '*.h' | xargs clang-format -i

# === Test & Analysis ===
test: $(TEST)
	$<

valgrind-test: $(TEST)
	cd $(BUILD_DIR) && valgrind ./vt100_test

# === Documentation ===
docs: | $(BUILD_DIR)
	mkdir -p $(BUILD_DIR)/docs/reference
	doxygen Doxyfile

# === Distribution ===

dist: | $(BUILD_DIR)
       @name=vt100-$(shell date +%Y%m%d); \
       tar --exclude-vcs --exclude='*.o' --exclude='*.a' --exclude='*.swp' --exclude='*.pyc' --exclude='*.ipynb' -cJf $(BUILD_DIR)/$$name.tar.xz \
	       README.md LICENSE.md Makefile vt100.c vt100.h vt100_test.c Doxyfile docs/ PLAN.md; \
       echo "Created: $(BUILD_DIR)/$$name.tar.xz"


# === Install/Uninstall ===
install: all
	mkdir -p $(INCLUDEDIR)
	mkdir -p $(LIBDIR)
	cp vt100.h $(INCLUDEDIR)/vt100.h
	cp $(BUILD_DIR)/libvt100.a $(LIBDIR)/libvt100.a
	echo "Installed libvt100.a to $(LIBDIR) and vt100.h to $(INCLUDEDIR)"

uninstall:
	rm -f $(LIBDIR)/libvt100.a
	rm -f $(INCLUDEDIR)/vt100.h
	echo "Uninstalled libvt100.a from $(LIBDIR) and vt100.h from $(INCLUDEDIR)"

.PHONY: all clean test docs dist valgrind-test install uninstall format help
