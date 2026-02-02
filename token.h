#ifndef _TOKEN_H
#define _TOKEN_H

struct str_t {
  const char *buf;
  size_t len;
};

enum token_type_t {
  token_word_type,
  token_number_type,
  token_emoji_type,
  token_latex_type,
  token_latex_multiline_type,
  token_other_type
};

struct token_t {
  enum token_type_t type;
  struct str_t data;
};
const char *token_type_string(enum token_type_t type);
int string_split_token(
  const char *input, size_t sz, struct token_t **tokens, size_t *len
);
void token_destroy(struct token_t *tokens);

#endif
