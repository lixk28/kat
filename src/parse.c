#include "hashmap.h"
#include "lex.h"
#include "parse.h"
#include "stack.h"
#include "symbol.h"
#include "scope.h"
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdnoreturn.h>

static char *type_list[] = {
  [KAT_INT]  = "int",
  [KAT_CHAR] = "char",
  [KAT_STR]  = "str",
  [KAT_BOOL] = "bool",
  [KAT_NIL]  = "nil"
};

static bool is_valid_type(char *type)
{
  for (unsigned i = 0; i < sizeof(type_list) / sizeof(*type_list); i++) {
    if (!strcmp(type, type_list[i]))
      return true;
  }
  return false;
}

static token_t *peek(token_t **token)
{
  if (token)
    return (*token)->next;
  return NULL;
}

// match the token with the given string
static bool expect_str(token_t **token, const char *str)
{
  return ((*token)->len == strlen(str) &&
          !memcmp((*token)->begin, str, sizeof(char) * (*token)->len));
}

// match the token with the given token type
static bool expect_type(token_t **token, TK_TYPE tok_type)
{
  return (*token)->type == tok_type;
}

// match the token with the given string
static bool expect_next_str(token_t **token, const char *str)
{
  token_t *next_tok = peek(token);
  return (next_tok->len == strlen(str) &&
          !memcmp(next_tok->begin, str, sizeof(char) * next_tok->len));
}

// match the token with the given token type
static bool expect_next_type(token_t **token, TK_TYPE tok_type)
{
  token_t *next_tok = peek(token);
  return next_tok->type == tok_type;
}

// advance the current token
static bool advance(token_t **token)
{
  if (token && (*token)->type != TK_EOF) {
    *token = (*token)->next;
    return true;
  }
  return false;
}

// consume current token
// if match, consume the current token and return true
// otherwise, do nothing and return false
static bool consume(token_t **token, const char *str)
{
  if ((*token)->type != TK_EOF && expect_str(token, str)) {
    advance(token);
    return true;
  }
  return false;
}

// kat does not support compound data type currently
// so the type getter is hard coded
static KAT_TYPE tok2type(token_t *token)
{
  if (!expect_type(&token, TK_KW) && !expect_type(&token, TK_ID)) {
    fprintf(stderr, "expected type name at line %ld\n", token->line);
    exit(1);
  }

  if (expect_str(&token, "int"))
    return KAT_INT;
  if (expect_str(&token, "char"))
    return KAT_CHAR;
  if (expect_str(&token, "str"))
    return KAT_STR;
  if (expect_str(&token, "bool"))
    return KAT_BOOL;

  fprintf(stderr, "unknown data type \"");
  fwrite(token->begin, sizeof(char), token->len, stderr);
  fprintf(stderr, "\" at line %ld\n", token->line);
  exit(1);
}

static type_t *make_type(token_t *type_tok)
{
  type_t *type = calloc(1, sizeof(type_t));
  type->name = type_tok == NULL ? "nil" : tok2cstr(type_tok);
  type->kind = type_tok == NULL ? KAT_NIL : tok2type(type_tok);
  switch (type->kind) {
    case KAT_INT: type->size = 4; break;
    case KAT_CHAR: type->size = 4; break;
    case KAT_STR: type->size = 4; break;
    case KAT_BOOL: type->size = 4; break;
    case KAT_NIL: type->size = 0; break;
  }
  type->next = NULL;
  return type;
}

// make node for new declared variable
static node_t *make_decl_var_node(token_t *var_tok, token_t *type_tok)
{
  node_t *var_node = calloc(1, sizeof(node_t));
  var_node->type = ND_VAR;
  var_node->var = make_var_symbol(var_tok, make_type(type_tok));
  var_node->params = NULL;
  var_node->body = NULL;
  var_node->lhs = NULL;
  var_node->op = NULL;
  var_node->rhs = NULL;
  var_node->cond = NULL;
  var_node->if_stmt = NULL;
  var_node->else_stmt = NULL;
  var_node->while_stmt = NULL;
  var_node->next = NULL;
  return var_node;
}

// make node for variable reference
static node_t *make_ref_var_node(token_t *var_tok)
{
  node_t *var_node = calloc(1, sizeof(node_t));
  var_node->type = ND_VAR;
  var_node->var = find_symbol_by_tok(var_scope, var_tok);
  var_node->params = NULL;
  var_node->body = NULL;
  var_node->lhs = NULL;
  var_node->op = NULL;
  var_node->rhs = NULL;
  var_node->cond = NULL;
  var_node->if_stmt = NULL;
  var_node->else_stmt = NULL;
  var_node->while_stmt = NULL;
  var_node->next = NULL;
  return var_node;
}

