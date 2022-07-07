#include "lex.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

static bool start_with(char *p, const char *prefix)
{
  return strncmp(p, prefix, strlen(prefix)) == 0;
}

static bool iskeyword(token_t *token)
{
  static char *keywords[] = {
    "if", "else", "elif", "while", "break", "continue",
    "func", "return", "let",
    "int", "float", "char", "str", "bool", "true", "false"
  };

  for (unsigned i = 0; i < sizeof(keywords) / sizeof(*keywords); i++) {
    if ((strlen(keywords[i]) == token->len) &&
        !strncmp(token->begin, keywords[i], token->len))
      return true;
  }
  return false;
}

static size_t islpunct(char *p)
{
  static char *long_puncts[] = {
    "+=", "-=", "*=", "/=",
    ">=", "<=", "==", "!=", "&&", "||",
    "=>"
  };
  for (unsigned i = 0; i < sizeof(long_puncts) / sizeof(*long_puncts); i++) {
    if (start_with(p, long_puncts[i]))
      return strlen(long_puncts[i]);
  }
  return 0;
}

static token_t *make_token(TK_TYPE token_type, char *token_begin, char *token_end, size_t line)
{
  token_t *token = calloc(1, sizeof(token_t));
  token->type = token_type;
  token->begin = token_begin;
  token->len = token_end - token_begin;
  token->line = line;
  token->ival = 0;
  token->cval = '\0';
  token->sval = NULL;
  token->next = NULL;
  return token;
}

static void convert_number(token_t *token)
{
  char *dot = strchr(token->begin, '.');
  if (!dot) { // there is no dot, integer
    token->ival = strtoll(token->begin, NULL, 10);
  } else {  // there is dot, float
    char *end = NULL;
    token->fval = strtold(token->begin, &end);
    if (token->begin + token->len != end) {
      // TODO: improve error message
      fprintf(stderr, "invalid numeric value at line %ld\n", token->line);
      exit(1);
    }
  }
}

// return the c-style string of the given token
char *tok2cstr(token_t *token)
{
  char *name = malloc(sizeof(char) * (token->len + 1));
  memcpy(name, token->begin, sizeof(char) * token->len);
  name[token->len] = '\0';
  return name;
}

token_t * lex(char *buf)
{
  token_t head = {};
  token_t *curr = &head;

  char *p = buf;
  size_t line = 1;

  while (*p != '\0') {
    // skip whitespaces
    if (*p == ' ' || *p == '\t' || *p == '\f' || *p == '\r') {
      p++;
      continue;
    }

    // skip newline
    if (*p == '\n') {
      line++;
      p++;
      continue;
    }

    // TODO: skip comments
    // if (start_with(p, "//") || start_with(p, "/*"))
    // {

    // }

    // read numbers
    // number = ["+" | "-"] {digit}- ["." {digit}-]
    if (isdigit(*p) || ((*p == '+' || *p == '-') && isdigit(*(p + 1)))) {
      char *q = p++;
      while (isdigit(*p) || *p == '.')
        p++;
      token_t *token = make_token(TK_NUM, q, p, line);
      curr->next = token;
      curr = curr->next;
      convert_number(token);
      continue;
    }

    // read characters
    // TODO: escape character support
    if (*p == '\'') {
      char *q = p++;
      char *quote = strchr(p, '\'');
      if (!quote) {
        // TODO: improve error message
        fprintf(stderr, "unclosed character literal at line %ld\n", line);
        exit(1);
      } else if (quote - p > 1) {
        fprintf(stderr, "too many characters at line %ld\n", line);
        exit(1);
      } else {
        token_t *token = make_token(TK_CHR, q, quote + 1, line);
        token->cval = *p;
        curr->next = token;
        curr = curr->next;
        p = quote + 1;
      }
      continue;
    }

    // read strings
    // TODO: escape character support
    if (*p == '"') {
      char *q = p++;
      char *quote = strchr(p, '"');
      if (!quote) {
        // TODO: improve error message
        fprintf(stderr, "unclosed string literal at line %ld\n", line);
        exit(1);
      } else {
        token_t *token = make_token(TK_STR, q, quote + 1, line);
        size_t length = quote - p;
        token->sval = malloc(sizeof(char) * (length + 1));
        strncpy(token->sval, p, length);
        token->sval[length] = '\0';
        curr->next = token;
        curr = curr->next;
        p = quote + 1;
      }
      continue;
    }

    // read identifiers and keywords
    // identifier = (letter | underscore) , {letter | underscore | digit}
    // first to check if it is a valid identifier, then match it with keywords
    if (isalpha(*p) || *p == '_') {
      char *q = p++;
      while (isalnum(*p) || *p == '_')
        p++;
      token_t *token = make_token(TK_ID, q, p, line);
      if (iskeyword(token))
        token->type = TK_KW;
      curr->next = token;
      curr = curr->next;
      continue;
    }

    // read operators and punctuators
    // use longest match policy
    if (ispunct(*p)) {
      size_t len = islpunct(p);
      len = (len == 0) ? 1 : len;
      token_t *token = make_token(TK_PUNCT, p, p + len, line);
      curr->next = token;
      curr = curr->next;
      p += len;
      continue;
    }

    fprintf(stderr, "invalid character %c for lexer at line %ld\n", *p, line);
    exit(1);
  }

  // EOF token
  curr->next = calloc(1, sizeof(token_t));
  curr->next->type = TK_EOF;
  curr->next->next = NULL;

  return head.next;
}

void dump_token_list(token_t *tokens)
{
  fprintf(stdout, "token list dump:\n");
  while (tokens) {
    switch (tokens->type) {
      case TK_KW:
        fprintf(stdout, "{<keyword>: ");
        fwrite(tokens->begin, sizeof(char), tokens->len, stdout);
        fprintf(stdout, " at line %ld}\n", tokens->line);
        break;
      case TK_ID:
        fprintf(stdout, "{<identifier>: ");
        fwrite(tokens->begin, sizeof(char), tokens->len, stdout);
        fprintf(stdout, " at line %ld}\n", tokens->line);
        break;
      case TK_NUM:
        fprintf(stdout, "{<number>: ");
        fwrite(tokens->begin, sizeof(char), tokens->len, stdout);
        fprintf(stdout, " at line %ld, ival = %ld, fval = %Lf}\n", tokens->line, tokens->ival, tokens->fval);
        break;
      case TK_CHR:
        fprintf(stdout, "{<character>: ");
        fwrite(tokens->begin, sizeof(char), tokens->len, stdout);
        fprintf(stdout, " at line %ld, cval = %c}\n", tokens->line, tokens->cval);
        break;
      case TK_STR:
        fprintf(stdout, "{<string>: ");
        fwrite(tokens->begin, sizeof(char), tokens->len, stdout);
        fprintf(stdout, " at line %ld, sval = %s}\n", tokens->line, tokens->sval);
        break;
      case TK_PUNCT:
        fprintf(stdout, "{<punctuator>: ");
        fwrite(tokens->begin, sizeof(char), tokens->len, stdout);
        fprintf(stdout, " at line %ld}\n", tokens->line);
        break;
      case TK_EOF:
        fprintf(stdout, "{<eof>}\n");
        break;
      default:
        break;
    }
    tokens = tokens->next;
  }
}
