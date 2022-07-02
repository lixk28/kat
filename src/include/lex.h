#ifndef LEX_H
#define LEX_H

#include <stdint.h>
#include <stddef.h>

typedef enum TK_TYPE
{
  TK_KW,
  TK_ID,
  TK_NUM,
  TK_CHR,
  TK_STR,
  TK_PUNCT,
  TK_EOF,
} TK_TYPE;

typedef struct token_t
{
  TK_TYPE type;
  char *begin;
  size_t len;
  size_t line;

  union {
    int64_t ival;
    long double fval;
  };
  char cval;
  char *sval;

  struct token_t *next;
} token_t;

token_t *lex(char *buf);

void dump_token_list(token_t *tokens);

#endif
