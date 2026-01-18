#define _DEFAULT_SOURCE
#include "module_loader.h"
#include "parser.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <libgen.h>
#include <limits.h>
#include <unistd.h>

ModuleCache* module_cache_create(void) {
    ModuleCache* cache = xmalloc(sizeof(ModuleCache));
    cache->modules = NULL;
    cache->count = 0;
    cache->capacity = 0;
    cache->import_chain = NULL;
    cache->chain_depth = 0;
    return cache;
}

void module_cache_free(ModuleCache* cache) {
    if (!cache) return;
    
    for (int i = 0; i < cache->count; i++) {
        xfree(cache->modules[i].absolute_path);
        xfree(cache->modules[i].source_code);
        xfree(cache->modules[i].module_name);
        if (cache->modules[i].ast) {
            ast_program_free(cache->modules[i].ast);
        }
    }
    xfree(cache->modules);
    
    for (int i = 0; i < cache->chain_depth; i++) {
        xfree(cache->import_chain[i]);
    }
    xfree(cache->import_chain);
    
    xfree(cache);
}

/* Free cache structure without freeing the ASTs (they're being reused) */
static void module_cache_free_keep_asts(ModuleCache* cache) {
    if (!cache) return;
    
    for (int i = 0; i < cache->count; i++) {
        xfree(cache->modules[i].absolute_path);
        xfree(cache->modules[i].source_code);
        xfree(cache->modules[i].module_name);
        /* Don't free the AST - we're returning it */
    }
    xfree(cache->modules);
    
    for (int i = 0; i < cache->chain_depth; i++) {
        xfree(cache->import_chain[i]);
    }
    xfree(cache->import_chain);
    
    xfree(cache);
}

char* load_file(const char* path, char** out_error) {
    FILE* f = fopen(path, "r");
    if (!f) {
        if (out_error) {
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "Cannot open file '%s'", path);
            *out_error = xstrdup(buffer);
        }
        return NULL;
    }
    
    /* Seek to end to get size */
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    if (size < 0) {
        if (out_error) {
            *out_error = xstrdup("Failed to determine file size");
        }
        fclose(f);
        return NULL;
    }
    
    char* source = xmalloc(size + 1);
    size_t read_size = fread(source, 1, size, f);
    fclose(f);
    
    if ((long)read_size != size) {
        if (out_error) {
            *out_error = xstrdup("Failed to read file contents");
        }
        xfree(source);
        return NULL;
    }
    
    source[size] = '\0';
    return source;
}

char* resolve_module_path(const char* relative_to_dir, const char* relative_path, char** out_error) {
    char* result = xmalloc(PATH_MAX + 1);
    
    /* If path is absolute, use it directly */
    if (relative_path[0] == '/') {
        strcpy(result, relative_path);
        return result;
    }
    
    /* Build relative path from the base directory */
    snprintf(result, PATH_MAX + 1, "%s/%s", relative_to_dir, relative_path);
    
    /* Normalize the path (handle ./ and ../ ) */
    char resolved[PATH_MAX + 1];
    if (!realpath(result, resolved)) {
        if (out_error) {
            char buffer[512];
            snprintf(buffer, sizeof(buffer), "Cannot resolve path '%s' relative to '%s'", 
                     relative_path, relative_to_dir);
            *out_error = xstrdup(buffer);
        }
        xfree(result);
        return NULL;
    }
    
    strcpy(result, resolved);
    return result;
}

/* Check if path is in the current import chain (for circular detection) */
static int is_path_in_chain(ModuleCache* cache, const char* path) {
    for (int i = 0; i < cache->chain_depth; i++) {
        if (strcmp(cache->import_chain[i], path) == 0) {
            return 1;
        }
    }
    return 0;
}

/* Add path to import chain */
static void add_to_import_chain(ModuleCache* cache, const char* path) {
    if (cache->chain_depth >= 1024) {
        return;  /* Prevent stack overflow */
    }
    
    cache->import_chain = xrealloc(cache->import_chain, 
                                   (cache->chain_depth + 1) * sizeof(char*));
    cache->import_chain[cache->chain_depth++] = xstrdup(path);
}

/* Remove path from import chain (backtrack) */
static void remove_from_import_chain(ModuleCache* cache) {
    if (cache->chain_depth > 0) {
        cache->chain_depth--;
        xfree(cache->import_chain[cache->chain_depth]);
    }
}

