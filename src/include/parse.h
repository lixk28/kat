#ifndef PARSE_H
#define PARSE_H

#include "lex.h"
#include "symbol.h"

typedef enum ND_TYPE
{
  ND_NIL,       // do nothing
  ND_PROG,      // program
  ND_VAR,       // variable
  ND_FUNC,      // function
  ND_EXPR,      // expression
  ND_FNCALL,    // function call
  ND_BLOCK,     // statement block {..}
  ND_DECL_STMT, // declaration statement
  ND_EXPR_STMT, // expression statement
  ND_COND,      // condition
  ND_IF,        // if statement
  ND_WHILE,     // while statement
  ND_RETURN,    // return statement
  ND_NUM,       // number
  ND_LPAREN,    // "(" (used for opp, but will not appear in ast)
  ND_RPAREN,    // ")" (used for opp, but will not appear in ast)
  ND_ASSIGN,    // =
  ND_POS,       // + (unary)
  ND_NEG,       // - (unary)
  ND_ADD,       // + (binary)
  ND_SUB,       // - (binary)
  ND_MUL,       // *
  ND_DIV,       // /
  ND_LOGAND,    // &&
  ND_LOGOR,     // ||
  ND_EQ,        // ==
  ND_NE,        // !=
  ND_LT,        // <
  ND_LE,        // <=
  ND_GT,        // >
  ND_GE,        // >=
} ND_TYPE;

typedef struct node_t
{
  ND_TYPE type;

  // used when node type is ND_VAR or ND_FUNC
  union {
    symbol_t *var;
    symbol_t *func;
  };

  // function call or definition
  // the head of parameter list
  // parameters are organized to linked list
  struct node_t *params;

  // function body or statement block
  // the head of functions or statements
  // functions and statements are organized to linked list
  union {
    struct node_t *body;
    struct node_t *block;
  };

  // lhs, rhs and op
  // used by expression and condition
  // also used by declaration statement and expression statement
  struct node_t *lhs;
  struct node_t *op;
  struct node_t *rhs;

  // condition of if or while statement
  // used when node type is ND_IF or ND_WHILE
  struct node_t *cond;

  // if-else statement
  // used when node type is ND_IF
  struct node_t *if_stmt;
  struct node_t *else_stmt;

  // while statement
  // used when node type is ND_WHILE
  struct node_t *while_stmt;

  // the next node
  // used when node type is ND_FUNC
  // or the node is a statement node
  struct node_t *next;

  // literals
  union {
    int64_t ival;
    long double fval;
  };
  char cval;
  char *sval;
} node_t;

node_t *parse(token_t *token_list);

void dump_ast(node_t *tree);

#endif
