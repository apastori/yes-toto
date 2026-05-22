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

MAIN_SRCS := src/main.c \
	src/yes_toto_emit.c \
	src/yes_toto_write.c \
	src/yes_toto_line.c \
	src/yes_toto_cli.c

HDRS := $(wildcard include/*.h)

TEST_SRCS := tests/test_runner.c \
	tests/test_fill_one_byte.c \
	tests/test_fill_divides_exact.c \
	tests/test_fill_indivisible.c \
	tests/test_fill_line_eq_buf.c \
	tests/test_fill_ub_note.c

.PHONY: all debug test clean install

all: yes-toto

yes-toto: $(MAIN_SRCS) $(HDRS)
	$(CC) $(CFLAGS) -o $@ $(MAIN_SRCS)

debug: yes-toto-debug

yes-toto-debug: $(MAIN_SRCS) $(HDRS)
	$(CC) $(CFLAGS_DEBUG) -o $@ $(MAIN_SRCS)

tests/test_core: $(TEST_SRCS) $(HDRS) $(wildcard tests/*.h)
	$(CC) $(CFLAGS) -o $@ $(TEST_SRCS)

test: tests/test_core
	./tests/test_core

clean:
	rm -f yes-toto yes-toto-debug tests/test_core

install: yes-toto
	install -m 755 yes-toto /usr/local/bin