static node_t *make_node(ND_TYPE node_type)
{
  node_t *node = calloc(1, sizeof(node_t));
  node->type = node_type;
  return node;
}

/* parsing part */

static token_t *find_right_close_paren(token_t *token);
static node_t *parse_fncall(token_t **token);
static node_t *parse_expr_list(token_t **token);
static node_t *parse_op(token_t **token, int arity);
static node_t *parse_expr(token_t **token, token_t *end_token);
static node_t *parse_expr_stmt(token_t **token);
static node_t *parse_decl_stmt(token_t **token);
static node_t *parse_stmt_block(token_t **token, bool is_func_body);
static node_t *parse_if(token_t **token);
static node_t *parse_while(token_t **token);
static node_t *parse_return(token_t **token);
static node_t *parse_func(token_t **token);

// find the matching right paren
static token_t *find_right_close_paren(token_t *token)
{
  token_t *save = token;
  stack_t *paren_stack = new_stack(32, sizeof(ND_TYPE));
  while (true) {
    if (expect_type(&token, TK_EOF)) {
      fprintf(stderr, "expected \")\" at line %ld\n", save->line);
      exit(1);
    }

    if (expect_str(&token, "(")) {
      push(paren_stack, &(ND_TYPE) {ND_LPAREN});
      token = token->next;
      continue;
    } else if (expect_str(&token, ")")) {
      pop(paren_stack, NULL);
      if (is_empty(paren_stack))
        break;
      token = token->next;
      continue;
    } else {
      token = token->next;
      continue;
    }
  }
  return token;
}

// parse expression list
// expression-list = "(" [expression {"," expression }] ")" ;
static node_t *parse_expr_list(token_t **token)
{
  node_t expr_head = { .next = NULL };
  node_t *curr_expr = &expr_head;

  if (expect_str(token, "(")) {
    token_t *right_close_paren = find_right_close_paren(*token);
    advance(token);
  parse_next_expr:
    curr_expr->next = parse_expr(token, right_close_paren);
    curr_expr = curr_expr->next;
    if (consume(token, ","))
      goto parse_next_expr;
    consume(token, ")");
  }

  return expr_head.next;
}

// parse function call
// function-call = identifier expression-list ;
static node_t *parse_fncall(token_t **token)
{
  if (expect_type(token, TK_ID)) {
    symbol_t *var_symbol = find_symbol_by_tok(var_scope, *token);
    if (var_symbol) {
      fprintf(stderr, "variable \"%s\" cannot be called as a function at line %ld\n", tok2cstr(*token), (*token)->line);
      exit(1);
    }

    symbol_t *func_symbol = find_symbol_by_tok(func_scope, *token);
    if (!func_symbol) {
      fprintf(stderr, "use of undeclared function \"%s\" at line %ld\n", tok2cstr(*token), (*token)->line);
      exit(1);
    }

    advance(token);

    if (!expect_str(token, "(")) {
      fprintf(stderr, "expected \"(\" in call of function %s at line %ld\n", tok2cstr(*token), (*token)->line);
      exit(1);
    }

    node_t *fncall_node = make_node(ND_FNCALL);
    fncall_node->func = func_symbol;

    // TODO: check if the params match (types)
    // // expected param types
    // type_t *params_type = func_symbol->params_type;

    // parse params, which is a expression list "(" expr {"," expr} ")"
    fncall_node->params = parse_expr_list(token);
    return fncall_node;
  } else {
    return NULL;
  }
}

