#include "lex.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
  const char *path = argv[1];
  FILE *file = fopen(path, "r");
  fseek(file, 0L, SEEK_END);
  size_t len = ftell(file);
  fseek(file, 0L, SEEK_SET);
  char *buf = malloc(sizeof(char) * (len + 1));
  fread(buf, sizeof(char), len, file);
  buf[len] = '\0';
  fclose(file);

#ifdef DEBUG
  fprintf(stdout, "%s\n", buf);
#endif

  token_t *tokens = lex(buf);
  dump_token_list(tokens);

  return 0;
}
