#ifndef _UNICODE_H
#define _UNICODE_H

#include <stddef.h>

struct utf8_lower_range {
  unsigned int start, end, delta;
};

size_t utf8_charlen(unsigned char c);
unsigned int utf8_codepoint(const char *p, size_t *len);
int utf8_tolower(const char *input, size_t len, char **out, size_t *sz);

#endif
