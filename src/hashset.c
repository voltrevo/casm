#include "hashset.h"
#include "utils.h"
#include <string.h>

/* Hash table entry - stores name and optional symbol ID */
typedef struct HashSetEntry {
    char* name;
    uint32_t symbol_id;
    struct HashSetEntry* next;  /* Collision chain */
} HashSetEntry;

/* HashSet structure - 256 buckets for 1-byte hash */
struct HashSet {
    HashSetEntry* buckets[256];  /* Fixed array of 256 buckets */
};

/* ============================================================================
 * HASH FUNCTION
 * ============================================================================
 * Simple 1-byte hash: (hash * 31 + char) & 0xFF
 * This distributes strings across 256 buckets
 */
static uint8_t hashset_hash(const char* str)
{
    uint8_t hash = 0;
    if (str == NULL) {
        return 0;
    }
    while (*str) {
        hash = (hash * 31 + (uint8_t)*str) & 0xFF;
        str++;
    }
    return hash;
}

/* ============================================================================
 * PUBLIC API
 * ============================================================================ */

HashSet* hashset_create(void)
{
    HashSet* set = (HashSet*)xmalloc(sizeof(HashSet));
    /* Initialize all buckets to NULL */
    for (int i = 0; i < 256; i++) {
        set->buckets[i] = NULL;
    }
    return set;
}

void hashset_free(HashSet* set)
{
    if (set == NULL) {
        return;
    }

    /* Free all entries in all buckets */
    for (int i = 0; i < 256; i++) {
        HashSetEntry* entry = set->buckets[i];
        while (entry != NULL) {
            HashSetEntry* next = entry->next;
            xfree(entry->name);
            xfree(entry);
            entry = next;
        }
    }

    xfree(set);
}

void hashset_add(HashSet* set, const char* name)
{
    if (set == NULL || name == NULL) {
        return;
    }

    /* Hash the name to get bucket index */
    uint8_t hash = hashset_hash(name);
    HashSetEntry* bucket = set->buckets[hash];

    /* Check if entry already exists */
    while (bucket != NULL) {
        if (strcmp(bucket->name, name) == 0) {
            /* Entry already exists, don't duplicate */
            return;
        }
        bucket = bucket->next;
    }

    /* Entry doesn't exist, create new one */
    HashSetEntry* new_entry = (HashSetEntry*)xmalloc(sizeof(HashSetEntry));
    new_entry->name = xstrdup(name);
    new_entry->symbol_id = 0;  /* No ID set for this variant */
    new_entry->next = set->buckets[hash];
    set->buckets[hash] = new_entry;
}

int hashset_contains(HashSet* set, const char* name)
{
    if (set == NULL || name == NULL) {
        return 0;
    }

    /* Hash the name to get bucket index */
    uint8_t hash = hashset_hash(name);
    HashSetEntry* entry = set->buckets[hash];

    /* Search the bucket's chain */
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            return 1;  /* Found */
        }
        entry = entry->next;
    }

    return 0;  /* Not found */
}

void hashset_add_with_id(HashSet* set, const char* name, uint32_t symbol_id)
{
    if (set == NULL || name == NULL) {
        return;
    }

    /* Hash the name to get bucket index */
    uint8_t hash = hashset_hash(name);
    HashSetEntry* bucket = set->buckets[hash];

    /* Check if entry already exists */
    while (bucket != NULL) {
        if (strcmp(bucket->name, name) == 0) {
            /* Entry already exists, update its ID */
            bucket->symbol_id = symbol_id;
            return;
        }
        bucket = bucket->next;
    }

    /* Entry doesn't exist, create new one */
    HashSetEntry* new_entry = (HashSetEntry*)xmalloc(sizeof(HashSetEntry));
    new_entry->name = xstrdup(name);
    new_entry->symbol_id = symbol_id;
    new_entry->next = set->buckets[hash];
    set->buckets[hash] = new_entry;
}

uint32_t hashset_get_id(HashSet* set, const char* name)
{
    if (set == NULL || name == NULL) {
        return 0;
    }

    /* Hash the name to get bucket index */
    uint8_t hash = hashset_hash(name);
    HashSetEntry* entry = set->buckets[hash];

    /* Search the bucket's chain */
    while (entry != NULL) {
        if (strcmp(entry->name, name) == 0) {
            return entry->symbol_id;
        }
        entry = entry->next;
    }

    return 0;  /* Not found */
}