// parse operator
// operator = "+" | "-" | "*" | "/" | "&&" | "||" | ">" | "<" | ">=" | "<=" | "==" | "!=" ;
// currently we hard coding these operators and their properties (unary or binary, precedence, associativity, etc.)
// it's better to use a hashmap store these information
static node_t *parse_op(token_t **token, int arity)
{
  if (consume(token, "+")) {
    switch (arity) {
      case 1: return make_node(ND_POS);
      case 2: return make_node(ND_ADD);
      default:
        fprintf(stderr, "internal error in %s at line %d\n", __FILE__, __LINE__);
        exit(1);
    }
  }
  if (consume(token, "-")) {
    switch (arity) {
      case 1: return make_node(ND_NEG);
      case 2: return make_node(ND_SUB);
      default:
        fprintf(stderr, "internal error in %s at line %d\n", __FILE__, __LINE__);
        exit(1);
    }
  }
  if (consume(token, "*"))  return make_node(ND_MUL);
  if (consume(token, "/"))  return make_node(ND_DIV);
  if (consume(token, "&&")) return make_node(ND_LOGAND);
  if (consume(token, "||")) return make_node(ND_LOGOR);
  if (consume(token, ">"))  return make_node(ND_GT);
  if (consume(token, "<"))  return make_node(ND_LT);
  if (consume(token, ">=")) return make_node(ND_GE);
  if (consume(token, "<=")) return make_node(ND_LE);
  if (consume(token, "==")) return make_node(ND_EQ);
  if (consume(token, "!=")) return make_node(ND_NE);

  return NULL;
}

static int get_precedence(node_t *op_node)
{
  switch (op_node->type) {
    case ND_LOGAND:
    case ND_LOGOR:
      return 1;
    case ND_GT:
    case ND_LT:
    case ND_GE:
    case ND_LE:
    case ND_EQ:
    case ND_NE:
      return 2;
    case ND_ADD:
    case ND_SUB:
      return 3;
    case ND_MUL:
    case ND_DIV:
      return 4;
    case ND_FNCALL:
      return 5;
    case ND_POS:
    case ND_NEG:
      return 6;
    case ND_LPAREN:
    case ND_RPAREN:
      return 7;
    default:
      return -1;
  }
}

// parse expression
// expression = primary {operator primary} ;
static node_t *parse_expr(token_t **token, token_t *end_token)
{
  // operator-precedence parsing
  // expr_stack, op_stack, lookahead
  // - if lookahead is an operator:
  //   * it has an lower or equal (<=) precedence over the top of op_stack,
  //     then we pop operands from expr_stack, and make a node with type ND_EXPR,
  //     and fill the pointer lhs, op, rhs
  //   * it has an higher (>) precedence than the top of op_stack,
  //     then we push it onto op_stack
  // - if lookahead is a primary:
  //   * parse it and return its node, and push the node onto expr_stack

  stack_t *expr_stack = new_stack(32, sizeof(node_t *));
  stack_t *op_stack = new_stack(32, sizeof(node_t *));

  while (!expect_str(token, ",") && !expect_str(token, "{") && !expect_str(token, ";") && (*token) != end_token) {
    if (expect_str(token, "(")) { // left paren
      node_t *lparen_node = make_node(ND_LPAREN);
      push(op_stack, &lparen_node);
      advance(token);
      continue;
    }

    if (expect_str(token, ")")) { // right paren
      node_t *top = NULL;
      gettop(op_stack, &top);
      if (!top) {
        fprintf(stderr, "mismatched parentheses at line %ld\n", (*token)->line);
        exit(1);
      }
      while (top->type != ND_LPAREN) {
        node_t *expr_node = make_node(ND_EXPR);
        pop(expr_stack, &(expr_node->rhs));
        pop(expr_stack, &(expr_node->lhs));
        pop(op_stack, &(expr_node->op));

        // TODO:
        // type checking
        // * if lhs and rhs has consistent types
        // * if types of operands of op match lhs and rhs

        push(expr_stack, &expr_node);
        gettop(op_stack, &top);
      }
      pop(op_stack, NULL);
      advance(token);
      continue;
    }

    if (expect_type(token, TK_PUNCT)) // operator
    {
      // kat only supports binary operator currently
      node_t *op = parse_op(token, 2);
      if (!op) {
        fprintf(stderr, "invalid expression at line %ld\n", (*token)->line);
        exit(1);
      }
      node_t *top = NULL;
      gettop(op_stack, &top);
      while (top && top->type != ND_LPAREN && get_precedence(top) >= get_precedence(op)) {
        node_t *expr_node = make_node(ND_EXPR);
        pop(expr_stack, &(expr_node->rhs));
        pop(expr_stack, &(expr_node->lhs));
        pop(op_stack, &(expr_node->op));

        // TODO:
        // type checking
        // * if lhs and rhs has consistent types
        // * if types of operands of op match lhs and rhs

        push(expr_stack, &expr_node);
        gettop(op_stack, &top);
      }
      push(op_stack, &op);
      continue;
    }

    if (expect_type(token, TK_NUM)) { // number
      node_t *num_node = make_node(ND_NUM);
      // kat only supports integer numeric value currently
      num_node->ival = (*token)->ival;
      push(expr_stack, &num_node);
      advance(token);
      continue;
    }

    if (expect_type(token, TK_ID)) {  // variable or function call
      token_t *next_tok = peek(token);
      if (expect_str(&next_tok, "(")) { // function call
        node_t *fncall_node = parse_fncall(token);
        push(expr_stack, &fncall_node);
      } else {  // variable
        symbol_t *var_symbol = find_symbol_by_tok(var_scope, *token);
        if (!var_symbol) {
          fprintf(stderr, "use of undeclared variable \"%s\" at line %ld\n", tok2cstr(*token), (*token)->line);
          exit(1);
        }
        node_t *var_node = make_ref_var_node(*token);
        push(expr_stack, &var_node);
        advance(token);
      }
      continue;
    }
  }

  // handle the remaining operators
  while (!is_empty(op_stack)) {
    node_t *top = NULL;
    gettop(op_stack, &top);
    if (top->type == ND_LPAREN) {
      fprintf(stderr, "mismatched parentheses at line %ld\n", (*token)->line);
      exit(1);
    }

    node_t *expr_node = make_node(ND_EXPR);
    pop(expr_stack, &(expr_node->rhs));
    pop(expr_stack, &(expr_node->lhs));
    pop(op_stack, &(expr_node->op));
    push(expr_stack, &expr_node);
  }

  // return the top of expr_stack
  node_t *expr_node = NULL;
  gettop(expr_stack, &expr_node);
  return expr_node;
}

