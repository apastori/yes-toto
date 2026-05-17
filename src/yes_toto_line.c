/*
 * Chain-of-thought (Step 1 — file scope):
 *
 * 1. Single responsibility: join argv[1..] with spaces, append newline, using
 *    stack storage up to YES_TOTO_JOIN_STACK_CAP or malloc when larger.
 * 2. malloc: failure returns -1; errno from allocator.
 * 3. No allocation in any loop that could be considered the output hot path;
 *    this runs once before yes_toto_run.
 * 4. Not throughput-critical.
 * 5. C11 (+ POSIX exec limits in practice cap argv mass).
 */

#include "yes_toto_line.h"

#include <stdlib.h>
#include <string.h>

/* Stack cap for joined line; larger lines use one heap allocation (pre-loop). */
#define YES_TOTO_JOIN_STACK_CAP 65536u

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
int build_output_line(int argc, char **argv, const char **out_line,
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
