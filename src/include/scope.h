#ifndef SCOPE_H
#define SCOPE_H

#include "lex.h"
#include "hashmap.h"
#include "symbol.h"

typedef struct scope_t
{
  hashmap_t *symbol_table;
  struct scope_t *next;
} scope_t;

extern scope_t *var_scope;
extern scope_t *func_scope;

void enter_scope();
void leave_scope();
void add_symbol(scope_t *scope, symbol_t *symbol);
void del_symbol(scope_t *scope, symbol_t *symbol);
symbol_t *find_symbol_by_name(scope_t *scope, char *symbol_name);
symbol_t *find_symbol_by_tok(scope_t *scope, token_t *token);

#endif