#ifndef MODULE_LOADER_H
#define MODULE_LOADER_H

#include "utils.h"
#include "ast.h"

/* Loaded module information */
typedef struct {
    char* absolute_path;     /* Resolved absolute path */
    char* source_code;       /* File contents */
    char* module_name;       /* Import alias */
    ASTProgram* ast;         /* Parsed AST */
} LoadedModule;

/* Module cache - tracks all loaded modules */
typedef struct ModuleCache {
    LoadedModule* modules;
    int count;
    int capacity;
    char** import_chain;     /* For circular import detection */
    int chain_depth;
} ModuleCache;

/* Public API */
ModuleCache* module_cache_create(void);
void module_cache_free(ModuleCache* cache);

/* Load a module and add to cache
 * relative_to_dir: directory of the file doing the importing
 * relative_path: the import path (e.g., "./math.csm")
 *
 * Returns NULL on error, sets error message
 * Returns pointer to LoadedModule on success
 */
LoadedModule* module_cache_load(ModuleCache* cache, 
                                 const char* relative_to_dir,
                                 const char* relative_path,
                                 char** out_error);

/* Load all imports recursively and build complete AST
 * main_file: path to the main .csm file
 * Returns complete AST with all imports, or NULL on error
 */
ASTProgram* build_complete_ast(const char* main_file, char** out_error);

/* Free a merged AST (doesn't double-free shared statement bodies)
 * This should be used instead of ast_program_free for ASTs returned by build_complete_ast
 */
void ast_program_free_merged(ASTProgram* program);

/* Resolve an absolute path relative to a directory
 * Returns allocated string on success, must be freed
 * Returns NULL on error
 */
char* resolve_module_path(const char* relative_to_dir, const char* relative_path, char** out_error);

/* Load file contents
 * Returns allocated string on success, must be freed
 * Returns NULL on error, sets out_error
 */
char* load_file(const char* path, char** out_error);

#endif /* MODULE_LOADER_H */
