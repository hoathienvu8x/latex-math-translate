#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "token.h"
#include "iobuf.h"

static size_t utf8_charlen(unsigned char c) {
  if (c < 0x80) return 1;
  if ((c >> 5) == 0x6) return 2;
  if ((c >> 4) == 0xe) return 3;
  if ((c >> 3) == 0x1e) return 4;
  return 1;
}

static unsigned int utf8_codepoint(const char *p, size_t *len) {
  unsigned char c = (unsigned char)p[0];
  unsigned int codepoint = 0;
  *len = utf8_charlen(c);
  switch (*len) {
    case 1: {
      codepoint = c;
      break;
    }
    case 2: {
      codepoint = ((c & 0x1F) << 6) | (p[1] & 0x3F);
      break;
    }
    case 3: {
      codepoint = ((c & 0x0F) << 12) | ((p[1] & 0x3F) << 6) | (p[2] & 0x3F);
      break;
    }
    case 4: {
      codepoint = (
        ((c & 0x07) << 18) | ((p[1] & 0x3F) << 12) |
        ((p[2] & 0x3F) << 6) | (p[3] & 0x3F)
      );
      break;
    }
  }
  return codepoint;
}

static int is_emoji(const char *p, size_t *len) {
  size_t clen = 0;
  unsigned int cp = utf8_codepoint(p, &clen);
  if (
    cp == 0x200d || cp == 0xfe0f ||
    (cp >= 0x1f600 && cp <= 0x1f64f) ||
    (cp >= 0x1f300 && cp <= 0x1f5ff) ||
    (cp >= 0x1f680 && cp <= 0x1f6ff) ||
    (cp >= 0x1f900 && cp <= 0x1f9ff) ||
    (cp >= 0x1fa70 && cp <= 0x1faff) ||
    (cp >= 0x1f3a0 && cp <= 0x1f3ff) ||
    (cp >= 0x1f700 && cp <= 0x1f77f) ||
    (cp >= 0x2700 && cp <= 0x27bf) ||
    (cp >= 0x2600 && cp <= 0x26ff) ||
    (cp >= 0x2300 && cp <= 0x23ff) ||
    (cp >= 0x2194 && cp <= 0x21aa) ||
    (cp >= 0x1f1e6 && cp <= 0x1f1ff) ||
    (cp == 0xfe0f) || (cp == 0x200d) ||
    (cp == 0x2764) || (cp == 0x1f494) || (cp == 0x1f495) ||
    (cp == 0x1f49b) || (cp == 0x1f49c) || (cp == 0x1f49a) ||
    (cp == 0x1f499) || (cp == 0x1f48b) || (cp == 0x1f48c) ||
    (cp == 0x1f4a9) || (cp == 0x1f60d) || (cp == 0x1f923) ||
    (cp == 0x1f970) || (cp == 0x1fae0) || (cp == 0x1fae1) ||
    (cp == 0x1fae2) || (cp == 0x1fae3) || (cp == 0x1fae4)
  ) {
    *len = clen;
    return 1;
  }
  return 0;
}
const char *token_type_string(enum token_type_t type) {
  if (type == token_word_type) return "Word";
  if (type == token_number_type) return "Number";
  if (type == token_emoji_type) return "Emoji";
  if (type == token_latex_type) return "Latex";
  if (type == token_latex_multiline_type) return "Latex multiline";
  return "Other";
}
int string_split_token(
  const char *input, size_t sz, struct token_t **tokens, size_t *len
) {
  const char *p, *end;
  struct token_t *_tokens = NULL, *tmp;
  size_t count = 0;

  if (!input || !sz || !tokens || !len) return -1;
  p = input, end = input + sz;
  *tokens = NULL, *len = 0;
  while (p < end && *p) {
    const char *e, *start = NULL;
    size_t tlen = 0, clen;
    enum token_type_t type = token_other_type;
    while (p < end && *p && isspace(*p)) p++;
    if (p >= end || !*p) goto clean_up;
    start = p;
    clen = utf8_charlen((unsigned char)*p);
    if (*p == '$' && (p + 1) < end && *(p + 1) != '$') {
      p++, start = p;
      while (p < end && *p && *p != '$') {
        p += utf8_charlen((unsigned char)*p);
      }
      e = p;
      if (p < end && *p == '$') {
        p += 1;
      }
      tlen = (size_t)(e - start);
      type = token_latex_type;
    } else if (*p == '$' && (p + 1) < end && *(p + 1) == '$') {
      p += 2, start = p;
      while (p < end && !(*p == '$' && (p + 1) < end && *(p + 1) == '$')) {
        p += utf8_charlen((unsigned char)*p);
      }
      e = p;
      if (p < end && *p == '$' && *(p + 1) == '$') {
        p += 2;
      }
      tlen = (size_t)(e - start);
      type = token_latex_multiline_type;
    } else if (*p == '\\' && (p + 1) < end && *(p + 1) == '[') {
      p += 2, start = p;
      while (p < end && !(*p == '\\' && (p + 1) < end && *(p + 1) == ']')) {
        p += utf8_charlen((unsigned char)*p);
      }
      e = p;
      if (p < end && *p == '\\' && *(p + 1) == ']') {
        p += 2;
      }
      tlen = (size_t)(e - start);
      type = token_latex_multiline_type;
    } else if (is_emoji(p, &clen)) {
      type = token_emoji_type;
      p += clen, tlen = clen;
      while (is_emoji(p, &clen)) {
        p += clen, tlen += clen;
      }
    } else if (isdigit(*p)) {
      int dot_seen = 0;
      type = token_number_type;
      while (p < end && *p && !isspace(*p) && (
        isdigit(*p) || (*p == '.' && !dot_seen) || *p == ','
      )) {
        if (*p == '.') dot_seen = 1;
        clen = utf8_charlen((unsigned char)*p);
        p += clen, tlen += clen;
      }
    } else if (isalpha(*p) || (*p & 0x80)) {
      type = token_word_type;
      while (p < end && *p && !isspace(*p) && (isalpha(*p) || (*p & 0x80))) {
        clen = utf8_charlen((unsigned char)*p);
        p += clen, tlen += clen;
      }
    } else {
      unsigned char first = *p;
      if (
        first == '.' || first == '(' || first == '[' ||
        first == ')' || first == ']'
      ) {
        while (p < end && *p == first) {
          clen = utf8_charlen((unsigned char)*p);
          p += clen, tlen += clen;
        }
      } else {
        while (
          p < end && !isspace(*p) && *p != '.' && !isdigit(*p) &&
          !(isalpha(*p) || (*p & 0x80))
        ) {
          if (*p == ')' || *p == ']') break;
          clen = utf8_charlen((unsigned char)*p);
          p += clen, tlen += clen;
        }
      }
    }
    tmp = (struct token_t *)realloc(
      _tokens, (count + 1) * sizeof(struct token_t)
    );
    if (!tmp) {
      goto clean_up;
    }
    tmp[count].data.buf = start;
    tmp[count].data.len = tlen;
    tmp[count].type = type;
    _tokens = tmp;
    count++;
  }
  *tokens = _tokens;
  *len = count;
  return 0;

clean_up:
  if (_tokens) free(_tokens);
  return -1;
}

