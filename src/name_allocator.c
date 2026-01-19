#include "name_allocator.h"
#include "hashset.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>

/* Allocation record: maps symbol_id to allocated name */
typedef struct {
    uint32_t symbol_id;
    char* allocated_name;
    char* original_name;
    char* module_path;
    int is_reachable;
} AllocationRecord;

/* Name allocator structure */
struct NameAllocator {
    AllocationRecord* allocations;
    int allocation_count;
    int allocation_capacity;
    HashSet* used_names;  /* Track which names we've already allocated */
};

/* Helper: Extract basename from path (e.g., "/path/to/module_a.csm" -> "module_a") */
static char* extract_basename(const char* path) {
    if (!path || strlen(path) == 0) {
        return xstrdup("unknown");
    }

    /* Find the last slash */
    const char* last_slash = strrchr(path, '/');
    const char* start = (last_slash) ? last_slash + 1 : path;

    /* Remove .csm extension if present */
    char* result = xstrdup(start);
    char* dot = strchr(result, '.');
    if (dot) {
        *dot = '\0';
    }

    return result;
}

/* Helper: Try to allocate a name, return 1 if successful, 0 if name already taken */
static int try_allocate_name(NameAllocator* allocator, uint32_t symbol_id, const char* name) {
    if (hashset_contains(allocator->used_names, name)) {
        return 0;  /* Name already used */
    }

    /* Find the allocation record for this symbol_id */
    for (int i = 0; i < allocator->allocation_count; i++) {
        if (allocator->allocations[i].symbol_id == symbol_id) {
            allocator->allocations[i].allocated_name = xstrdup(name);
            hashset_add(allocator->used_names, name);
            return 1;  /* Success */
        }
    }

    return 0;  /* Symbol not found */
}

/* Helper: Check if multiple reachable functions share the same original name but different modules */
static int has_same_name_from_different_module(NameAllocator* allocator, 
                                               uint32_t* reachable_ids, 
                                               int reachable_count,
                                               const AllocationRecord* record) {
    for (int i = 0; i < reachable_count; i++) {
        for (int j = 0; j < allocator->allocation_count; j++) {
            AllocationRecord* other = &allocator->allocations[j];
            if (other->symbol_id == reachable_ids[i] && 
                other->is_reachable &&
                other != record &&
                strcmp(other->original_name, record->original_name) == 0 &&
                strcmp(other->module_path, record->module_path) != 0) {
                return 1;  /* Found another reachable function with same name, different module */
            }
        }
    }
    return 0;
}

/* Allocate names following the priority rules, with smart collision detection */
static void allocate_names(NameAllocator* allocator, ASTProgram* program, uint32_t* reachable_ids, int reachable_count) {
    (void)program;  /* Unused - program context already in allocation records */
    
    /* For each reachable function, allocate a name following the priority */
    for (int i = 0; i < reachable_count; i++) {
        uint32_t symbol_id = reachable_ids[i];

        /* Find the allocation record */
        AllocationRecord* record = NULL;
        for (int j = 0; j < allocator->allocation_count; j++) {
            if (allocator->allocations[j].symbol_id == symbol_id) {
                record = &allocator->allocations[j];
                break;
            }
        }

        if (!record) continue;
        if (record->allocated_name) continue;  /* Already allocated */

        /* Check if this name conflicts with another reachable function from a different module */
        int has_conflict = has_same_name_from_different_module(allocator, reachable_ids, reachable_count, record);
        
        if (has_conflict) {
            /* Force module-based mangling: basename_originalname */
            char* basename = extract_basename(record->module_path);
            char combined[512];
            snprintf(combined, sizeof(combined), "%s_%s", basename, record->original_name);
            
            if (try_allocate_name(allocator, symbol_id, combined)) {
                xfree(basename);
                continue;
            }

            /* Fallback: Try basename_originalname_N for N >= 2 */
            int counter = 2;
            while (counter <= 100) {  /* Safety limit */
                snprintf(combined, sizeof(combined), "%s_%s_%d", basename, record->original_name, counter);
                if (try_allocate_name(allocator, symbol_id, combined)) {
                    break;
                }
                counter++;
            }
            xfree(basename);
        } else {
            /* No conflict - use standard priority */
            
            /* Priority 1: Try original name */
            if (try_allocate_name(allocator, symbol_id, record->original_name)) {
                continue;
            }

            /* Priority 2: Try basename_originalname */
            char* basename = extract_basename(record->module_path);
            char combined[512];
            snprintf(combined, sizeof(combined), "%s_%s", basename, record->original_name);
            
            if (try_allocate_name(allocator, symbol_id, combined)) {
                xfree(basename);
                continue;
            }

            /* Priority 3: Try basename_originalname_N for N >= 2 */
            int counter = 2;
            while (counter <= 100) {  /* Safety limit */
                snprintf(combined, sizeof(combined), "%s_%s_%d", basename, record->original_name, counter);
                if (try_allocate_name(allocator, symbol_id, combined)) {
                    break;
                }
                counter++;
            }

            xfree(basename);
        }
    }
}

