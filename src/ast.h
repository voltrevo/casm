#ifndef AST_H
#define AST_H

#include <stdint.h>
#include "lexer.h"
#include "utils.h"

/* Forward declarations */
typedef struct ASTNode ASTNode;
typedef struct ASTProgram ASTProgram;
typedef struct ASTImportStatement ASTImportStatement;
typedef struct ASTFunctionDef ASTFunctionDef;
typedef struct ASTVarDecl ASTVarDecl;
typedef struct ASTParameter ASTParameter;
typedef struct ASTBlock ASTBlock;
typedef struct ASTStatement ASTStatement;
struct ModuleCache;  /* Forward declare as incomplete type, defined in module_loader.h */
typedef struct ASTExpression ASTExpression;
typedef struct ASTReturnStmt ASTReturnStmt;
typedef struct ASTExprStmt ASTExprStmt;
typedef struct ASTVarDeclStmt ASTVarDeclStmt;
typedef struct ASTIfStmt ASTIfStmt;
typedef struct ASTWhileStmt ASTWhileStmt;
typedef struct ASTForStmt ASTForStmt;
typedef struct ASTBlockStmt ASTBlockStmt;
typedef struct ASTDbgStmt ASTDbgStmt;
typedef struct ASTElseIfClause ASTElseIfClause;
typedef struct ASTBinaryOp ASTBinaryOp;
typedef struct ASTUnaryOp ASTUnaryOp;
typedef struct ASTFunctionCall ASTFunctionCall;
typedef struct ASTLiteral ASTLiteral;
typedef struct ASTVariable ASTVariable;

/* Type representation */
typedef enum {
    TYPE_I8,
    TYPE_I16,
    TYPE_I32,
    TYPE_I64,
    TYPE_U8,
    TYPE_U16,
    TYPE_U32,
    TYPE_U64,
    TYPE_BOOL,
    TYPE_VOID,
} CasmType;

typedef struct {
    CasmType type;
    SourceLocation location;
} TypeNode;

const char* type_to_string(CasmType type);
CasmType token_type_to_casm_type(TokenType tok_type);

/* Parameter for function definitions */
struct ASTParameter {
    char* name;
    TypeNode type;
    SourceLocation location;
};

/* Variable declaration */
struct ASTVarDecl {
    char* name;
    TypeNode type;
    ASTExpression* initializer;  /* NULL if no initializer */
    SourceLocation location;
};

/* Statements */
typedef enum {
    STMT_RETURN,
    STMT_EXPR,
    STMT_VAR_DECL,
    STMT_IF,
    STMT_WHILE,
    STMT_FOR,
    STMT_BLOCK,
    STMT_DBG,
} StatementType;

struct ASTReturnStmt {
    ASTExpression* value;  /* NULL if returning void */
    SourceLocation location;
};

struct ASTExprStmt {
    ASTExpression* expr;
    SourceLocation location;
};

struct ASTVarDeclStmt {
    ASTVarDecl var_decl;
};

/* Block (sequence of statements) - forward declared to allow usage in control flow */
struct ASTBlock {
    ASTStatement* statements;
    int statement_count;
    SourceLocation location;
};

/* Else-if clause (linked list of conditions) */
struct ASTElseIfClause {
    ASTExpression* condition;
    ASTBlock body;
    struct ASTElseIfClause* next;  /* NULL if no more else-if */
};

/* If statement with optional else-if chain and optional else */
struct ASTIfStmt {
    ASTExpression* condition;
    ASTBlock then_body;
    ASTElseIfClause* else_if_chain;  /* NULL if no else-if */
    ASTBlock* else_body;  /* NULL if no else block */
    SourceLocation location;
};

/* While statement */
struct ASTWhileStmt {
    ASTExpression* condition;
    ASTBlock body;
    SourceLocation location;
};

/* For statement: for(init; condition; update) { body } */
struct ASTForStmt {
    ASTStatement* init;  /* NULL if no init (e.g., for(; cond; update)) */
    ASTExpression* condition;
    ASTExpression* update;  /* NULL if no update (e.g., for(;;)) */
    ASTBlock body;
    SourceLocation location;
};

/* Bare block statement */
struct ASTBlockStmt {
    ASTBlock block;
    SourceLocation location;
};

/* Debug statement: dbg(expr1, expr2, ...) */
struct ASTDbgStmt {
    char** arg_names;         /* Source text of each argument (e.g., "x", "x + 1") */
    ASTExpression* arguments; /* Evaluated expressions */
    int argument_count;
    SourceLocation location;  /* Location of dbg() call */
};

