#ifndef STACK_H
#define STACK_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef struct stack_t
{
  uint8_t *bottom;
  uint8_t *top;
  size_t element_size;
  size_t capacity;
  size_t size;
} stack_t;

stack_t *new_stack(size_t capacity, size_t element_size);
void push(stack_t *stack, void *element);
void pop(stack_t *stack, void *element);
void gettop(stack_t *stack, void *element);
size_t size(stack_t *stack);
bool is_empty(stack_t *stack);
void destroy_stack(stack_t *stack);

#endif
