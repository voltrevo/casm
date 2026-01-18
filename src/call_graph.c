#include "call_graph.h"
#include "utils.h"
#include <string.h>
#include <stdio.h>

/* Helper: Add a callee to a node's call list (avoid duplicates) */
static void add_callee(CallGraphNode* node, uint32_t callee_id) {
    /* Check if already present */
    for (int i = 0; i < node->callee_count; i++) {
        if (node->callees[i].callee_id == callee_id) {
            return;  /* Already in list */
        }
    }

    /* Add new callee */
    if (node->callee_count >= node->callee_capacity) {
        node->callee_capacity = (node->callee_capacity == 0) ? 10 : node->callee_capacity * 2;
        node->callees = xrealloc(node->callees, node->callee_capacity * sizeof(CallGraphEdge));
    }

    node->callees[node->callee_count].callee_id = callee_id;
    node->callee_count++;
}

/* Helper: Collect all function calls from an expression */
static void collect_function_calls(ASTExpression* expr, char*** out_calls, int* out_count, int* out_capacity) {
    if (!expr) return;

    if (expr->type == EXPR_FUNCTION_CALL) {
        ASTFunctionCall* call = &expr->as.function_call;
        
        /* Check if already in list */
        for (int i = 0; i < *out_count; i++) {
            if (strcmp((*out_calls)[i], call->function_name) == 0) {
                return;  /* Already added */
            }
        }

        /* Add to list */
        if (*out_count >= *out_capacity) {
            *out_capacity = (*out_capacity == 0) ? 10 : *out_capacity * 2;
            *out_calls = xrealloc(*out_calls, *out_capacity * sizeof(char*));
        }

        (*out_calls)[(*out_count)++] = xstrdup(call->function_name);
    }

    /* Recursively check subexpressions */
    switch (expr->type) {
        case EXPR_BINARY_OP:
            collect_function_calls(expr->as.binary_op.left, out_calls, out_count, out_capacity);
            collect_function_calls(expr->as.binary_op.right, out_calls, out_count, out_capacity);
            break;
        case EXPR_UNARY_OP:
            collect_function_calls(expr->as.unary_op.operand, out_calls, out_count, out_capacity);
            break;
        default:
            break;
    }
}

