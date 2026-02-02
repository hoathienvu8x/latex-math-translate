#ifndef _IOBUF_H
#define _IOBUF_H

struct iobuf_t {
  unsigned char *buf;
  size_t len, align;
};
int iobuf_init(struct iobuf_t *io, size_t align);
size_t iobuf_append(
  struct iobuf_t *io, const void *buf, size_t len
);
size_t iobuf_push(struct iobuf_t *io, char ch);
size_t iobuf_push_string(struct iobuf_t *io, const char *s);
size_t iobuf_printf(struct iobuf_t *io, const char *fmt, ...);
void iobuf_free(struct iobuf_t *io);

#endif
