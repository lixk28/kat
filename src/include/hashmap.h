#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>

typedef struct entry_t
{
  void *key;
  void *val;
  struct entry_t *next;
} entry_t;

typedef struct hashmap_t
{
  size_t capcity;
} hashmap_t;

hashmap_t *new_hashmap();

void hashmap_add(hashmap_t *hashmap, const void *key, void *val);

void hashmap_remove(hashmap_t *hashmap, const void *key);

void hashmap_get(hashmap_t *hashmap, const void *key);

void delete_hashmap(hashmap_t *hashmap);

#endif
