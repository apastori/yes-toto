/*
 * Chain-of-thought (Step 1 — file scope):
 *
 * 1. Single responsibility: format error lines on stderr for syscall/setup
 *    failures.
 * 2. No syscalls here; strerror reads errno set by the caller.
 * 3. No heap in this path beyond fprintf’s stdio internals.
 * 4. Not throughput-sensitive.
 * 5. C11 via the project Makefile (-std=c11).
 */

#include "yes_toto_emit.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

/*
 * yes_toto_emit_error — print `yes-toto: context: strerror(errno)` to stderr.
 *
 * Preconditions: context != NULL; errno reflects the failed syscall/setup.
 * Postconditions: exactly one line on stderr.
 */
void yes_toto_emit_error(const char *context)
{
    fprintf(stderr, "yes-toto: %s: %s\n", context, strerror(errno));
}