void token_destroy(struct token_t *tokens) {
  if (tokens) free(tokens);
}
static int tokens_to_iobuf(
  const struct token_t *tokens, size_t n, struct iobuf_t *sb, int raw
) {
  size_t i;
  const char *latex = "$$";
  for (i = 0; i < n; i++) {
    const char *text = tokens[i].data.buf;
    size_t len = tokens[i].data.len;
    if (i > 0 && iobuf_push(sb, ' ') == 0) return -1;
    if (
      tokens[i].type == token_latex_type ||
      tokens[i].type == token_latex_multiline_type
    ) {
      while (len > 0 && isspace(text[0])) {
        text++, len--;
      }
      while (len > 0 && isspace(text[len - 1])) len--;
      if (raw) {
        if (iobuf_push(sb, '*') == 0) return -1;
      } else {
        size_t slen = 1;
        if (tokens[i].type == token_latex_multiline_type) {
          slen = 2;
        }
        if (
          iobuf_append(sb, latex, slen) == 0 ||
          iobuf_append(sb, text, len) == 0 ||
          iobuf_append(sb, latex, slen) == 0
        ) {
          return -1;
        }
      }
    } else {
      if (iobuf_append(sb, text, len) == 0) return -1;
    }
  }
  return 0;
}

int tokens_to_string(
  const struct token_t *tokens, size_t n, char **out, size_t *len
) {
  if (tokens) {
    struct iobuf_t io;
    if (iobuf_init(&io, 64)) return -1;
    if (tokens_to_iobuf(tokens, n, &io, 0)) {
      iobuf_free(&io);
      return -1;
    }
    *out = (char *)io.buf;
    if (len) *len = io.len;
    return 0;
  }
  return -1;
}

int tokens_to_string_raw(
  const struct token_t *tokens, size_t n, char **out, size_t *len
) {
  if (tokens) {
    struct iobuf_t io;
    if (iobuf_init(&io, 64)) return -1;
    if (tokens_to_iobuf(tokens, n, &io, 1)) {
      iobuf_free(&io);
      return -1;
    }
    *out = (char *)io.buf;
    if (len) *len = io.len;
    return 0;
  }
  return -1;
}
