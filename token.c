#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "token.h"
#include "iobuf.h"
#include "unicode.h"

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

  if (!input || sz == 0 || !tokens || !len) return -1;

  p = input;
  end = input + sz;
  *tokens = NULL;
  *len = 0;

  while (p < end && *p) {
    const char *start;
    size_t tlen = 0, clen = 0;
    enum token_type_t type = token_other_type;

    while (p < end && *p && isspace((unsigned char)*p)) p++;
    if (p >= end || !*p) break;
    if (*p == '$' && (p + 1) < end && *(p + 1) != '$') {
      const char *q;
      p++, start = p;
      while (p < end && *p && *p != '$') {
        p += utf8_charlen((unsigned char)*p);
      }
      tlen = (size_t)(p - start);
      if (p < end && *p == '$') p++;
      q = p;
      while (q < end && isspace((unsigned char)*q)) q++;
      if (q < end && *q == '-') {
        const char *r = q + 1;
        while (r < end && isspace((unsigned char)*r)) r++;
        if (r < end && (isalpha((unsigned char)*r) || (*r & 0x80))) {
          p = r;
          while (p < end && (isalpha((unsigned char)*p) || (*p & 0x80))) {
            p += utf8_charlen((unsigned char)*p);
          }
          tlen = (size_t)(p - start);
        }
      }
      type = token_latex_type;
    } else if (
      (*p == '$' && (p + 1) < end && *(p + 1) == '$') ||
      (*p == '\\' && (p + 1) < end && *(p + 1) == '[')
    ) {
      char end_char = (*p == '$') ? '$' : ']';
      p += 2, start = p;
      while (p < end) {
        if (
          end_char == '$' && *p == '$' &&
          (p + 1) < end && *(p + 1) == '$'
        ) {
          break;
        }
        if (
          end_char == ']' && *p == '\\' &&
          (p + 1) < end && *(p + 1) == ']'
        ) {
          break;
        }
        p += utf8_charlen((unsigned char)*p);
      }
      tlen = (size_t)(p - start);
      if (p < end) p += 2;
      type = token_latex_multiline_type;
    } else if (*p == '\\') {
      size_t slash_count = 0;
      start = p;
      while (p + slash_count < end && *(p + slash_count) == '\\') {
        slash_count++;
      }

      if (slash_count == 2) {
        p += 2, tlen = 2;
        type = token_latex_type;
      } else if (
        slash_count == 1 && (p + 1 < end) &&
        isalpha((unsigned char)*(p + 1))
      ) {
        type = token_latex_type, p++;
        while (p < end && isalpha((unsigned char)*p)) p++;
        if (p < end && *p == '{') {
          int braces = 0;
          while (p < end) {
            if (*p == '{') {
              braces++;
            } else if (*p == '}') {
              braces--;
            }
            p++;
            if (braces == 0) break;
          }
        }
        tlen = (size_t)(p - start);
      } else {
        char c = *p;
        while (p < end && *p == c) p++;
        tlen = (size_t)(p - start);
        type = token_other_type;
      }
    } else if (is_emoji(p, &clen) && p + clen <= end) {
      start = p, type = token_emoji_type;
      do {
        p += clen;
        tlen += clen;
      } while (p < end && is_emoji(p, &clen) && p + clen <= end);
    } else if (isalpha((unsigned char)*p) || (*p & 0x80)) {
      start = p, type = token_word_type;
      while (p < end) {
        if (isalpha((unsigned char)*p) || (*p & 0x80)) {
          p += utf8_charlen((unsigned char)*p);
        } else if (*p == '\'') {
          const char *next = p + 1;
          while (next < end && isspace((unsigned char)*next)) next++;
          if (next < end && (isalpha((unsigned char)*next) || (*next & 0x80))) {
            p = next;
            continue;
          }
          break;
        } else if (isspace((unsigned char)*p) || *p == '-') {
          const char *q = p;
          while (q < end && isspace((unsigned char)*q)) q++;
          if (q < end && *q == '-') {
            q++;
            while (q < end && isspace((unsigned char)*q)) q++;
            if (q < end && (isalpha((unsigned char)*q) || (*q & 0x80))) {
              p = q;
              continue;
            }
          }
          break; 
        } else {
          break;
        }
      }
      tlen = (size_t)(p - start);
    } else if (isdigit((unsigned char)*p)) {
      int dot_seen = 0;
      start = p, type = token_number_type;
      while (
        p < end && (isdigit((unsigned char)*p) ||
        (*p == '.' && !dot_seen) || *p == ',')
      ) {
        if (*p == '.') dot_seen = 1;
        p++;
      }
      tlen = (size_t)(p - start);
    } else if (ispunct((unsigned char)*p)) {
      type = token_other_type, start = p;
      while (
        p < end && ispunct((unsigned char)*p) &&
        *p != '$' && *p != '\\'
      ) {
        p++;
        tlen = (size_t)(p - start);
      }
    } else {
      start = p;
      clen = utf8_charlen((unsigned char)*p);
      p += clen, tlen = clen;
    }

    if (tlen == 0) continue;

    tmp = realloc(_tokens, (count + 1) * sizeof(struct token_t));
    if (!tmp) goto clean_up;

    _tokens = tmp;
    _tokens[count].type = type;
    _tokens[count].data.buf = start;
    _tokens[count].data.len = tlen;
    count++;
  }

  *tokens = _tokens;
  *len = count;
  return 0;

