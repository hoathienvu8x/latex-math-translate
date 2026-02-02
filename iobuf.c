#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include "iobuf.h"

static int iobuf_resize(struct iobuf_t *io, size_t n) {
  if (!io) return -1;
  if (io->len + n >= io->align) {
    size_t align = io->align == 0 ? n : (n + io->align);
    unsigned char *buf = (unsigned char *)realloc(
      io->buf, align * sizeof(unsigned char)
    );
    if (!buf) return -1;
    io->buf = buf;
    io->align = align;
  }
  return 0;
}
int iobuf_init(struct iobuf_t *io, size_t align) {
  if (io) {
    io->buf = NULL;
    io->align = 0;
    io->len = 0;
    return iobuf_resize(io, align);
  }
  return -1;
}
size_t iobuf_append(
  struct iobuf_t *io, const void *buf, size_t len
) {
  if (io && buf && iobuf_resize(io, len) == 0) {
    memcpy(io->buf + io->len, buf, len);
    io->len += len;
    return len;
  }
  return 0;
}
size_t iobuf_push(struct iobuf_t *io, char ch) {
  return iobuf_append(io, &ch, 1);
}
size_t iobuf_push_string(struct iobuf_t *io, const char *s) {
  return iobuf_append(io, s, s ? strlen(s) : 0);
}
size_t iobuf_printf(struct iobuf_t *io, const char *fmt, ...) {
  va_list args;
  int needed, written;
  if (!io || !fmt) return 0;
  va_start(args, fmt);
  needed = vsnprintf(NULL, 0, fmt, args);
  va_end(args);
  if (needed < 0 || iobuf_resize(io, (size_t)needed + 1) != 0) return 0;
  va_start(args, fmt);
  written = vsnprintf(
    (char *)(io->buf + io->len), (size_t)needed + 1, fmt, args
  );
  va_end(args);
  if (written > 0) {
    io->len += (size_t)written;
    return (size_t)written;
  }
  return 0;
}
void iobuf_free(struct iobuf_t *io) {
  if (io && io->buf) {
    free(io->buf);
  }
}