struct ASTStatement {
    StatementType type;
    SourceLocation location;
    union {
        ASTReturnStmt return_stmt;
        ASTExprStmt expr_stmt;
        ASTVarDeclStmt var_decl_stmt;
        ASTIfStmt if_stmt;
        ASTWhileStmt while_stmt;
        ASTForStmt for_stmt;
        ASTBlockStmt block_stmt;
        ASTDbgStmt dbg_stmt;
    } as;
};

/* Expressions */
typedef enum {
    EXPR_BINARY_OP,
    EXPR_UNARY_OP,
    EXPR_FUNCTION_CALL,
    EXPR_LITERAL,
    EXPR_VARIABLE,
} ExpressionType;

typedef enum {
    BINOP_ADD,       /* + */
    BINOP_SUB,       /* - */
    BINOP_MUL,       /* * */
    BINOP_DIV,       /* / */
    BINOP_MOD,       /* % */
    BINOP_EQ,        /* == */
    BINOP_NE,        /* != */
    BINOP_LT,        /* < */
    BINOP_GT,        /* > */
    BINOP_LE,        /* <= */
    BINOP_GE,        /* >= */
    BINOP_AND,       /* && */
    BINOP_OR,        /* || */
    BINOP_ASSIGN,    /* = (assignment) */
} BinaryOpType;

typedef enum {
    UNOP_NEG,        /* - (negation) */
    UNOP_NOT,        /* ! (logical not) */
} UnaryOpType;

typedef enum {
    LITERAL_INT,
    LITERAL_BOOL,
} LiteralType;

struct ASTBinaryOp {
    ASTExpression* left;
    ASTExpression* right;
    BinaryOpType op;
    SourceLocation location;
};

struct ASTUnaryOp {
    ASTExpression* operand;
    UnaryOpType op;
    SourceLocation location;
};

struct ASTFunctionCall {
    char* function_name;
    ASTExpression* arguments;
    int argument_count;
    SourceLocation location;
};

struct ASTLiteral {
    LiteralType type;
    union {
        long int_value;
        int bool_value;  /* 0 = false, 1 = true */
    } value;
    SourceLocation location;
};

struct ASTVariable {
    char* name;
    SourceLocation location;
};

struct ASTExpression {
    ExpressionType type;
    SourceLocation location;
    union {
        ASTBinaryOp binary_op;
        ASTUnaryOp unary_op;
        ASTFunctionCall function_call;
        ASTLiteral literal;
        ASTVariable variable;
    } as;
    CasmType resolved_type;  /* filled by semantic analyzer */
};

/* Function definition */
struct ASTFunctionDef {
    char* name;
    TypeNode return_type;
    ASTParameter* parameters;
    int parameter_count;
    ASTBlock body;
    SourceLocation location;
    /* Symbol deduplication fields */
    uint32_t symbol_id;         /* Unique identifier (assigned during merge) */
    char* original_name;        /* Name before deduplication */
    char* module_path;          /* Source file path (e.g., "module_a.csm") */
    char* allocated_name;       /* Final resolved name (NULL if not allocated/dead code) */
};

/* Import statement */
struct ASTImportStatement {
    char** imported_names;     /* Array of function names to import, e.g., ["add", "multiply"] */
    int name_count;            /* Number of imported names */
    char* file_path;           /* e.g., "./math.csm" */
    SourceLocation location;
};

/* Top-level program */
struct ASTProgram {
    ASTImportStatement* imports;
    int import_count;
    ASTFunctionDef* functions;
    int function_count;
    struct ModuleCache* source_cache;  /* For merged programs: keeps source module cache alive */
};

/* Constructor/destructor helpers */
ASTProgram* ast_program_create(void);
void ast_program_free(ASTProgram* program);

ASTImportStatement* ast_import_create(char** names, int name_count, const char* file_path, SourceLocation location);
void ast_import_free_contents(ASTImportStatement* import);
void ast_import_free(ASTImportStatement* import);

ASTFunctionDef* ast_function_create(const char* name, TypeNode return_type, SourceLocation location);
void ast_function_free(ASTFunctionDef* func);

ASTBlock* ast_block_create(void);
void ast_block_free(ASTBlock* block);
void ast_block_add_statement(ASTBlock* block, ASTStatement stmt);

ASTStatement* ast_statement_create(StatementType type, SourceLocation location);
void ast_statement_free(ASTStatement* stmt);

ASTExpression* ast_expression_create(ExpressionType type, SourceLocation location);
void ast_expression_free(ASTExpression* expr);

ASTParameter* ast_parameter_create(const char* name, TypeNode type, SourceLocation location);
void ast_parameter_free(ASTParameter* param);

/* Helper functions for control flow statements */
ASTElseIfClause* ast_else_if_create(ASTExpression* cond, ASTBlock body, SourceLocation location);
void ast_else_if_free(ASTElseIfClause* clause);

#endif /* AST_H */
