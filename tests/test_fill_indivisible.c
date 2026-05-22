/*
 * Chain-of-thought (Step 1 — file scope):
 *
 * 1. Single responsibility: assert slack is skipped when sizes are coprime-ish.
 * 2. No syscalls.
 * 3. No heap.
 * 4. Not throughput-sensitive.
 * 5. C11 / assert().
 */

#include "test_fill_indivisible.h"

#include "yes_toto.h"

#include <assert.h>
#include <stdio.h>

/*
 * Test 3 reasoning: when buf_size % line_len != 0, the remainder slack must
 * remain untouched — we only write floor(buf_size / line_len) full lines.
 */
void test_line_does_not_divide_buffer(void)
{
    char buf[10];
    const char line[] = "abc";
    const size_t line_len = sizeof(line) - 1u;
    const size_t n = fill_buffer(buf, sizeof(buf), line, line_len);

    assert(n == 9);
    assert(n % line_len == 0);
    printf("PASS: indivisible buffer — trailing slack not written\n");
}