/* Create name allocator */
NameAllocator* name_allocator_create(ASTProgram* program) {
    if (!program) return NULL;

    NameAllocator* allocator = xmalloc(sizeof(NameAllocator));
    allocator->allocations = NULL;
    allocator->allocation_count = 0;
    allocator->allocation_capacity = 0;
    allocator->used_names = hashset_create();

    /* Step 1: Build call graph to determine reachability */
    CallGraph* graph = call_graph_create(program);
    if (!graph) {
        allocator->used_names = hashset_create();  /* Empty allocator */
        return allocator;
    }

    /* Step 2: Get reachable functions from main */
    int reachable_count = 0;
    uint32_t* reachable_ids = call_graph_get_reachable_functions(graph, &reachable_count);

    /* Step 3: Create allocation records for ALL functions */
    if (program->function_count > 0) {
        allocator->allocations = xmalloc(program->function_count * sizeof(AllocationRecord));
        allocator->allocation_capacity = program->function_count;

        for (int i = 0; i < program->function_count; i++) {
            ASTFunctionDef* func = &program->functions[i];
            AllocationRecord* record = &allocator->allocations[allocator->allocation_count++];

            record->symbol_id = func->symbol_id;
            record->original_name = xstrdup(func->original_name ? func->original_name : func->name);
            record->module_path = xstrdup(func->module_path ? func->module_path : "");
            record->allocated_name = NULL;  /* Not allocated yet */

            /* Check if reachable */
            record->is_reachable = 0;
            if (reachable_ids) {
                for (int j = 0; j < reachable_count; j++) {
                    if (reachable_ids[j] == func->symbol_id) {
                        record->is_reachable = 1;
                        break;
                    }
                }
            }
        }
    }

    /* Step 4: Allocate names to reachable functions */
    if (reachable_ids) {
        allocate_names(allocator, program, reachable_ids, reachable_count);
        xfree(reachable_ids);
    }

    call_graph_free(graph);
    return allocator;
}

/* Free name allocator */
void name_allocator_free(NameAllocator* allocator) {
    if (!allocator) return;

    for (int i = 0; i < allocator->allocation_count; i++) {
        xfree(allocator->allocations[i].allocated_name);
        xfree(allocator->allocations[i].original_name);
        xfree(allocator->allocations[i].module_path);
    }
    xfree(allocator->allocations);
    hashset_free(allocator->used_names);
    xfree(allocator);
}

/* Apply allocations to the program */
void name_allocator_apply(NameAllocator* allocator, ASTProgram* program) {
    if (!allocator || !program) return;

    for (int i = 0; i < program->function_count; i++) {
        ASTFunctionDef* func = &program->functions[i];

        /* Find the allocation for this function */
        for (int j = 0; j < allocator->allocation_count; j++) {
            if (allocator->allocations[j].symbol_id == func->symbol_id) {
                func->allocated_name = allocator->allocations[j].allocated_name 
                    ? xstrdup(allocator->allocations[j].allocated_name) 
                    : NULL;
                break;
            }
        }
    }
}

/* Get allocated name for a symbol_id */
const char* name_allocator_get_name(NameAllocator* allocator, uint32_t symbol_id) {
    if (!allocator) return NULL;

    for (int i = 0; i < allocator->allocation_count; i++) {
        if (allocator->allocations[i].symbol_id == symbol_id) {
            return allocator->allocations[i].allocated_name;
        }
    }

    return NULL;
}
