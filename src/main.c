/*
 * Chain-of-thought (Step 1 — before code):
 *
 * 1. Single responsibility: argv handling (including --help / --version
 *    discovery across all arguments), build the output line (with '\n'), configure
 *    SIGPIPE handling, and run the high-throughput write loop using
 *    fill_buffer() plus robust write() retry/partial-write handling,
 *    with a fallback path for lines longer than YES_TOTO_BUF_SIZE.
 *
 * 2. Syscall failure modes:
 *    - write(STDOUT_FILENO, ...): returns -1 with errno EINTR → retry;
 *      EPIPE with SIGPIPE ignored → treat as normal consumer closure, exit 0;
 *      other errno → perror-style message, exit 1. Partial success (0<n<count)
 *      → advance pointer and continue (not an error).
 *    - sigaction(SIGPIPE, SIG_IGN): on failure sets errno; we check and abort
 *      with a clear error to stderr, exit 1 (pathological on POSIX).
 *    - fflush(stdout) before exit(0) on EPIPE: may fail with errno; we
 *      deliberately ignore the return value because the pipe is already broken
 *      and we still want exit 0 per spec.
 *    - malloc (build_output_line only): failure returns NULL; errno set by
 *      the allocator (typically ENOMEM).
 *
 * 3. Heap in hot loop: none. yes_toto_run() uses only stack buffers and
 *    fill_buffer(); the join buffer may use heap only before the
 *    loop if the constructed line exceeds the stack cap (see build_output_line).
 *
 * 4. Throughput bottleneck: syscall overhead dominates once the line fits in
 *    the 64 KiB slab; pre-filling amortises write() count. Pipe buffer
 *    back-pressure still limits the producer — large writes align with common
 *    pipe capacities. For pathological line_len > slab, fallback does one
 *    line per outer iteration (unavoidably more syscalls).
 *
 * 5. C standard: C11 (-std=c11) for memcpy bounds idioms, static inline in the
 *    header, stdalign if ever needed later, and consistent POSIX feature macros
 *    with _POSIX_C_SOURCE=200809L for POSIX.1-2008 APIs used here.
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
 * Signal handling (Step 5):
 *
 * SIGPIPE default terminates with non-zero status and may print "Broken pipe"
 * from the shell; for yes, consumer close is success. Ignoring SIGPIPE is the
 * POSIX idiom so write() fails with errno == EPIPE instead; we then exit 0.
 */

#include "yes_toto.h"

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/* Stack cap for joined line; larger lines use one heap allocation (pre-loop). */
#define YES_TOTO_JOIN_STACK_CAP 65536u

#define YES_TOTO_ARG_HELP    "--help"
#define YES_TOTO_ARG_VERSION "--version"

enum { YES_TOTO_EXIT_OK_PIPE = 0, YES_TOTO_EXIT_WRITE_ERR = 1 };

/*
 * yes_toto_emit_error — print `yes-toto: context: strerror(errno)` to stderr.
 *
 * Preconditions: context != NULL; errno reflects the failed syscall/setup.
 * Postconditions: exactly one line on stderr.
 */
static void yes_toto_emit_error(const char *context)
{
    fprintf(stderr, "yes-toto: %s: %s\n", context, strerror(errno));
}

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
        ssize_t n = write(STDOUT_FILENO, buf + sent, count - sent);
        if (n < 0) {
            if (errno == EINTR) {
                continue;
            }
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
    if (line_len > YES_TOTO_BUF_SIZE) {
        for (;;) {
            if (try_write_all(line, line_len) < 0) {
                yes_toto_emit_error("write");
                exit(YES_TOTO_EXIT_WRITE_ERR);
            }
        }
    }

    char buf[YES_TOTO_BUF_SIZE];
    const size_t filled = fill_buffer(buf, YES_TOTO_BUF_SIZE, line, line_len);

    for (;;) {
        if (try_write_all(buf, filled) < 0) {
            yes_toto_emit_error("write");
            exit(YES_TOTO_EXIT_WRITE_ERR);
        }
    }
}

/*
 * build_output_line — construct "joined\\n" from argv[1..argc-1], default "y".
 *
 * Preconditions: argc >= 1, argv non-NULL.
 * Postconditions: *out_line points at storage holding the line; *out_len is
 *                 length including '\\n'. *heap_block is set if malloc was used
 *                 (caller may free before exit; we don't for infinite loop —
 *                 process lifetime only).
 * Returns: 0 on success, -1 on allocation failure.
 *
 * Maximum length: POSIX places no small cap on argv strings collectively
 * (ARG_MAX constrains execve); we support arbitrary length via heap when the
 * joined line exceeds YES_TOTO_JOIN_STACK_CAP.
 */