/* Forward declaration for recursive loading */
static LoadedModule* module_cache_load_internal(ModuleCache* cache, 
                                                 const char* relative_to_dir,
                                                 const char* relative_path,
                                                 char** out_error);

LoadedModule* module_cache_load(ModuleCache* cache, 
                                 const char* relative_to_dir,
                                 const char* relative_path,
                                 char** out_error) {
    return module_cache_load_internal(cache, relative_to_dir, relative_path, out_error);
}

static LoadedModule* module_cache_load_internal(ModuleCache* cache, 
                                                 const char* relative_to_dir,
                                                 const char* relative_path,
                                                 char** out_error) {
    char* resolved_path = resolve_module_path(relative_to_dir, relative_path, out_error);
    if (!resolved_path) {
        return NULL;
    }
    
    /* Check for circular import in the current chain */
    if (is_path_in_chain(cache, resolved_path)) {
        if (out_error) {
            char buffer[512];
            snprintf(buffer, sizeof(buffer), 
                     "Circular import detected: '%s'", resolved_path);
            *out_error = xstrdup(buffer);
        }
        xfree(resolved_path);
        return NULL;
    }
    
    /* Check if already fully loaded in cache */
    for (int i = 0; i < cache->count; i++) {
        if (strcmp(cache->modules[i].absolute_path, resolved_path) == 0) {
            xfree(resolved_path);
            return &cache->modules[i];
        }
    }
    
    /* Load the file */
    add_to_import_chain(cache, resolved_path);
    char* source = load_file(resolved_path, out_error);
    
    if (!source) {
        remove_from_import_chain(cache);
        xfree(resolved_path);
        return NULL;
    }
    
    /* Parse the file */
    Parser* parser = parser_create(source);
    ASTProgram* ast = parser_parse(parser);
    
    if (parser->errors->error_count > 0) {
        if (out_error) {
            if (parser->errors->error_count > 0) {
                ParseError* first_error = &parser->errors->errors[0];
                char buffer[512];
                snprintf(buffer, sizeof(buffer), "%s (line %d)", 
                         first_error->message, first_error->location.line);
                *out_error = xstrdup(buffer);
            }
        }
        ast_program_free(ast);
        parser_free(parser);
        remove_from_import_chain(cache);
        xfree(resolved_path);
        xfree(source);
        return NULL;
    }
    
    parser_free(parser);
    
    /* Recursively load all imports from this file */
    char file_dir[PATH_MAX + 1];
    strcpy(file_dir, resolved_path);
    char* dir = dirname(file_dir);
    
    for (int i = 0; i < ast->import_count; i++) {
        LoadedModule* imported = module_cache_load_internal(
            cache, 
            dir,
            ast->imports[i].file_path,
            out_error
        );
        
        if (!imported) {
            ast_program_free(ast);
            remove_from_import_chain(cache);
            xfree(resolved_path);
            xfree(source);
            return NULL;
        }
    }
    
    remove_from_import_chain(cache);
    
    /* Add to cache */
    if (cache->count >= cache->capacity) {
        cache->capacity = cache->capacity == 0 ? 10 : cache->capacity * 2;
        cache->modules = xrealloc(cache->modules, cache->capacity * sizeof(LoadedModule));
    }
    
    LoadedModule* module = &cache->modules[cache->count++];
    module->absolute_path = resolved_path;
    module->source_code = source;
    module->module_name = NULL;  /* Not used in new design */
    module->ast = ast;
    
    return module;
}

