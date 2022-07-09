#ifndef CODEGEN_H
#define CODEGEN_H

#include "parse.h"
#include <stdio.h>

extern char *source_file_path;
extern char *output_file_path;
extern FILE *output_file;

void codegen(node_t *tree);

#endif