/* Helper: Collect all function calls from a statement */
static void collect_calls_from_statement(ASTStatement* stmt, char*** out_calls, int* out_count, int* out_capacity) {
    if (!stmt) return;

    switch (stmt->type) {
        case STMT_EXPR:
            collect_function_calls(stmt->as.expr_stmt.expr, out_calls, out_count, out_capacity);
            break;
        case STMT_VAR_DECL:
            if (stmt->as.var_decl_stmt.var_decl.initializer) {
                collect_function_calls(stmt->as.var_decl_stmt.var_decl.initializer, out_calls, out_count, out_capacity);
            }
            break;
        case STMT_IF: {
            ASTIfStmt* if_stmt = &stmt->as.if_stmt;
            collect_function_calls(if_stmt->condition, out_calls, out_count, out_capacity);
            for (int i = 0; i < if_stmt->then_body.statement_count; i++) {
                collect_calls_from_statement(&if_stmt->then_body.statements[i], out_calls, out_count, out_capacity);
            }
            for (ASTElseIfClause* clause = if_stmt->else_if_chain; clause; clause = clause->next) {
                collect_function_calls(clause->condition, out_calls, out_count, out_capacity);
                for (int i = 0; i < clause->body.statement_count; i++) {
                    collect_calls_from_statement(&clause->body.statements[i], out_calls, out_count, out_capacity);
                }
            }
            if (if_stmt->else_body) {
                for (int i = 0; i < if_stmt->else_body->statement_count; i++) {
                    collect_calls_from_statement(&if_stmt->else_body->statements[i], out_calls, out_count, out_capacity);
                }
            }
            break;
        }
        case STMT_WHILE: {
            ASTWhileStmt* while_stmt = &stmt->as.while_stmt;
            collect_function_calls(while_stmt->condition, out_calls, out_count, out_capacity);
            for (int i = 0; i < while_stmt->body.statement_count; i++) {
                collect_calls_from_statement(&while_stmt->body.statements[i], out_calls, out_count, out_capacity);
            }
            break;
        }
        case STMT_FOR: {
            ASTForStmt* for_stmt = &stmt->as.for_stmt;
            if (for_stmt->init) {
                collect_calls_from_statement(for_stmt->init, out_calls, out_count, out_capacity);
            }
            if (for_stmt->condition) {
                collect_function_calls(for_stmt->condition, out_calls, out_count, out_capacity);
            }
            if (for_stmt->update) {
                collect_function_calls(for_stmt->update, out_calls, out_count, out_capacity);
            }
            for (int i = 0; i < for_stmt->body.statement_count; i++) {
                collect_calls_from_statement(&for_stmt->body.statements[i], out_calls, out_count, out_capacity);
            }
            break;
        }
        case STMT_BLOCK: {
            ASTBlockStmt* block_stmt = &stmt->as.block_stmt;
            for (int i = 0; i < block_stmt->block.statement_count; i++) {
                collect_calls_from_statement(&block_stmt->block.statements[i], out_calls, out_count, out_capacity);
            }
            break;
        }
        case STMT_RETURN:
            if (stmt->as.return_stmt.value) {
                collect_function_calls(stmt->as.return_stmt.value, out_calls, out_count, out_capacity);
            }
            break;
        case STMT_DBG: {
            ASTDbgStmt* dbg_stmt = &stmt->as.dbg_stmt;
            for (int i = 0; i < dbg_stmt->argument_count; i++) {
                collect_function_calls(&dbg_stmt->arguments[i], out_calls, out_count, out_capacity);
            }
            break;
        }
    }
}

/* Build call graph from AST */
CallGraph* call_graph_create(ASTProgram* program) {
    if (!program) return NULL;

    CallGraph* graph = xmalloc(sizeof(CallGraph));
    graph->nodes = NULL;
    graph->node_count = 0;
    graph->node_capacity = 0;
    graph->entry_point_id = 0;

    /* Step 1: Create nodes for all functions */
    if (program->function_count > 0) {
        graph->nodes = xmalloc(program->function_count * sizeof(CallGraphNode));
        graph->node_capacity = program->function_count;

        for (int i = 0; i < program->function_count; i++) {
            ASTFunctionDef* func = &program->functions[i];
            CallGraphNode* node = &graph->nodes[graph->node_count++];

            node->symbol_id = func->symbol_id;
            node->function_name = xstrdup(func->name);
            node->callees = NULL;
            node->callee_count = 0;
            node->callee_capacity = 0;
            node->is_entry_point = (strcmp(func->name, "main") == 0) ? 1 : 0;

            if (node->is_entry_point) {
                graph->entry_point_id = func->symbol_id;
            }
        }
    }

    /* Step 2: For each function, collect all function calls it makes */
    for (int i = 0; i < program->function_count; i++) {
        ASTFunctionDef* func = &program->functions[i];
        
        /* Find the corresponding node */
        CallGraphNode* caller_node = NULL;
        for (int j = 0; j < graph->node_count; j++) {
            if (graph->nodes[j].symbol_id == func->symbol_id) {
                caller_node = &graph->nodes[j];
                break;
            }
        }

        if (!caller_node) continue;

        /* Collect all function calls from this function */
        char** called_functions = NULL;
        int called_count = 0;
        int called_capacity = 0;

        for (int j = 0; j < func->body.statement_count; j++) {
            collect_calls_from_statement(&func->body.statements[j], &called_functions, &called_count, &called_capacity);
        }

        /* Link each called function to this node */
        for (int j = 0; j < called_count; j++) {
            /* Find ALL functions with this name and link them
             * This is conservative - if there are multiple functions with the same name,
             * we link to all of them (they'll all be marked as reachable) */
            for (int k = 0; k < program->function_count; k++) {
                if (strcmp(program->functions[k].name, called_functions[j]) == 0) {
                    add_callee(caller_node, program->functions[k].symbol_id);
                }
            }
            xfree(called_functions[j]);
        }
        xfree(called_functions);
    }

    return graph;
}