// parse expression statement
// kat does not support compound assignment operator currently
// [identifier "="] expression ";" ;
static node_t *parse_expr_stmt(token_t **token)
{
  if (expect_type(token, TK_ID) && expect_next_str(token, "=")) {
    // TODO:
    // * check if the identifier is a left value
    symbol_t *var_symbol = find_symbol_by_tok(var_scope, *token);
    symbol_t *func_symbol = find_symbol_by_tok(func_scope, *token);
    if (!var_symbol) {
      fprintf(stderr, "use of undeclared variable \"%s\" at line %ld\n", tok2cstr(*token), (*token)->line);
      exit(1);
    }
    if (func_symbol) {
      fprintf(stderr, "function \"%s\" cannot be used as a variable at line %ld\n", tok2cstr(var_symbol->token), var_symbol->token->line);
      exit(1);
    }

    node_t *var_node = make_ref_var_node(*token);

    advance(token);

    consume(token, "=");

    node_t *expr_node = parse_expr(token, NULL);

    if (!consume(token, ";")) {
      fprintf(stderr, "an expression statement must end with \";\" at line %ld\n", (*token)->line);
      exit(1);
    }

    node_t *expr_stmt_node = make_node(ND_EXPR_STMT);
    expr_stmt_node->lhs = var_node;
    expr_stmt_node->op = make_node(ND_ASSIGN);
    expr_stmt_node->rhs = expr_node;
    return expr_stmt_node;
  } else {
    node_t *expr_node = parse_expr(token, NULL);

    if (!consume(token, ";")) {
      fprintf(stderr, "an expression statement must end with \";\" at line %ld\n", (*token)->line);
      exit(1);
    }

    node_t *expr_stmt_node = make_node(ND_EXPR_STMT);
    expr_stmt_node->rhs = expr_node;
    return expr_stmt_node;
  }
  return NULL;
}

