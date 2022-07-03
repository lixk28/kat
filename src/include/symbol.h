#ifndef SYMBOL_H
#define SYMBOL_H

#include "lex.h"
#include "hashmap.h"
#include <stddef.h>
#include <stdbool.h>

typedef enum KAT_TYPE
{
  KAT_INT,
  KAT_CHAR,
  KAT_STR,
  KAT_BOOL,
  KAT_NIL
} KAT_TYPE;

typedef struct type_t
{
  char *name;
  size_t size;
  KAT_TYPE kind;

  struct type_t *next;
} type_t;

typedef struct symbol_t
{
  char *name;
  bool is_var;
  bool is_func;

  // variable's type or function's return type
  union {
    type_t *type;
    type_t *return_type;
  };

  size_t params_num;
  type_t *params_type;

  int offset;

  token_t *token;

  struct symbol_t *next;
} symbol_t;

typedef struct scope_t
{
  hashmap_t symbol_table;
  struct scope_t *next;
} scope_t;

#endif
