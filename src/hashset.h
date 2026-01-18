#ifndef HASHSET_H
#define HASHSET_H

#include <stdint.h>

/* Opaque hashset type */
typedef struct HashSet HashSet;

/* Basic operations */
HashSet* hashset_create(void);
void hashset_free(HashSet* set);

/* Add and lookup without symbol ID */
void hashset_add(HashSet* set, const char* name);
int hashset_contains(HashSet* set, const char* name);

/* Add and lookup with symbol ID */
void hashset_add_with_id(HashSet* set, const char* name, uint32_t symbol_id);
uint32_t hashset_get_id(HashSet* set, const char* name);

#endif /* HASHSET_H */