// parse declaration statement
// "let" identifier ":" type ["=" expression] ";" ;
static node_t *parse_decl_stmt(token_t **token)
{
  // kat does not support multiple declarations statement currently
  // each declaration must be divided one by one
  if (consume(token, "let")) {
    token_t *var_tok = NULL;
    node_t *op_node = NULL;
    node_t *expr_node = NULL;

    if (expect_type(token, TK_ID)) {
      var_tok = *token;
      advance(token);
    } else {
      fprintf(stderr, "expected variable name at line %ld\n", (*token)->line);
      exit(1);
    }

    if (!consume(token, ":")) {
      fprintf(stderr, "expected declaration seperator at line %ld\n", (*token)->line);
      exit(1);
    }

    symbol_t *var_symbol = find_symbol_by_tok(var_scope, var_tok);
    symbol_t *func_symbol = find_symbol_by_tok(func_scope, var_tok);
    if (var_symbol) {
      fprintf(stderr, "redeclaration of \"%s\" at line %ld\n", var_symbol->name, var_tok->line);
      fprintf(stderr, "variable \"%s\" was first defined at line %ld\n", var_symbol->name, var_symbol->token->line);
      exit(1);
    }
    if (func_symbol) {
      fprintf(stderr, "\"%s\" is a function and cannot be declared as a variable at line %ld\n", func_symbol->name, var_tok->line);
      fprintf(stderr, "function \"%s\" was first defined at line %ld\n", func_symbol->name, func_symbol->token->line);
      exit(1);
    }

    node_t *var_node = make_decl_var_node(var_tok, *token);
    add_symbol(var_scope, var_node->var);
    advance(token);

    if (consume(token, "=")) {  // the declared variable is initialized
      op_node = make_node(ND_ASSIGN);
      expr_node = parse_expr(token, NULL);

      // TODO: type checking
      // * see if the type of the expression is consistent with the variable
    }

    if (!consume(token, ";")) { // the declared variable is unintialized
      fprintf(stderr, "a declaration statement should end with \";\" at line %ld\n", (*token)->line);
      exit(1);
    }

    node_t *decl = make_node(ND_DECL_STMT);
    decl->lhs = var_node;
    decl->op = op_node;
    decl->rhs = expr_node;
    return decl;
  } else {
    return NULL;
  }
}

// parse if statement
// kat does not support "elif" currently
// "if" condition block {"elif" block} ["else" block] ;
static node_t *parse_if(token_t **token)
{
  if (consume(token, "if")) {
    node_t *if_cond_node = parse_expr(token, NULL);
    node_t *if_stmt_node = parse_stmt_block(token, false);

    node_t *else_stmt_node = NULL;
    if (consume(token, "else"))
      else_stmt_node = parse_stmt_block(token, false);

    node_t *if_node = make_node(ND_IF);
    if_node->cond = if_cond_node;
    if_node->if_stmt = if_stmt_node;
    if_node->else_stmt = else_stmt_node;
    // TODO: if-else tags
    return if_node;
  } else {
    return NULL;
  }
}

// parse while statement
// "while" condition block ;
static node_t *parse_while(token_t **token)
{
  if (consume(token, "while")) {
    node_t *while_node = make_node(ND_WHILE);
    node_t *while_cond_node = parse_expr(token, NULL);
    node_t *while_stmt_node = parse_stmt_block(token, false);
    while_node->cond = while_cond_node;
    while_node->while_stmt = while_stmt_node;
    // TODO: while tags
    return while_node;
  }
  return NULL;
}

static node_t *parse_return(token_t **token)
{
  if (consume(token, "return")) {
    node_t *return_node = make_node(ND_RETURN);
    return_node->rhs = parse_expr(token, NULL);

    if (!consume(token, ";")) {
      fprintf(stderr, "expected ending \";\" for return statement at line %ld\n", (*token)->line);
      exit(1);
    }

    return return_node;
  }
}

// parse statement block
// block = "{" {statement} "}" ;
static node_t *parse_stmt_block(token_t **token, bool is_func_body)
{
  if (consume(token, "{")) {
    node_t stmt_head = { .next = NULL };
    node_t *curr_stmt = &stmt_head;

    if (!is_func_body)
      enter_scope();

    while (!consume(token, "}")) {
      if (expect_str(token, "let")) {
        curr_stmt->next = parse_decl_stmt(token);
        curr_stmt = curr_stmt->next;
        continue;
      }

      if (expect_str(token, "if")) {
        curr_stmt->next = parse_if(token);
        curr_stmt = curr_stmt->next;
        continue;
      }

      if (expect_str(token, "while")) {
        curr_stmt->next = parse_while(token);
        curr_stmt = curr_stmt->next;
        continue;
      }

      // TODO: parse break/continue/return statement
      if (expect_str(token, "return")) {
        curr_stmt->next = parse_return(token);
        curr_stmt = curr_stmt->next;
        continue;
      }

      curr_stmt->next = parse_expr_stmt(token);
      curr_stmt = curr_stmt->next;
    }

    leave_scope();

    curr_stmt->next = NULL;
    return stmt_head.next;
  } else {
    fprintf(stderr, "a statement block must begin with \"{\" at line %ld\n", (*token)->line);
    exit(1);
  }
}

