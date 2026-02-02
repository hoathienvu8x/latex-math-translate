#include <stdio.h>
#include <string.h>
#include "token.h"

int main(int argc, char **argv) {
  struct token_t *tokens = NULL;
  size_t i, ntok = 0;
  if (argc < 2) {
    return -1;
  }
  if (string_split_token(argv[1], strlen(argv[1]), &tokens, &ntok)) {
    return -1;
  }
  for (i = 0; i < ntok; i++) {
    printf(
      "%.*s (%s)\n", (int)tokens[i].data.len, tokens[i].data.buf,
      token_type_string(tokens[i].type)
    );
  }
  token_destroy(tokens);
  return 0;
}
