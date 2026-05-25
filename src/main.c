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
 *    - write(STDOUT_FILENO, ...): returns -1 with errno EINTR → exit 130
 *      (SIGINT / Ctrl+C); EPIPE with SIGPIPE ignored → treat as normal
 *      consumer closure, exit 0; other errno → perror-style message, exit 1.
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

#include "yes_toto.h"

#include "yes_toto_cli.h"
#include "yes_toto_emit.h"
#include "yes_toto_line.h"

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

    if (install_sigint_handler() != 0) {
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