// function = "func" identifier "(" parameter-list ")" ["=>", type] block ;
// parameter-list = [parameter {"," parameter}] ;
// parameter = identifier ":" type ;
// block = "{" {statement} "}" ;
static node_t *parse_func(token_t **token)
{
  if (consume(token, "func")) {
    // parse function name
    token_t *func_tok = NULL;
    if (expect_type(token, TK_ID)) {
      func_tok = *token;
      advance(token);
    } else {
      fprintf(stderr, "expected function name at line %ld\n", (*token)->line);
      exit(1);
    }

    enter_scope();

    // parse parameter list
    size_t params_num = 0;
    node_t params_head = { .next = NULL };
    node_t *curr_param = &params_head;
    type_t types_head = { .next = NULL };
    type_t *curr_type = &types_head;
    if (consume(token, "(")) {
      // if the next token is ")" then the function has no parameters
      // we consume ")" and do nothing
      // otherwise the function has parameters
      if (!consume(token, ")")) {
        while (true) {
          token_t *var_tok = NULL;
          token_t *type_tok = NULL;
          if (expect_type(token, TK_ID)) {
            var_tok = *token;
            advance(token);
          } else {
            fprintf(stderr, "expected parameter name for function %s at line %ld\n", tok2cstr(func_tok), (*token)->line);
            exit(1);
          }

          if (!consume(token, ":")) {
            fprintf(stderr, "expected name-type seperator \":\" for function %s at line %ld\n", tok2cstr(func_tok), (*token)->line);
            exit(1);
          }

          if (expect_type(token, TK_KW) || expect_type(token, TK_ID)) {
            type_tok = *token;
            advance(token);
          } else {
            fprintf(stderr, "expected type specifier for parameter for function %s at line %ld\n", tok2cstr(func_tok), (*token)->line);
            exit(1);
          }

          symbol_t *var_symbol = find_symbol_by_tok(var_scope, var_tok);
          if (var_symbol) {
            fprintf(stderr, "redeclaration of parameter \"%s\" at line %ld\n", var_symbol->name, var_tok->line);
            exit(1);
          }

          params_num++;

          curr_type->next = make_type(type_tok);
          curr_type = curr_type->next;

          if (!is_valid_type(curr_type->name)) {
            fprintf(stderr, "invalid type for parameter \"%s\" at line %ld", tok2cstr(var_tok), var_tok->line);
            exit(1);
          }

          curr_param->next = make_decl_var_node(var_tok, type_tok);
          curr_param = curr_param->next;
          add_symbol(var_scope, curr_param->var);

          if (consume(token, ")")) {
            break;
          } else if (consume(token, ",")) {
            continue;
          } else {
            fprintf(stderr, "expected right paren \")\" at the end of parameter list for function %s at line %ld\n", tok2cstr(func_tok), (*token)->line);
            exit(1);
          }
        }
      }
    } else {
      fprintf(stderr, "expected left paren \"(\" at line %ld\n", (*token)->len);
      exit(1);
    }

    // parse return type
    type_t *return_type = NULL;
    if (consume(token, "=>")) {
      return_type = make_type(*token);
      advance(token);
    } else {
      return_type = make_type(NULL);
    }

    // make a symbol of function definition and add it to symbol table
    symbol_t *func_symbol = find_symbol_by_tok(func_scope, func_tok);
    if (func_symbol) {
      fprintf(stderr, "redeclaration of function \"%s\" at line %ld\n", func_symbol->name, func_tok->line);
      fprintf(stderr, "function \"%s\" was first defined at line %ld\n", func_symbol->name, func_symbol->token->line);
      exit(1);
    }
    func_symbol = make_fn_symbol(func_tok, return_type, types_head.next, params_num);
    add_symbol(func_scope, func_symbol);

    // parse function body
    // do not enter new scope
    node_t *func_body = parse_stmt_block(token, true);

    node_t *func_node = make_node(ND_FUNC);
    func_node->func = func_symbol;
    func_node->params = params_head.next;
    func_node->body = func_body;
    return func_node;
  } else {
    fprintf(stderr, "a function must begin with \"func\" at line %ld\n", (*token)->line);
    exit(1);
  }
}

node_t *parse(token_t *token_list)
{
  node_t *tree = make_node(ND_PROG);
  token_t *token = token_list;

  func_scope->symbol_table = new_hashmap(128);

  node_t func_head = { .next = NULL };
  node_t *curr_func = &func_head;
  while (token->type != TK_EOF) {
    curr_func->next = parse_func(&token);
    curr_func = curr_func->next;
  }
  tree->body = func_head.next;

  return tree;
}

/* dump ast */

