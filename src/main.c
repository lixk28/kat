#include "lex.h"
#include "parse.h"
#include "codegen.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
  source_file_path = argv[1];
  FILE *source_file = fopen(source_file_path, "r");
  fseek(source_file, 0L, SEEK_END);
  size_t len = ftell(source_file);
  fseek(source_file, 0L, SEEK_SET);
  char *buf = malloc(sizeof(char) * (len + 1));
  fread(buf, sizeof(char), len, source_file);
  buf[len] = '\0';
  fclose(source_file);

  // fprintf(stdout, "%s\n", buf);

  token_t *tokens = lex(buf);
  // dump_token_list(tokens);

  node_t *ast = parse(tokens);
  // dump_ast(ast);

  if (argc >= 3) {
    output_file_path = malloc(sizeof(char) * (strlen(argv[2]) + 5));
    strcpy(output_file_path, argv[2]);
    strcat(output_file_path, ".s");
    output_file = fopen(output_file_path, "w");

    codegen(ast);

    fclose(output_file);

    execl("/usr/bin/gcc", "gcc", "-m32", output_file_path, "-o", argv[2], (char *) NULL);
  }

  return 0;
}
