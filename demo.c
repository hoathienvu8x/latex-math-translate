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

int main(int argc, char **argv) {
  struct token_t *tokens = NULL;
  char *content = NULL;
  size_t i, ntok = 0, len = 0;
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
  for (i = 0; i < ntok; i++) {
    printf(
      "%.*s (%s)\n", (int)tokens[i].data.len, tokens[i].data.buf,
      token_type_string(tokens[i].type)
    );
  }
  free(content);
  token_destroy(tokens);
  return 0;
}