static void dump(int depth, char *fmt, ...) __attribute__((format(printf, 2, 3)));
static void dump_num(node_t *node, int depth);
static void dump_op(node_t *node, int depth);
static void dump_fncall(node_t *node, int depth);
static void dump_var(node_t *node, int depth);
static void dump_expr(node_t *node, int depth);
static void dump_expr_stmt(node_t *node, int depth);
static void dump_if_stmt(node_t *node, int depth);
static void dump_while_stmt(node_t *node, int depth);
static void dump_decl_stmt(node_t *node, int depth);
static void dump_block(node_t *node, int depth);
static void dump_func(node_t *node, int depth);

static void dump(int depth, char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  for (int i = 0; i < depth; i++)
    fprintf(stdout, "    ");
  vfprintf(stdout, fmt, ap);
  va_end(ap);
}

// dump numeric value node
static void dump_num(node_t *node, int depth)
{
  dump(depth, "Num: %ld\n", node->ival);
}

// dump operator node
static void dump_op(node_t *node, int depth)
{
  static char *op_list[] = {
    [ND_ASSIGN] = "Assign: =",
    [ND_ADD] = "Add: +",
    [ND_SUB] = "Subtract: -",
    [ND_MUL] = "Multiply: *",
    [ND_DIV] = "Divide: /",
    [ND_LOGAND] = "LogicalAnd: &&",
    [ND_LOGOR] = "LogicalOr: ||",
    [ND_EQ] = "EqualTo: ==",
    [ND_NE] = "NotEqualTo: !=",
    [ND_LT] = "LessThan: <",
    [ND_LE] = "LessThanOrEqualTo: <=",
    [ND_GT] = "GreaterThan: >",
    [ND_GE] = "GreaterThanOrEqualTo: >="
  };
  dump(depth, "%s\n", op_list[node->type]);
}

// dump function call node
static void dump_fncall(node_t *node, int depth)
{
  dump(depth, "FunCall: %s\n", node->func ? node->func->name : "nil");
  int n = 0;
  for (node_t *param = node->params; param != NULL; param = param->next)
  {
    dump(depth + 1, "param%d:\n", ++n);
    switch (param->type) {
      case ND_EXPR: dump_expr(param, depth + 2); break;
      case ND_NUM: dump_num(param, depth + 2); break;
      case ND_VAR: dump_var(param, depth + 2); break;
      case ND_FNCALL: dump_fncall(param, depth + 2); break;
      default:
        fprintf(stderr, "not implemented yet in %s at line %d\n", __FILE__, __LINE__);
        exit(1);
    }
  }
}

// dump variable
static void dump_var(node_t *node, int depth)
{
  dump(depth, "Variable: %s %s\n",
       node->var ? node->var->name : "nil",
       node->var ? type_list[node->var->type->kind] : "nil");
}

// dump expression
static void dump_expr(node_t *node, int depth)
{
  dump(depth, "Expr:\n");
  switch (node->lhs->type) {
    case ND_EXPR: dump_expr(node->lhs, depth + 1); break;
    case ND_NUM: dump_num(node->lhs, depth + 1); break;
    case ND_VAR: dump_var(node->lhs, depth + 1); break;
    case ND_FNCALL: dump_fncall(node->lhs, depth + 1); break;
    default:
      fprintf(stderr, "not implemented yet in %s at line %d\n", __FILE__, __LINE__);
      exit(1);
  }
  dump_op(node->op, depth + 1);
  switch (node->rhs->type) {
    case ND_EXPR: dump_expr(node->rhs, depth + 1); break;
    case ND_NUM: dump_num(node->rhs, depth + 1); break;
    case ND_VAR: dump_var(node->rhs, depth + 1); break;
    case ND_FNCALL: dump_fncall(node->rhs, depth + 1); break;
    default:
      fprintf(stderr, "not implemented yet in %s at line %d\n", __FILE__, __LINE__);
      exit(1);
  }
}