ASTProgram* build_complete_ast(const char* main_file, char** out_error) {
    ModuleCache* cache = module_cache_create();
    
    /* Resolve main file to absolute path */
    char cwd[PATH_MAX + 1];
    if (!getcwd(cwd, PATH_MAX)) {
        if (out_error) {
            *out_error = xstrdup("Failed to get current working directory");
        }
        module_cache_free(cache);
        return NULL;
    }
    
    char* abs_main_file = resolve_module_path(cwd, main_file, out_error);
    if (!abs_main_file) {
        module_cache_free(cache);
        return NULL;
    }
    
    /* Get directory of main file */
    char main_dir[PATH_MAX + 1];
    strcpy(main_dir, abs_main_file);
    char* dir = dirname(main_dir);
    
    /* Load main file and all imports recursively */
    LoadedModule* main_module = module_cache_load(cache, dir, abs_main_file, out_error);
    
    if (!main_module) {
        module_cache_free(cache);
        xfree(abs_main_file);
        return NULL;
    }
    
    /* Merge all ASTs into a single program */
    ASTProgram* complete = ast_program_create();
    
    /* Allocate space for all imports from all modules */
    int total_imports = 0;
    for (int i = 0; i < cache->count; i++) {
        if (cache->modules[i].ast) {
            total_imports += cache->modules[i].ast->import_count;
        }
    }
    
    int total_functions = 0;
    for (int i = 0; i < cache->count; i++) {
        if (cache->modules[i].ast) {
            total_functions += cache->modules[i].ast->function_count;
        }
    }
    
    /* Copy imports from the main file only (imports are already processed recursively) */
    if (main_module->ast && main_module->ast->import_count > 0) {
        complete->imports = xmalloc(main_module->ast->import_count * sizeof(ASTImportStatement));
        for (int i = 0; i < main_module->ast->import_count; i++) {
            ASTImportStatement* src_import = &main_module->ast->imports[i];
            ASTImportStatement* dst_import = &complete->imports[complete->import_count++];
            
            /* Copy file path */
            dst_import->file_path = xstrdup(src_import->file_path);
            
            /* Copy imported names array */
            dst_import->name_count = src_import->name_count;
            if (src_import->name_count > 0) {
                dst_import->imported_names = xmalloc(src_import->name_count * sizeof(char*));
                for (int k = 0; k < src_import->name_count; k++) {
                    dst_import->imported_names[k] = xstrdup(src_import->imported_names[k]);
                }
            } else {
                dst_import->imported_names = NULL;
            }
            
            dst_import->location = src_import->location;
        }
    }
    
    /* Copy all functions from all modules */
    if (total_functions > 0) {
        complete->functions = xmalloc(total_functions * sizeof(ASTFunctionDef));
    }
    
    for (int i = 0; i < cache->count; i++) {
        if (cache->modules[i].ast && cache->modules[i].ast->function_count > 0) {
            for (int j = 0; j < cache->modules[i].ast->function_count; j++) {
                ASTFunctionDef* src_func = &cache->modules[i].ast->functions[j];
                ASTFunctionDef* dst_func = &complete->functions[complete->function_count++];
                
                /* Copy function with original name (no mangling) */
                dst_func->name = xstrdup(src_func->name);
                dst_func->return_type = src_func->return_type;
                dst_func->location = src_func->location;
                
                /* Copy parameters */
                dst_func->parameter_count = src_func->parameter_count;
                if (src_func->parameter_count > 0) {
                    dst_func->parameters = xmalloc(src_func->parameter_count * sizeof(ASTParameter));
                    for (int k = 0; k < src_func->parameter_count; k++) {
                        dst_func->parameters[k].name = xstrdup(src_func->parameters[k].name);
                        dst_func->parameters[k].type = src_func->parameters[k].type;
                        dst_func->parameters[k].location = src_func->parameters[k].location;
                    }
                } else {
                    dst_func->parameters = NULL;
                }
                
                /* Copy body */
                dst_func->body = src_func->body;
                /* Note: we're sharing the statements array, not copying it
                 * This is okay as long as we don't modify the original AST */
            }
        }
    }
    
    module_cache_free_keep_asts(cache);
    return complete;
}

/* Free a merged AST (doesn't double-free shared statement bodies) */
void ast_program_free_merged(ASTProgram* program) {
    if (!program) return;
    
    /* Free imports */
    for (int i = 0; i < program->import_count; i++) {
        ast_import_free_contents(&program->imports[i]);
    }
    xfree(program->imports);
    
    /* Free functions but NOT their bodies (those are shared from module ASTs) */
    for (int i = 0; i < program->function_count; i++) {
        ASTFunctionDef* func = &program->functions[i];
        xfree(func->name);
        for (int j = 0; j < func->parameter_count; j++) {
            ast_parameter_free(&func->parameters[j]);
        }
        xfree(func->parameters);
        /* Don't free the body statements - they're shared from module ASTs */
    }
    xfree(program->functions);
    
    xfree(program);
}
