# yes-toto

A small C11 utility that behaves like GNU `yes`: it repeatedly prints a line
until stdout closes (for example at the end of a pipe) or you stop it with
Ctrl+C. It uses raw `write(2)` in the hot path (no stdio formatting in the
loop).

**Output strategy:** when stdout is a **terminal**, the program writes **one
line per loop** (like GNU `yes`) so the emulator stays responsive and Ctrl+C
works reliably. When stdout is a **pipe** or file, it prefills a **64 KiB**
slab via `fill_buffer()` to amortise syscalls (`yes-toto | head`, etc.).

## Build

```sh
make            # release binary: ./yes-toto
make debug      # sanitizers + debug symbols
make test       # unit tests for fill_buffer
make clean
sudo make install   # installs to /usr/local/bin (optional)
```

Requires `gcc` or `clang` and `write(2)` (POSIX-like systems). On Linux, `sigaction` is used for `SIGPIPE`; on Windows (MSYS2 UCRT64 MinGW) build natively — see below.

### Windows (MSYS2 UCRT64)

Build from the **MSYS2 UCRT64** terminal (recommended):

```sh
pacman -S make mingw-w64-ucrt-x86_64-gcc   # if not already installed
cd /c/Users/alfon/Desktop/code/yes-toto
make clean && make
```

The resulting `yes-toto` is a native Windows executable and runs in UCRT64, Git Bash, cmd, and PowerShell.

### Linux

```sh
make clean && make
```

## Usage

- `yes-toto` — prints `y` plus newline forever.
- `yes-toto STRING ...` — prints the arguments joined by single spaces, plus newline, forever.
- `yes-toto --help` / `yes-toto --version` — print help or version and exit.

`--help` is detected anywhere in the argument list and takes precedence over `--version`.

### Pipes and terminals

| stdout destination | Write pattern | Example |
|--------------------|---------------|---------|
| Terminal (`isatty`) | one line per loop | `./yes-toto` — Ctrl+C exits **130** |
| Pipe or redirect | 64 KiB prefill slab | `./yes-toto \| head -n 5` — exits **0** when `head` closes the pipe |

On Windows (MinGW), a broken pipe may surface as `EINVAL` instead of `EPIPE`; the write path normalises that to `EPIPE` so piped use stays quiet (no stderr error).

## Exit codes

- `0` — normal close: `--help`, `--version`, or broken pipe (`EPIPE`; on Linux after ignoring `SIGPIPE`).
- `1` — write or setup error (messages on stderr: `yes-toto: <context>: <reason>`).
- `130` — stopped by Ctrl+C / `SIGINT` (`128 + 2`, conventional shell status).

## Layout

- `LICENSE.txt` — GNU General Public License, version 2.
- `c_version.txt` — C11 standard, compiler flags, and toolchain notes.
- `include/yes_toto.h` — constants, exit enum, `fill_buffer()`, `yes_toto_run()`.
- `include/yes_toto_*.h` — declarations for emit, line build, CLI helpers.
- `src/main.c` — entry point only.
- `src/yes_toto_emit.c`, `yes_toto_write.c`, `yes_toto_line.c`, `yes_toto_cli.c` — implementation units.
- `tests/test_runner.c` plus `tests/test_fill_*.c` / `.h` — `assert()`-based `fill_buffer` tests; built binary `tests/test_core`.
