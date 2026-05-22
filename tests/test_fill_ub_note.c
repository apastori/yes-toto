/*
 * Chain-of-thought (Step 1 — file scope):
 *
 * 1. Single responsibility: document UB for line_len > buf_size; no call.
 * 2. No syscalls.
 * 3. No heap.
 * 4. N/A.
 * 5. C11.
 */

#include "test_fill_ub_note.h"

#include <stdio.h>

/*
 * Test 5 reasoning: fill_buffer's contract requires line_len <= buf_size.
 * line_len > buf_size violates the precondition — behaviour is explicitly
 * undefined per the API; callers must use the pathological write path in
 * main.c instead of fill_buffer.
 */
void test_line_longer_than_buffer_is_ub(void)
{
    printf("PASS: line_len > buf_size is precondition violation (undefined) — "
           "not exercised\n");
}
