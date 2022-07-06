#ifndef HASHMAP_H
#define HASHMAP_H

#include <stddef.h>
#include <stdbool.h>

typedef struct entry_t
{
  bool used;    // if the entry has been used
  char *key;    // string (without null '\0' as its tail)
  size_t len;   // key length (number of characters)
  void *val;    // value
} entry_t;

typedef struct hashmap_t
{
  entry_t *bucket;  // chain of entries
  size_t capcity;   // maximum number of entries
  size_t size;      // the number of entries used
} hashmap_t;

hashmap_t *new_hashmap(size_t capacity);

void delete_hashmap(hashmap_t *hashmap);

void hashmap_add(hashmap_t *hashmap, char *key, size_t len, void *val);
void hashmap_add_cstr(hashmap_t *hashmap, char *key, void *val);

void hashmap_remove(hashmap_t *hashmap, char *key, size_t len);
void hashmap_remove_cstr(hashmap_t *hashmap, char *key);

entry_t *hashmap_get(hashmap_t *hashmap, char *key, size_t len);
entry_t *hashmap_get_cstr(hashmap_t *hashmap, char *key);

#ifdef DEBUG
  void hashmap_test();
#endif

#endif
