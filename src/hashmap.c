#include "hashmap.h"
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// fnv-1 hash
// https://en.wikipedia.org/wiki/Fowler-Noll-Vo_hash_function
#define FNV_OFFSET_BASIS 0xcbf29ce484222325
#define FNV_PRIME 0x100000001b3
static uint64_t fnv_hash(char *s, size_t len)
{
  uint64_t hash = FNV_OFFSET_BASIS;
  for (unsigned i = 0; i < len; i++) {
    hash *= FNV_PRIME;
    hash ^= (unsigned char)s[i];
  }
  return hash;
}

hashmap_t *new_hashmap(size_t capacity)
{
  hashmap_t *hashmap = calloc(1, sizeof(hashmap_t));
  hashmap->capcity = capacity;
  hashmap->bucket = calloc(capacity, sizeof(entry_t));
  hashmap->size = 0;
  for (unsigned i = 0; i < hashmap->capcity; i++)  {
    hashmap->bucket[i].used = false;
    hashmap->bucket[i].key = NULL;
    hashmap->bucket[i].len = 0;
    hashmap->bucket[i].val = NULL;
  }
  return hashmap;
}

void delete_hashmap(hashmap_t *hashmap)
{
  if (hashmap) {
    if (hashmap->bucket)
      free(hashmap->bucket);
    free(hashmap);
  }
}

static bool match(entry_t *entry, char *key, size_t len)
{
  return entry->used == true  // first to make sure the entry is alive
      && entry->len == len
      && !memcmp(entry->key, key, sizeof(char) * len);
}

void hashmap_add(hashmap_t *hashmap, char *key, size_t len, void *val)
{
  if (hashmap->size == hashmap->capcity) {
    fprintf(stderr, "hashmap is full, cannot add entry\n");
    return;
  }

  // get hash value
  uint64_t hash = fnv_hash(key, len);

  // linear probing
  for (unsigned i = 0; i < hashmap->capcity; i++) {
    entry_t *entry = hashmap->bucket + (hash + i) % hashmap->capcity;

    // the entry is not used
    if (entry->used == false) {
      entry->used = true;
      entry->key = key;
      entry->len = len;
      entry->val = val;
      hashmap->size++;
      break;
    }
  }
}

void hashmap_remove(hashmap_t *hashmap, char *key, size_t len)
{
  if (hashmap->size == 0) {
    fprintf(stderr, "hashmap is empty, cannot remove entry\n");
    return;
  }

  // get hash value
  uint64_t hash = fnv_hash(key, len);

  // linear probing
  for (unsigned i = 0; i < hashmap->capcity; i++) {
    entry_t *entry = hashmap->bucket + (hash + i) % hashmap->capcity;
    if (match(entry, key, len)) {
      entry->used = false;
      entry->key = NULL;
      entry->len = 0;
      entry->val = NULL;
      hashmap->size--;
      return;
    }
  }

  fprintf(stderr, "no entry has key \"%s\", cannot remove\n", key);
  return;
}

// if found, return the entry pointer, otherwise return null pointer
entry_t *hashmap_get(hashmap_t *hashmap, char *key, size_t len)
{
  if (hashmap->size == 0)
    return NULL;

  // get hash value
  uint64_t hash = fnv_hash(key, len);

  // linear probing
  for (unsigned i = 0; i < hashmap->capcity; i++) {
    entry_t *entry = hashmap->bucket + (hash + i) % hashmap->capcity;
    if (match(entry, key, len))
      return entry;
  }
  return NULL;
}

// the following functions pass c-style string as key, which is null-terminated
// use strlen to get the number of valid characters excluding '\0'

void hashmap_add_cstr(hashmap_t *hashmap, char *key, void *val)
{
  hashmap_add(hashmap, key, strlen(key), val);
}

void hashmap_remove_cstr(hashmap_t *hashmap, char *key)
{
  hashmap_remove(hashmap, key, strlen(key));
}

entry_t *hashmap_get_cstr(hashmap_t *hashmap, char *key)
{
  return hashmap_get(hashmap, key, strlen(key));
}

#ifdef DEBUG
void hashmap_test()
{
  hashmap_t *map = new_hashmap(5);

  hashmap_add_cstr(map, "test1", "YES");
  hashmap_add_cstr(map, "test2", "NO");

  entry_t *test1 = hashmap_get_cstr(map, "test1");
  entry_t *test2 = hashmap_get_cstr(map, "test2");

  printf("test1: %s\n", (char *) test1->val);
  printf("test2: %s\n", (char *) test2->val);

  hashmap_remove_cstr(map, "test1");
  printf("%s\n", hashmap_get_cstr(map, "test1") == NULL ? "null" : "not null");
}
#endif