void call_graph_free(CallGraph* graph) {
    if (!graph) return;

    for (int i = 0; i < graph->node_count; i++) {
        xfree(graph->nodes[i].function_name);
        xfree(graph->nodes[i].callees);
    }
    xfree(graph->nodes);
    xfree(graph);
}

uint32_t* call_graph_get_reachable_functions(CallGraph* graph, int* out_count) {
    if (!graph || graph->entry_point_id == 0) {
        *out_count = 0;
        return NULL;
    }

    /* Use BFS to find all reachable functions from entry point */
    uint32_t* visited = xmalloc(graph->node_count * sizeof(uint32_t));
    int visited_count = 0;

    uint32_t* queue = xmalloc(graph->node_count * sizeof(uint32_t));
    int queue_head = 0;
    int queue_tail = 0;

    /* Start with entry point */
    queue[queue_tail++] = graph->entry_point_id;

    while (queue_head < queue_tail) {
        uint32_t current_id = queue[queue_head++];

        /* Check if already visited */
        int already_visited = 0;
        for (int i = 0; i < visited_count; i++) {
            if (visited[i] == current_id) {
                already_visited = 1;
                break;
            }
        }
        if (already_visited) continue;

        /* Add to visited */
        visited[visited_count++] = current_id;

        /* Find the node and add its callees to queue */
        for (int i = 0; i < graph->node_count; i++) {
            if (graph->nodes[i].symbol_id == current_id) {
                for (int j = 0; j < graph->nodes[i].callee_count; j++) {
                    queue[queue_tail++] = graph->nodes[i].callees[j].callee_id;
                }
                break;
            }
        }
    }

    xfree(queue);
    *out_count = visited_count;
    return visited;
}

uint32_t* call_graph_get_callers(CallGraph* graph, uint32_t callee_id, int* out_count) {
    uint32_t* callers = xmalloc(graph->node_count * sizeof(uint32_t));
    int caller_count = 0;

    for (int i = 0; i < graph->node_count; i++) {
        for (int j = 0; j < graph->nodes[i].callee_count; j++) {
            if (graph->nodes[i].callees[j].callee_id == callee_id) {
                /* Check if already in list */
                int already_added = 0;
                for (int k = 0; k < caller_count; k++) {
                    if (callers[k] == graph->nodes[i].symbol_id) {
                        already_added = 1;
                        break;
                    }
                }
                if (!already_added) {
                    callers[caller_count++] = graph->nodes[i].symbol_id;
                }
                break;
            }
        }
    }

    if (caller_count == 0) {
        xfree(callers);
        *out_count = 0;
        return NULL;
    }

    *out_count = caller_count;
    return callers;
}

void call_graph_print(CallGraph* graph) {
    if (!graph) {
        printf("Call graph is NULL\n");
        return;
    }

    printf("=== Call Graph ===\n");
    printf("Entry point ID: %u\n\n", graph->entry_point_id);

    for (int i = 0; i < graph->node_count; i++) {
        CallGraphNode* node = &graph->nodes[i];
        printf("Function: %s (ID: %u)%s\n", node->function_name, node->symbol_id,
               node->is_entry_point ? " [ENTRY POINT]" : "");

        if (node->callee_count > 0) {
            printf("  Calls:\n");
            for (int j = 0; j < node->callee_count; j++) {
                printf("    -> ID %u\n", node->callees[j].callee_id);
            }
        } else {
            printf("  (no calls)\n");
        }
        printf("\n");
    }
}
