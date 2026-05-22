/*
 * Chain-of-thought (Step 1 — before code):
 *
 * 1. Single responsibility: invoke each fill_buffer unit test in order.
 * 2. Syscalls: none.
 * 3. Heap: none in runner.
 * 4. N/A.
 * 5. C11 — matches the main program; assert() lives in individual tests.
 */

#include "test_fill_divides_exact.h"
#include "test_fill_indivisible.h"
#include "test_fill_line_eq_buf.h"
#include "test_fill_one_byte.h"
#include "test_fill_ub_note.h"

int main(void)
{
    test_one_byte_line_fills_completely();
    test_line_divides_buffer_exactly();
    test_line_does_not_divide_buffer();
    test_line_equals_buffer_size();
    test_line_longer_than_buffer_is_ub();
    return 0;
}
