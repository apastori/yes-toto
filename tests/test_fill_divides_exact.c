/*
 * Chain-of-thought (Step 1 — file scope):
 *
 * 1. Single responsibility: assert even packing when line_len divides buf_size.
 * 2. No syscalls.
 * 3. No heap.
 * 4. Not throughput-sensitive.
 * 5. C11 / assert().
 */

#include "test_fill_divides_exact.h"

#include "yes_toto.h"

#include <assert.h>
#include <stdio.h>

/*
 * Test 2 reasoning: when line_len divides buf_size, the last copy ends exactly
 * at buf_size with no slack — proves we do not stop early when the buffer is
 * evenly packed.
 */
void test_line_divides_buffer_exactly(void)
{
    char buf[12];
    const char line[] = "abc";
    const size_t line_len = sizeof(line) - 1u;
    const size_t n = fill_buffer(buf, sizeof(buf), line, line_len);

    assert(n == sizeof(buf));
    assert(n % line_len == 0);
    printf("PASS: line divides buffer exactly — no partial copy\n");
}
