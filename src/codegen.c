#include "codegen.h"
#include "parse.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

char *source_file_path;
char *output_file_path;
FILE *output_file;

// emit instruction (a loc) to output file
static void emit(char *fmt, ...) __attribute__((format(printf, 1, 2)));
static void emit(char *fmt, ...)
{
  va_list ap;
  va_start(ap, fmt);
  vfprintf(output_file, fmt, ap);
  va_end(ap);
  fprintf(output_file, "\n");
}

static void gen_expr(node_t *node);
static void gen_fncall(node_t *node);
static void gen_decl_stmt(node_t *node);
static void gen_expr_stmt(node_t *node);
static void gen_if_stmt(node_t *node);
static void gen_while_stmt(node_t *node);
static void gen_return_stmt(node_t *node);
static void gen_block(node_t *node);
static void gen_func(node_t *node);
static void gen_data();
static void gen_text(node_t *node);

static void gen_runtime_print();

static void gen_expr(node_t *node)
{
  if (node) {
    if (node->type == ND_NUM)
    {
      emit("  pushl $%ld", node->ival);
      return;
    }

    if (node->type == ND_VAR) {
      emit("  pushl %d(%%ebp)", node->var->offset);
      return;
    }

    if (node->type == ND_FNCALL) {
      gen_fncall(node);
      return;
    }

    gen_expr(node->lhs);
    gen_expr(node->rhs);

    emit("  popl %%edi");
    emit("  popl %%eax");

    switch (node->op->type) {
    case ND_ADD:
      emit("  addl %%edi, %%eax");
      break;
    case ND_SUB:
      emit("  subl %%edi, %%eax\n");
      break;
    case ND_MUL:
      emit("  imul %%edi, %%eax");
      break;
    case ND_DIV:
      emit("  cqo");
      emit("  idiv %%eax");
      break;
    case ND_ASSIGN:
    case ND_LOGAND:
    case ND_LOGOR:
    case ND_EQ:
    case ND_NE:
    case ND_LT:
    case ND_LE:
    case ND_GT:
    case ND_GE:
      fprintf(stdout, "not implemented yet\n");
      exit(1);
    }

    emit("  pushl %%eax");
  }
}

static void gen_fncall(node_t *node)
{
  // TODO: generate code for normal functions
  // runtime print integer function
  if (!strcmp(node->func->name, "print")) {
    gen_expr(node->params);
    emit("  call print");
  }
}

static void gen_decl_stmt(node_t *node)
{
  if (node->op) { // initialized declaration
    gen_expr(node->rhs);  // generate expression on the lhs, the value of expression is stored in %eax
    emit("  popl %%eax");
    emit("  movl %%eax, %d(%%ebp)", node->lhs->var->offset);
  } else {  // unintialized declaration
    // do nothing
  }
}

static void gen_expr_stmt(node_t *node)
{
  gen_expr(node->rhs);  // generate expression on the lhs, the value of expression is stored in %eax
  if (node->lhs) {
    emit("  popl %%eax");
    emit("  movl %%eax, %d(%%ebp)", node->lhs->var->offset);
  }
}

static void gen_if_stmt(node_t *node)
{
}

static void gen_while_stmt(node_t *node)
{
}

// the return value is stored in %eax
static void gen_return_stmt(node_t *node)
{
  gen_expr(node->rhs);
  emit("  popl %%eax");
  emit("  movl %%ebp, %%esp");
  emit("  popl %%ebp");
  emit("  ret");
}

// code generation for block of stmts
// node->type == ND_BLOCK
static void gen_block(node_t *node)
{
  while (node) {
    switch (node->type) {
    case ND_DECL_STMT: gen_decl_stmt(node); break;
    case ND_EXPR_STMT: gen_expr_stmt(node); break;
    case ND_IF: gen_if_stmt(node); break;
    case ND_WHILE: gen_while_stmt(node); break;
    case ND_RETURN: gen_return_stmt(node); break;
    default:
      fprintf(stderr, "not implemented yet\n");
      exit(1);
    }
    node = node->next;
  }
}

// calculate how many bytes needed by the local variables
// also set the offset off the stack for these variables
static size_t calculate_stack_size(node_t *node)
{
  // iterate through all the stmts in function body
  // stack only grows when delaration appears
  size_t stack_size = 0;
  node_t *stmt = node->body;
  while(stmt) {
    if (stmt->type == ND_DECL_STMT) {
      stack_size += stmt->lhs->var->type->size;
      stmt->lhs->var->offset = -stack_size;
    }
    stmt = stmt->next;
  }
  return stack_size;
}

// code generation for function definition
// node->type == ND_FUNC
static void gen_func(node_t *node)
{
  if (!strcmp(node->func->name, "print")) {
    gen_runtime_print();
    return;
  }

  // gnu gas directives for functions
  emit(".type %s, @function", node->func->name);
  emit(".globl %s", node->func->name);
  emit("%s:", node->func->name);

  // save stack frame
  emit("  pushl %%ebp");
  emit("  movl %%esp, %%ebp");

  // reserve space for local variables on the stack
  size_t stack_size = calculate_stack_size(node);
#ifdef DEBUG
  printf("stack size of function \"%s\" is %ld\n", node->func->name, stack_size);
#endif
  if (stack_size > 0)
    emit("  subl $%ld, %%esp", stack_size);
  // emit("  pusha");

  // so I add this message for main
  // every kat program will print this hello message :^)
  if (!strcmp(node->func->name, "main")) {
    emit("  push $msg");
    emit("  call printf");
    emit("  add $4, %%esp");
  }

  // generate function body
  if (node->body)
    gen_block(node->body);

  // restore stack frame
  // emit("  popa");
  emit("  movl %%ebp, %%esp");
  emit("  popl %%ebp");
  emit("  ret");
}

static void gen_data()
{
  emit(".section .data");
  emit("msg:");
  emit("  .asciz \"hello, friends :^)\\n\"");
  emit("number_formatter:");
  emit("  .asciz \"%%d\\n\"");
  emit("");
}

// static void gen_bss()
// {
//   emit("  .section .bss");
// }

// print the integer stored in %eax
static void gen_runtime_print()
{
  emit(".type print, @function");
  emit(".globl print");
  emit("print:");
  emit("  pushl %%ebp");
  emit("  movl %%esp, %%ebp");
  emit("  pushl %%eax");
  emit("  pushl $number_formatter");
  emit("  call printf");
  emit("  add $8, %%esp");
  emit("  movl %%ebp, %%esp");
  emit("  popl %%ebp");
  emit("  ret");
  emit("");
}

static void gen_text(node_t *tree)
{
  node_t *node = tree;
  if (node->type == ND_PROG) {
    emit(".section .text");
    node_t *func = node->body;
    while (func) {
      gen_func(func);
      func = func->next;
    }
  }
}

void codegen(node_t *tree)
{
  gen_data();
  // gen_bss();
  gen_text(tree);
}
