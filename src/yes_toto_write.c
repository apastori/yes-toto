/*
 * Chain-of-thought (Step 1 — file scope):
 *
 * 1. Single responsibility: buffered prefill and write loop to stdout until
 *    EPIPE (exit 0) or hard write error (exit 1); partial writes handled.
 * 2. write: EINTR or stop flag → _exit(130); EPIPE → fflush+exit(0); other
 *    errors return -1. TTY stdout uses one line per outer loop to avoid
 *    flooding the terminal; piped stdout keeps the 64 KiB prefill slab.
 * 3. No heap in the hot loop.
 * 4. Throughput: 64 KiB slab amortises syscalls; fallback pathwhen line exceeds
 *    slab does one line per outer iteration.
 * 5. C11 + POSIX.1-2008 (write, unistd).
 */

/*
 * Performance reasoning (Step 4):
 *
 * 1. Buffer strategy: we pre-fill YES_TOTO_BUF_SIZE bytes with full copies of
 *    the logical line so each write() pushes a 64 KiB chunk. This balances L2
 *    cache residency with typical pipe buffers and cuts syscalls versus
 *    per-line writes.
 *
 * 2. Why not printf/puts in the loop: stdio would parse format strings or
 *    maintain per-stream locks and extra copying for buffering policy —
 *    repeated locks and layer indirection dominate versus raw write() of a
 *    known payload. The spec forbids printf in the hot loop.
 *
 * 3. Partial writes: write() may return less than requested; ignoring the count
 *    drops bytes on the floor and corrupts the byte stream. We loop until the
 *    full prefill is consumed or an error occurs.
 *
 * 4. Fallback: if line_len > YES_TOTO_BUF_SIZE we cannot fit even one line in
 *    the slab; we write the line (still handling partial writes) once per
 *    outer iteration — not the common path.
 */

/*
 * Signal handling (Step 5 — write path):
 *
 * SIGPIPE default terminates with non-zero status and may print "Broken pipe"
 * from the shell; for yes, consumer close is success. Ignoring SIGPIPE is the
 * POSIX idiom so write() fails with errno == EPIPE instead; we then exit 0.
 */

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <io.h>
#endif

#include "yes_toto.h"

#include "yes_toto_cli.h"
#include "yes_toto_emit.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#if defined(_WIN32)
/*
 * maybe_normalize_broken_pipe_errno — map MinGW EINVAL to EPIPE on broken pipe.
 *
 * Preconditions: fd is a valid CRT file descriptor; buf non-NULL when called;
 *                errno reflects the failed write().
 * Postconditions: on pipe fds, errno may become EPIPE; otherwise unchanged.
 */
static void maybe_normalize_broken_pipe_errno(int fd, const void *buf)
{
    if (errno != EINVAL || buf == NULL) {
        return;
    }
    HANDLE h = (HANDLE)_get_osfhandle(fd);
    if (h != INVALID_HANDLE_VALUE && GetFileType(h) == FILE_TYPE_PIPE) {
        errno = EPIPE;
    }
}
#else
static void maybe_normalize_broken_pipe_errno(int fd, const void *buf)
{
    (void)fd;
    (void)buf;
}
#endif

/*
 * try_write_all — drain buf[0..count) to STDOUT_FILENO.
 *
 * Preconditions: buf valid for count bytes if count > 0.
 * Returns: 1 on full success, -1 on error (errno set; never EPIPE — EPIPE
 *          flushes stdout and exits with status 0 inside this helper).
 */
static int try_write_all(const char *buf, size_t count)
{
    size_t sent = 0;

    while (sent < count) {
        yes_toto_exit_if_stop_requested();
        ssize_t n = write(STDOUT_FILENO, buf + sent, count - sent);
        if (n < 0) {
            if (errno == EINTR) {
                _exit(YES_TOTO_EXIT_SIGINT);
            }
            maybe_normalize_broken_pipe_errno(STDOUT_FILENO, buf + sent);
            if (errno == EPIPE) {
                (void)fflush(stdout);
                exit(YES_TOTO_EXIT_OK_PIPE);
            }
            return -1;
        }
        sent += (size_t)n;
    }
    return 1;
}

/*
 * yes_toto_run — see `include/yes_toto.h` for the full contract.
 *
 * Preconditions: line != NULL, line_len > 0.
 * Postconditions: does not return; exits via try_write_all on EPIPE/write err.
 */
void yes_toto_run(const char *line, size_t line_len)
{
    /*
     * Interactive terminal: one line per write so Ctrl+C is picked up between
     * small writes and the emulator is not flooded with 64 KiB bursts.
     * Pipe/redirection: prefill slab for throughput (e.g. yes-toto | head).
     */
    const int stdout_is_tty = isatty(STDOUT_FILENO);

    if (line_len > YES_TOTO_BUF_SIZE || stdout_is_tty) {
        for (;;) {
            yes_toto_exit_if_stop_requested();
            if (try_write_all(line, line_len) < 0) {
                yes_toto_emit_error("write");
                exit(YES_TOTO_EXIT_WRITE_ERR);
            }
        }
    }

    char buf[YES_TOTO_BUF_SIZE];
    const size_t filled = fill_buffer(buf, YES_TOTO_BUF_SIZE, line, line_len);

    for (;;) {
        yes_toto_exit_if_stop_requested();
        if (try_write_all(buf, filled) < 0) {
            yes_toto_emit_error("write");
            exit(YES_TOTO_EXIT_WRITE_ERR);
        }
    }
}
