/*
 * Chain-of-thought (Step 1 — before code):
 *
 * 1. Single responsibility: declare the public contract for yes-toto — buffer
 *    size, version string, the core fill_buffer() algorithm used to amortise
 *    writes, and the entry symbol for the infinite output loop used by main.
 *
 * 2. Syscalls: this header defines no syscalls. fill_buffer() only touches
 *    user-provided memory; failure modes are caller bugs (NULL, bad lengths)
 *    and are covered by preconditions, not runtime checks here.
 *
 * 3. Heap in hot loop: none. fill_buffer() is allocation-free.
 *
 * 4. Throughput: the bottleneck for yes-like tools is syscall rate and pipe
 *    back-pressure once the buffer is tuned. fill_buffer() packs many logical
 *    lines into one 64 KiB chunk so each write(2) moves a large payload; the
 *    implementation in main.c repeats write until the chunk is fully drained.
 *
 * 5. Standard: C11 — the project builds with -std=c11; this header uses
 *    size_t from stddef.h and static inline from C99/C11.
 */

#ifndef YES_TOTO_H
#define YES_TOTO_H

#include <stddef.h>
#include <string.h>

/*
 * YES_TOTO_BUF_SIZE — 64 KiB write/prefill slab.
 *
 * Rationale: fits comfortably in L1/L2 on typical CPUs while matching common
 * pipe buffer behaviour (often 64 KiB on Linux) so each write() often fills
 * the kernel pipe buffer once, amortising syscall cost. Smaller buffers spend
 * more time in the kernel boundary; much larger buffers see diminishing
 * returns and worse cache locality for this serial producer.
 */
#define YES_TOTO_BUF_SIZE 65536u

/* Semantic version printed by `yes-toto --version`. */
#define YES_TOTO_VERSION_STRING "1.0.0"

enum {
    YES_TOTO_EXIT_OK_PIPE = 0,
    YES_TOTO_EXIT_WRITE_ERR = 1,
    /* Conventional shell status when stopped by Ctrl+C / SIGINT (128 + 2). */
    YES_TOTO_EXIT_SIGINT = 130
};

/*
 * yes_toto_run — write `line` (length `line_len`, includes trailing '\n')
 * to stdout forever until EPIPE (exit 0) or a non-EPIPE write error (exit 1).
 *
 * Preconditions:
 *   - line != NULL
 *   - line_len > 0
 * Postconditions: does not return on success path.
 * Error behaviour: may write to stderr and call exit(); see main.c.
 */
void yes_toto_run(const char *line, size_t line_len);

/*
 * fill_buffer — populate `buf` with as many complete copies of `line`
 * (length `line_len`) as fit, without writing partial copies.
 *
 * Returns the number of bytes written into `buf`.
 *
 * Precondition : line_len > 0 && line_len <= buf_size
 * Postcondition: return value is a multiple of line_len
 *                return value <= buf_size
 *
 * Algorithm (pseudocode):
 *   pos := 0
 *   while pos + line_len <= buf_size:
 *       copy line[0..line_len) to buf[pos..pos+line_len)
 *       pos += line_len
 *   return pos
 *
 * Termination: pos increases by at least 1 each iteration (line_len >= 1) and
 * is bounded by buf_size, so the while condition eventually fails.
 *
 * No partial copy: memcpy runs only when a full line_len bytes fit in the
 * remaining buf_size - pos space.
 */
static inline size_t fill_buffer(char *buf, size_t buf_size,
                                const char *line, size_t line_len)
{
    size_t pos = 0;
    while (pos + line_len <= buf_size) {
        memcpy(buf + pos, line, line_len);
        pos += line_len;
    }
    return pos;
}

#endif /* YES_TOTO_H */