// dump expression statement
static void dump_expr_stmt(node_t *node, int depth)
{
  dump(depth, "ExprStmt:\n");
  if (node->op != NULL) { // assignment
    dump_var(node->lhs, depth + 1);
    dump_op(node->op, depth + 1);
    switch (node->rhs->type) {
      case ND_EXPR: dump_expr(node->rhs, depth + 1); break;
      case ND_VAR: dump_expr(node->rhs, depth + 1); break;
      case ND_NUM: dump_num(node->rhs, depth + 1); break;
      case ND_FNCALL: dump_fncall(node->rhs, depth + 1); break;
      default:
        fprintf(stderr, "not implemnted in %s at line %d\n", __FILE__, __LINE__);
        exit(1);
    }
  } else {  // just expression
    switch (node->rhs->type) {
      case ND_EXPR: dump_expr(node->rhs, depth + 1); break;
      case ND_VAR: dump_expr(node->rhs, depth + 1); break;
      case ND_NUM: dump_num(node->rhs, depth + 1); break;
      case ND_FNCALL: dump_fncall(node->rhs, depth + 1); break;
      default:
        fprintf(stderr, "not implemnted in %s at line %d\n", __FILE__, __LINE__);
        exit(1);
    }
  }
}

// dump declaration statement
static void dump_decl_stmt(node_t *node, int depth)
{
  dump(depth, "DeclStmt ");
  if (node->op != NULL) { // initialized declaration
    fprintf(stdout, "(initialized):\n");
    dump_var(node->lhs, depth + 1);
    dump_op(node->op, depth + 1);
    switch (node->rhs->type) {
      case ND_EXPR: dump_expr(node->rhs, depth + 1); break;
      case ND_VAR: dump_expr(node->rhs, depth + 1); break;
      case ND_NUM: dump_num(node->rhs, depth + 1); break;
      case ND_FNCALL: dump_fncall(node->rhs, depth + 1); break;
      default:
        fprintf(stderr, "not implemnted in %s at line %d\n", __FILE__, __LINE__);
        exit(1);
    }
  } else {  // uninitialized declaration
    fprintf(stdout, "(uninitialized):\n");
    dump_var(node->lhs, depth + 1);
  }
}

// dump if statement
void dump_if_stmt(node_t *node, int depth)
{
  dump(depth, "IfElseStmt:\n");
  dump(depth + 1, "Condition:\n");
  dump_expr(node->cond, depth + 2);
  dump(depth + 1, "IfBlock:\n");
  if (node->if_stmt)
    dump_block(node->if_stmt, depth + 2);
  dump(depth + 1, "ElseBlock:\n");
  if (node->else_stmt)
    dump_block(node->else_stmt, depth + 2);
}

// dump while statement
static void dump_while_stmt(node_t *node, int depth)
{
  dump(depth, "WhileStmt:\n");
  dump(depth + 1, "Condition:\n");
  dump_expr(node->cond, depth + 2);
  dump(depth + 1, "WhileBlock:\n");
  dump_block(node->while_stmt, depth + 2);
}

// dump return statement
static void dump_return_stmt(node_t *node, int depth)
{
  dump(depth, "ReturnStmt:\n");
  if (node->rhs->type == ND_EXPR)
    dump_expr(node->rhs, depth + 1);
  else if (node->rhs->type == ND_NUM)
    dump_num(node->rhs, depth + 1);
}

// dump block of statements
static void dump_block(node_t *node, int depth)
{
  // loop through all the statements
  while (node != NULL) {
    // dump each statement according to its type
    switch (node->type) {
      case ND_DECL_STMT:
        dump_decl_stmt(node, depth);
        break;
      case ND_EXPR_STMT:
        dump_expr_stmt(node, depth);
        break;
      case ND_IF:
        dump_if_stmt(node, depth);
        break;
      case ND_WHILE:
        dump_while_stmt(node, depth);
        break;
      case ND_RETURN:
        dump_return_stmt(node, depth);
        break;
      default:
        dump(depth, "uh oh, internal dumping error in %s at line %d\n", __FILE__, __LINE__);
        break;
    }
    node = node->next;
  }
}

// dump function
static void dump_func(node_t *node, int depth)
{
  // dump function name, return type, and types of params
  dump(depth, "Function: %s (", node->func->name);
  for (node_t *param = node->params; param != NULL; param = param->next)
    fprintf(stdout, "%s: %s%s", param->var->name, param->var->type->name, param->next == NULL ? "" : ", ");
  fprintf(stdout, ") => ");
  fprintf(stdout, "%s\n", node->func->return_type->name);

  // dump function body (which is actually a block)
  dump(depth, "FuncBody:\n");
  dump_block(node->body, depth + 1);
}

// dump abstract syntax tree
void dump_ast(node_t *tree)
{
  if (tree && tree->type == ND_PROG) {
    int depth = 0;
    dump(depth, "Program:\n");

    // loop through the functions, dump each function
    node_t *func = tree->body;
    while (func != NULL) {
      dump_func(func, depth + 1);
      func = func->next;
    }
  }
}