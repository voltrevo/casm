#ifndef NAME_ALLOCATOR_H
#define NAME_ALLOCATOR_H

#include <stdint.h>
#include "ast.h"
#include "call_graph.h"

/* Forward declarations */
typedef struct NameAllocator NameAllocator;

/* Create name allocator
 * Analyzes reachability from main and allocates names to functions
 * Functions not reachable from main get allocated_name = NULL (dead code) */
NameAllocator* name_allocator_create(ASTProgram* program);

/* Free name allocator */
void name_allocator_free(NameAllocator* allocator);

/* Apply allocations to the program
 * Sets allocated_name field on each function */
void name_allocator_apply(NameAllocator* allocator, ASTProgram* program);

/* Get the allocated name for a symbol_id
 * Returns NULL if not allocated or not found */
const char* name_allocator_get_name(NameAllocator* allocator, uint32_t symbol_id);

/* Debug: print allocation results */
void name_allocator_print(NameAllocator* allocator);

#endif /* NAME_ALLOCATOR_H */
