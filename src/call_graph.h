#ifndef CALL_GRAPH_H
#define CALL_GRAPH_H

#include <stdint.h>
#include "ast.h"

/* Forward declarations */
typedef struct CallGraph CallGraph;
typedef struct CallGraphNode CallGraphNode;

/* Represents an edge from one function to another (caller -> callee) */
typedef struct {
    uint32_t callee_id;  /* symbol_id of the function being called */
} CallGraphEdge;

/* Represents a node in the call graph */
struct CallGraphNode {
    uint32_t symbol_id;           /* symbol_id of this function */
    char* function_name;          /* Name of the function */
    CallGraphEdge* callees;       /* Array of functions this calls */
    int callee_count;
    int callee_capacity;
    int is_entry_point;           /* 1 if this is main or exported */
};

/* Call graph structure */
struct CallGraph {
    CallGraphNode* nodes;
    int node_count;
    int node_capacity;
    uint32_t entry_point_id;      /* symbol_id of main function */
};

/* Build call graph from AST
 * Analyzes all function bodies to find which functions call which other functions
 * Returns NULL on error */
CallGraph* call_graph_create(ASTProgram* program);

/* Free call graph */
void call_graph_free(CallGraph* graph);

/* Find reachable functions from entry point
 * Returns array of symbol_ids that are reachable from main
 * Sets out_count to number of reachable functions
 * Caller must free the returned array */
uint32_t* call_graph_get_reachable_functions(CallGraph* graph, int* out_count);

/* Find all functions that call a given function
 * Used for reverse lookup during name allocation */
uint32_t* call_graph_get_callers(CallGraph* graph, uint32_t callee_id, int* out_count);

/* Debug: print call graph structure */
void call_graph_print(CallGraph* graph);

#endif /* CALL_GRAPH_H */
