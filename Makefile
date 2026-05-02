# yes-toto — POSIX-oriented Makefile (gcc or clang on Unix, MSYS2, etc.)
#
# Compiler flag rationale (release CFLAGS):
#   -std=c11          ISO C11 baseline requested by the project spec.
#   -O2               Strong optimisation without -O3’s aggressive trade-offs.
#   -Wall -Wextra     Enable most warnings.
#   -Wpedantic        Reject common extensions and dubious constructs.
#   -Werror           Treat warnings as build-breaking (zero-warning policy).
#   -D_POSIX_C_SOURCE=200809L  Expose POSIX.1-2008 (sigaction, write, etc.).

CC := $(shell command -v gcc >/dev/null 2>&1 && echo gcc || echo clang)

CFLAGS_COMMON := -std=c11 -Wall -Wextra -Wpedantic -Werror \
	-D_POSIX_C_SOURCE=200809L -Iinclude

CFLAGS := $(CFLAGS_COMMON) -O2

# Sanitizers need matching compile+link flags.
CFLAGS_DEBUG := $(CFLAGS_COMMON) -g -O1 -fsanitize=address,undefined \
	-fno-omit-frame-pointer

.PHONY: all debug test clean install

all: yes-toto

yes-toto: src/main.c include/yes_toto.h
	$(CC) $(CFLAGS) -o $@ src/main.c

debug: yes-toto-debug

yes-toto-debug: src/main.c include/yes_toto.h
	$(CC) $(CFLAGS_DEBUG) -o $@ src/main.c

tests/test_core: tests/test_core.c include/yes_toto.h
	$(CC) $(CFLAGS) -o $@ tests/test_core.c

test: tests/test_core
	./tests/test_core

clean:
	rm -f yes-toto yes-toto-debug tests/test_core

install: yes-toto
	install -m 755 yes-toto /usr/local/bin
