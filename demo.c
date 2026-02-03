#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "token.h"

static int read_file(const char *fpath, char **content, size_t *len) {
  long size;
  char *buf = NULL;
  *content = NULL;
  *len = 0;
  FILE *f = fopen(fpath, "rb");
  if (!f) return -1;

  fseek(f, 0, SEEK_END);
  size = ftell(f);
  rewind(f);

  buf = malloc(size);
  if (!buf) {
    fclose(f);
    return -1;
  }

  if (fread(buf, 1, size, f) != (size_t)size) {
    fclose(f);
    free(buf);
    return -1;
  }
  fclose(f);
  *content = buf;
  *len = (size_t)size;
  return 0;
}

static int is_sentence_end(const struct token_t *tok) {
  if (
    tok->type == token_latex_type ||
    tok->type == token_latex_multiline_type ||
    tok->type != token_other_type ||
    tok->data.len != 1
  ) {
    return 0;
  }
  return (
    tok->data.buf[0] == '.' ||
    tok->data.buf[0] == '!' ||
    tok->data.buf[0] == '?'
  );
}

int main(int argc, char **argv) {
  struct token_t *tokens = NULL;
  char *content = NULL;
  size_t i, j, ntok = 0, len = 0;
  if (argc < 2) {
    return -1;
  }
  if (read_file(argv[1], &content, &len)) {
    return -1;
  }
  if (string_split_token(content, len, &tokens, &ntok)) {
    free(content);
    return -1;
  }
  #define make_sentence(tokens, i, j) \
    if (i > j) { \
      char *text = NULL; \
      size_t len = 0; \
      if (tokens_to_string_raw( \
        &tokens[j], i - j + 1, &text, &len \
      ) == 0) { \
        printf("-> %.*s\n", (int)len, text); \
        free(text); \
      } \
    }

  for (j = 0, i = 0; i < ntok; i++) {
    if (is_sentence_end(&tokens[i])) {
      make_sentence(tokens, i, j);
      j = i + 1;
    }
  }
  make_sentence(tokens, i, j);
  #undef make_sentence
  free(content);
  token_destroy(tokens);
  return 0;
}