clean_up:
  free(_tokens);
  return -1;
}

void token_destroy(struct token_t *tokens) {
  if (tokens) free(tokens);
}
static int is_token_latex(const struct token_t *tok) {
  return (
    tok->type == token_latex_type ||
    tok->type == token_latex_multiline_type
  );
}
static int is_token_punct(const struct token_t *tok) {
  if (tok->data.len != 1) return 0;
  return (
    *tok->data.buf == '.' ||
    *tok->data.buf == ',' ||
    *tok->data.buf == ':' ||
    *tok->data.buf == ';' ||
    *tok->data.buf == '?' ||
    *tok->data.buf == '!'
  );
}
/*
static int is_token_lbrake(const struct token_t *tok) {
  if (tok->data.len != 1) return 0;
  return (
    *tok->data.buf == '(' || *tok->data.buf == '[' || *tok->data.buf == '{'
  );
}
static int is_token_rbrake(const struct token_t *tok) {
  if (tok->data.len != 1) return 0;
  return (
    *tok->data.buf == ')' || *tok->data.buf == ']' || *tok->data.buf == '}'
  );
}*/
static int tokens_to_iobuf(
  const struct token_t *tokens, size_t n, struct iobuf_t *sb, int raw
) {
  size_t i;
  const char *latex = "$$";
  for (i = 0; i < n; i++) {
    const char *text = tokens[i].data.buf;
    size_t len = tokens[i].data.len;
    if (is_token_latex(&tokens[i])) {
      if (raw && i > 0 && is_token_latex(&tokens[i - 1])) {
        continue;
      }
      while (len > 0 && isspace(text[0])) {
        text++, len--;
      }
      while (len > 0 && isspace(text[len - 1])) len--;
      if (len == 2 && strncmp(text, "\\\\", 2) == 0) {
        continue;
      }
      if (sb->len > 0 && iobuf_push(sb, ' ') == 0) return -1;
      if (raw) {
        if (iobuf_push(sb, '*') == 0) return -1;
      } else {
        size_t slen = 1;
        if (tokens[i].type == token_latex_multiline_type) {
          slen = 2;
        }
        if (
          iobuf_append(sb, latex, slen) == 0 ||
          iobuf_append(sb, text, len) == 0
        ) {
          return -1;
        }
        if (strstr(text, "$-") == NULL) {
          if (iobuf_append(sb, latex, slen) == 0) return -1;
        }
      }
    } else {
      if (
        sb->len > 0 && !is_token_punct(&tokens[i]) && iobuf_push(sb, ' ') == 0
      ) {
        return -1;
      }
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
