#ifndef YES_TOTO_LINE_H
#define YES_TOTO_LINE_H

#include <stddef.h>

int build_output_line(int argc, char **argv, const char **out_line,
                      size_t *out_len, void **heap_block);

#endif /* YES_TOTO_LINE_H */
