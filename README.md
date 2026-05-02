# yes-toto

A small C11 utility that behaves like GNU `yes`: it repeatedly prints a line
until stdout closes (for example at the end of a pipe). It uses a 64 KiB
prefetch buffer and `write(2)` so the hot path avoids stdio formatting and
minimises syscalls.

## Build

```sh
make            # release binary: ./yes-toto
make debug      # sanitizers + debug symbols
make test       # unit tests for fill_buffer
make clean
sudo make install   # installs to /usr/local/bin (optional)
```

Requires a POSIX-like environment with `gcc` or `clang`, `sigaction`, and `write`.

## Usage

- `yes-toto` — prints `y` plus newline forever.
- `yes-toto STRING ...` — prints the arguments joined by single spaces, plus newline, forever.
- `yes-toto --help` / `yes-toto --version` — print help or version and exit.

`--help` is detected anywhere in the argument list and takes precedence over `--version`.

## Exit codes

- `0` — normal close (including broken pipe / `EPIPE` after ignoring `SIGPIPE`).
- `1` — write or setup error (messages on stderr: `yes-toto: <context>: <reason>`).

## Layout

- `include/yes_toto.h` — constants, `fill_buffer()`, `yes_toto_run()`.
- `src/main.c` — argv handling, `SIGPIPE` setup, buffered write loop.
- `tests/test_core.c` — `assert()`-based tests.
