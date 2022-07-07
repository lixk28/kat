#include "lex.h"
#include "symbol.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

symbol_t *make_var_symbol(token_t *var_tok, type_t *var_type)
{
  symbol_t *symbol = calloc(1, sizeof(symbol_t));
  symbol->is_var = true;
  symbol->is_func = false;
  symbol->name = tok2cstr(var_tok);
  symbol->type = var_type;
  symbol->token = var_tok;
  switch (var_type->kind) {
    case KAT_INT: symbol->offset = -4; break;
    case KAT_CHAR: symbol->offset = -1; break;
    case KAT_STR: symbol->offset = -4; break;
    case KAT_BOOL: symbol->offset = -1; break;
    case KAT_NIL: symbol->offset = 0; break;
  }
  symbol->next = NULL;
  return symbol;
}

symbol_t *make_fn_symbol(token_t *func_tok, type_t *return_type, type_t *params_type, size_t params_num)
{
  symbol_t *symbol = calloc(1, sizeof(symbol_t));
  symbol->is_var = false;
  symbol->is_func = true;
  symbol->name = tok2cstr(func_tok);
  symbol->return_type = return_type;
  symbol->params_num = params_num;
  symbol->params_type = params_type;
  // TODO: offset is sum of params offset plus return address
  symbol->token = func_tok;
  symbol->next = NULL;
  return symbol;
}