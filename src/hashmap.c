#include "hashmap.h"
#include <stdint.h>

static uint32_t hash(const char *p)
{

}

hashmap_t *new_hashmap();

void hashmap_add(hashmap_t *hashmap, const void *key, void *val);

void hashmap_remove(hashmap_t *hashmap, const void *key);

void hashmap_get(hashmap_t *hashmap, const void *key);

void delete_hashmap(hashmap_t *hashmap);