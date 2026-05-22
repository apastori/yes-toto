/*
 * Chain-of-thought (Step 1 — file scope):
 *
 * 1. Single responsibility: assert one full line when line_len == buf_size.
 * 2. No syscalls.
 * 3. No heap.
 * 4. Not throughput-sensitive.
 * 5. C11 / assert().
 */

#include "test_fill_line_eq_buf.h"

#include "yes_toto.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

/*
 * Test 4 reasoning: line_len == buf_size allows exactly one memcpy of the full
 * logical line into the slab.
 */
void test_line_equals_buffer_size(void)
{
    char buf[4];
    const char line[] = "wxyz";
    const size_t line_len = sizeof(line) - 1u;

    assert(line_len == sizeof(buf));
    const size_t n = fill_buffer(buf, sizeof(buf), line, line_len);

    assert(n == sizeof(buf));
    assert(memcmp(buf, line, line_len) == 0);
    printf("PASS: line length equals buffer size — single full copy\n");
}
