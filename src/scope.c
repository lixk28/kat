#include "scope.h"
#include "lex.h"
#include <stdio.h>
#include <stdlib.h>

// variable scope
// the current scope is the head of scope list
scope_t *var_scope = &(scope_t) {
  .symbol_table = NULL,
  .next = NULL
};

// function scope
// only one function scope in the program
scope_t *func_scope = &(scope_t) {
  .symbol_table = NULL,
  .next = NULL
};

// dump symbol table
static void dump_symbol_table()
{
  hashmap_t *table = var_scope->symbol_table;
  for (unsigned i = 0; i < table->capcity; i++) {
    if (table->bucket[i].used == true) {
      symbol_t *symbol = (symbol_t *) table->bucket[i].val;
      if (symbol->is_var == true) {
        fprintf(stdout, "%s %s\n", symbol->name, symbol->type->name);
      } else if (symbol->is_func == true) {
        fprintf(stdout, "%s (", symbol->name);
        for (type_t *param = symbol->params_type; param != NULL; param = param->next)
          fprintf(stdout, "%s%s", param->name, param->next == NULL ? ")" : ", ");
        fprintf(stdout, " => %s\n", symbol->return_type->name);
      }
    }
  }
  fputc('\n', stdout);
}

// make a new scope
#define MAX_SYMBOL_NUM 128
static scope_t *make_scope()
{
  scope_t *new_scope = calloc(1, sizeof(scope_t));
  new_scope->symbol_table = new_hashmap(MAX_SYMBOL_NUM);
  new_scope->next = NULL;
  return new_scope;
}

// enter block scope
void enter_scope()
{
  scope_t *new_scope = make_scope();
  new_scope->next = var_scope;
  var_scope = new_scope;
}

// leave block scope
void leave_scope()
{
  // each time leave a scope, dump the symbol table
  #ifdef DEBUG
    dump_symbol_table();
  #endif
  var_scope = var_scope->next;
}

// add a new symbol to scope
void add_symbol(scope_t *scope, symbol_t *symbol)
{
  hashmap_add_cstr(scope->symbol_table, symbol->name, symbol);
}

// delete a symbol to scope
void del_symbol(scope_t *scope, symbol_t *symbol)
{
  hashmap_remove_cstr(scope->symbol_table, symbol->name);
}

// find a symbol given its name
// iterate through scopes
// if not found return null pointer
symbol_t *find_symbol_by_name(scope_t *scope, char *symbol_name)
{
  for (scope_t *curr = scope; curr && curr->symbol_table != NULL; curr = curr->next) {
    entry_t *entry = hashmap_get_cstr(curr->symbol_table, symbol_name);
    if (entry)
      return (symbol_t *) entry->val;
  }
  return NULL;
}

// find a symbol given its token
symbol_t *find_symbol_by_tok(scope_t *scope, token_t *token)
{
  for (scope_t *curr = scope; curr && curr->symbol_table != NULL; curr = curr->next) {
    entry_t *entry = hashmap_get(curr->symbol_table, token->begin, token->len);
    if (entry)
      return (symbol_t *) entry->val;
  }
  return NULL;
}
