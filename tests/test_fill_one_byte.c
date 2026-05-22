/*
 * Chain-of-thought (Step 1 — file scope):
 *
 * 1. Single responsibility: assert fill_buffer packs 1-byte lines to full buf.
 * 2. No syscalls.
 * 3. No heap.
 * 4. Not throughput-sensitive.
 * 5. C11 / assert().
 */

#include "test_fill_one_byte.h"

#include "yes_toto.h"

#include <assert.h>
#include <stdio.h>

/*
 * Test 1 reasoning: a 1-byte line divides any buffer size, so fill_buffer
 * must copy byte-for-byte until the buffer is exactly full — returned length
 * equals buf_size.
 */
void test_one_byte_line_fills_completely(void)
{
    char buf[32];
    const char line[] = "Z";
    const size_t n = fill_buffer(buf, sizeof(buf), line, sizeof(line) - 1u);

    assert(n == sizeof(buf));
    for (size_t i = 0; i < sizeof(buf); i++) {
        assert(buf[i] == 'Z');
    }
    printf("PASS: one-byte line fills buffer completely\n");
}