static int build_output_line(int argc, char **argv, const char **out_line,
                            size_t *out_len, void **heap_block)
{
    *heap_block = NULL;

    if (argc <= 1) {
        static const char k_default[] = "y\n";
        *out_line = k_default;
        *out_len = sizeof(k_default) - 1u;
        return 0;
    }

    size_t line_buffer_size = 0;
    for (int i = 1; i < argc; i++) {
        line_buffer_size += strlen(argv[i]);
        if (i < argc - 1) {
            line_buffer_size += 1u;
        }
    }
    line_buffer_size += 1u;

    char stack_line[YES_TOTO_JOIN_STACK_CAP];
    char *storage;
    if (line_buffer_size <= YES_TOTO_JOIN_STACK_CAP) {
        storage = stack_line;
    } else {
        storage = (char *)malloc(line_buffer_size);
        if (storage == NULL) {
            return -1;
        }
        *heap_block = storage;
    }

    size_t pos = 0;
    for (int i = 1; i < argc; i++) {
        const char *s = argv[i];
        const size_t slen = strlen(s);
        memcpy(storage + pos, s, slen);
        pos += slen;
        if (i < argc - 1) {
            storage[pos++] = ' ';
        }
    }
    storage[pos++] = '\n';
    *out_line = storage;
    *out_len = pos;
    return 0;
}

/*
 * argv_has_exact — return whether any argv[1..argc-1] equals `needle`.
 *
 * Preconditions: argc >= 1; argv points to argc C strings.
 * Postconditions: returns 1 if found, 0 otherwise.
 */
static int argv_has_exact(int argc, char **argv, const char *needle)
{
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], needle) == 0) {
            return 1;
        }
    }
    return 0;
}

/*
 * print_help — write usage text to stdout.
 *
 * Preconditions: none.
 * Postconditions: formatted help emitted; returns void.
 */
static void print_help(void)
{
    printf(
        "Usage: yes-toto [STRING]...\n"
        "       yes-toto OPTION\n"
        "\n"
        "Repeatedly print a line to standard output until stopped.\n"
        "\n"
        "With no STRING, print 'y'.\n"
        "\n"
        "Options:\n"
        "  --help     display this help and exit\n"
        "  --version  display version and exit\n"
    );
}

/*
 * print_version — write `yes-toto VERSION` line to stdout.
 *
 * Preconditions: none.
 * Postconditions: one line on stdout; returns void.
 */
static void print_version(void)
{
    printf("yes-toto %s\n", YES_TOTO_VERSION_STRING);
}

/*
 * install_sigpipe_ignore — configure SIGPIPE as ignored for EPIPE-on-write.
 *
 * Preconditions: none.
 * Postconditions: returns 0 on success, -1 on failure (errno set).
 */
static int install_sigpipe_ignore(void)
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) != 0) {
        yes_toto_emit_error("sigaction(SIGPIPE)");
        return -1;
    }
    return 0;
}

/*
 * scan_meta_flags — Step 7: scan all argv[1..] before interpreting strings.
 * --help wins over --version when both appear.
 *
 * Preconditions: argc >= 1.
 * Postconditions: sets *help or *version when those tokens appear anywhere.
 */
static void scan_meta_flags(int argc, char **argv, int *help, int *version)
{
    *help = argv_has_exact(argc, argv, YES_TOTO_ARG_HELP);
    *version = 0;
    if (*help) {
        return;
    }
    *version = argv_has_exact(argc, argv, YES_TOTO_ARG_VERSION);
}

/*
 * main — argv dispatcher for help/version or infinite output.
 *
 * Preconditions: argc >= 1, argv[0..argc-1] valid C strings.
 * Postconditions: returns 0/1 for meta paths or setup errors; otherwise does
 *                 not return (`yes_toto_run`).
 */
int main(int argc, char **argv)
{
    int want_help = 0;
    int want_version = 0;

    scan_meta_flags(argc, argv, &want_help, &want_version);

    if (want_help) {
        print_help();
        return YES_TOTO_EXIT_OK_PIPE;
    }
    
    if (want_version) {
        print_version();
        return YES_TOTO_EXIT_OK_PIPE;
    }

    if (install_sigpipe_ignore() != 0) {
        return YES_TOTO_EXIT_WRITE_ERR;
    }

    const char *line;
    size_t line_len;
    void *heap_line = NULL;

    if (build_output_line(argc, argv, &line, &line_len, &heap_line) != 0) {
        yes_toto_emit_error("build_output_line");
        return YES_TOTO_EXIT_WRITE_ERR;
    }

    (void)heap_line;

    yes_toto_run(line, line_len);
    return YES_TOTO_EXIT_OK_PIPE;
}
