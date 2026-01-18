#ifndef AST_H
#define AST_H

#include "lexer.h"
#include "utils.h"

/* Forward declarations */
typedef struct ASTNode ASTNode;
typedef struct ASTProgram ASTProgram;
typedef struct ASTFunctionDef ASTFunctionDef;
typedef struct ASTVarDecl ASTVarDecl;
typedef struct ASTParameter ASTParameter;
typedef struct ASTBlock ASTBlock;
typedef struct ASTStatement ASTStatement;
typedef struct ASTExpression ASTExpression;
typedef struct ASTReturnStmt ASTReturnStmt;
typedef struct ASTExprStmt ASTExprStmt;
typedef struct ASTVarDeclStmt ASTVarDeclStmt;
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

struct ASTStatement {
    StatementType type;
    SourceLocation location;
    union {
        ASTReturnStmt return_stmt;
        ASTExprStmt expr_stmt;
        ASTVarDeclStmt var_decl_stmt;
    } as;
};

/* Block (sequence of statements) */
struct ASTBlock {
    ASTStatement* statements;
    int statement_count;
    SourceLocation location;
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
};

/* Top-level program */
struct ASTProgram {
    ASTFunctionDef* functions;
    int function_count;
};

/* Constructor/destructor helpers */
ASTProgram* ast_program_create(void);
void ast_program_free(ASTProgram* program);

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

#endif /* AST_H */
