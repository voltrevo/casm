# CASM Control Flow Implementation Analysis

## Executive Summary

The CASM compiler has a well-structured foundation with **2,801 lines of code** across lexer, parser, semantic analyzer, and code generator. The **control flow keywords** (if, while, for, else) are already tokenized, but **parsing and semantic analysis are not yet implemented**. This analysis identifies exactly what needs to be added to support control flow.

---

## 1. PARSER STRUCTURE & CURRENT LIMITATIONS

### 1.1 Current AST Node Types

**File**: `src/ast.h` (lines 63-67)

```c
typedef enum {
    STMT_RETURN,
    STMT_EXPR,
    STMT_VAR_DECL,
} StatementType;
```

**Current Limitations:**
- Only 3 statement types exist
- **NO STMT_IF, STMT_WHILE, STMT_FOR** variants
- No support for else/else-if
- No support for loop body blocks

### 1.2 Parser Error Handling for Control Flow

**File**: `src/parser.c` (lines 508-535)

```c
/* Unsupported control flow - report error and skip it */
if (token.type == TOK_IF || token.type == TOK_WHILE || token.type == TOK_FOR) {
    const char* keyword = (token.type == TOK_IF) ? "if" : 
                          (token.type == TOK_WHILE) ? "while" : "for";
    char msg[100];
    snprintf(msg, sizeof(msg), "Control flow '%s' not yet implemented", keyword);
    parser_error(parser, msg);
    
    /* MUST advance to avoid infinite loop in block parser */
    advance(parser);
    
    /* Skip to closing brace or semicolon - simple recovery */
    int depth = 0;
    while (!check(parser, TOK_EOF)) {
        if (check(parser, TOK_LBRACE)) {
            depth++;
        } else if (check(parser, TOK_RBRACE)) {
            if (depth == 0) {
                /* Don't consume the closing brace - it belongs to parent block */
                break;
            }
            depth--;
        }
        advance(parser);
    }
    
    return NULL;  /* Return NULL to skip this statement */
}
```

**Key Observations:**
- Parser explicitly rejects control flow with error recovery
- Recovery mechanism in place (depth tracking)
- **ERROR BLOCK IS THE CURRENT BOTTLENECK** for control flow

### 1.3 Statement Parsing Flow

**File**: `src/parser.c` (lines 483-593)

The `parse_statement()` function handles:
1. `RETURN` statements
2. **Control flow rejection** (IF/WHILE/FOR)
3. Variable declarations
4. Expression statements

**CURRENT PARSE ORDER:**
```
parse_statement()
  → Check for TOK_RETURN
  → Check for TOK_IF/WHILE/FOR (REJECTED HERE!)
  → Check for variable declaration (type keyword)
  → Default to expression statement
```

---

## 2. AST STRUCTURE REQUIREMENTS

### 2.1 New AST Node Types Needed

All should be added to `src/ast.h` StatementType enum:

**Option A: Minimal Approach**
```c
typedef enum {
    STMT_RETURN,
    STMT_EXPR,
    STMT_VAR_DECL,
    STMT_IF,        // NEW
    STMT_WHILE,     // NEW
    STMT_FOR,       // NEW
} StatementType;
```

**Option B: More Flexible (Separate if/else-if/else)**
```c
typedef enum {
    STMT_RETURN,
    STMT_EXPR,
    STMT_VAR_DECL,
    STMT_IF,        // handles if + optional else
    STMT_WHILE,
    STMT_FOR,
} StatementType;
```

### 2.2 Required Structure Definitions

New structures needed in `src/ast.h`:

```c
/* If statement */
typedef struct {
    ASTExpression* condition;
    ASTBlock* then_body;       // Block for "if" part
    ASTBlock* else_body;       // NULL if no else, otherwise block for else
    SourceLocation location;
} ASTIfStmt;

/* While loop */
typedef struct {
    ASTExpression* condition;
    ASTBlock* body;
    SourceLocation location;
} ASTWhileStmt;

/* For loop */
typedef struct {
    ASTStatement* init;        // NULL or expression/var decl
    ASTExpression* condition;  // NULL = infinite loop
    ASTStatement* update;      // NULL or expression  
    ASTBlock* body;
    SourceLocation location;
} ASTForStmt;
```

### 2.3 Updated ASTStatement Union

```c
struct ASTStatement {
    StatementType type;
    SourceLocation location;
    union {
        ASTReturnStmt return_stmt;
        ASTExprStmt expr_stmt;
        ASTVarDeclStmt var_decl_stmt;
        ASTIfStmt if_stmt;         // NEW
        ASTWhileStmt while_stmt;   // NEW
        ASTForStmt for_stmt;       // NEW
    } as;
};
```

### 2.4 Block Structure Already Supports Nesting

**File**: `src/ast.h` (lines 94-98)

```c
struct ASTBlock {
    ASTStatement* statements;      // Array of statements
    int statement_count;
    SourceLocation location;
};
```

**GOOD NEWS**: Blocks can already contain any statements, including nested if/while/for!

---

## 3. LEXER STATUS

### 3.1 Control Flow Tokens Already Exist

**File**: `src/lexer.h` (lines 24-29)

```c
/* Keywords - Control Flow */
TOK_IF,
TOK_ELSE,
TOK_WHILE,
TOK_FOR,
TOK_RETURN,
```

**Status**: ✅ COMPLETE - All control flow tokens recognized

### 3.2 All Required Operators Available

**File**: `src/lexer.h` (lines 38-53)

- Comparison operators: `==`, `!=`, `<`, `>`, `<=`, `>=`
- Logical operators: `&&`, `||`, `!`
- Assignment: `=`
- Parentheses, braces, semicolons

**Status**: ✅ COMPLETE - All tokens present

---

## 4. SEMANTIC ANALYSIS REQUIREMENTS

### 4.1 Current Implementation

**File**: `src/semantics.c` (lines 183-228)

The `analyze_statement()` function currently handles:
```c
switch (stmt->type) {
    case STMT_RETURN: /* ... */
    case STMT_VAR_DECL: /* ... */
    case STMT_EXPR: /* ... */
}
```

**Current Type Checking:**
- Logical operators (&&, ||) require BOOL operands (lines 110-117)
- Unary NOT (!) requires BOOL operand (line 134)
- Comparison operators (<, >, <=, >=, ==, !=) return BOOL

### 4.2 Requirements for Control Flow Semantic Analysis

**For IF statements:**
- Condition MUST be TYPE_BOOL
- Recursive analyze nested blocks
- Add new scope for each block

**For WHILE statements:**
- Condition MUST be TYPE_BOOL
- Recursive analyze loop body block
- Add new scope for loop body

**For FOR statements:**
- Init expression: already supported by existing STMT_EXPR/STMT_VAR_DECL
- Condition: MUST be TYPE_BOOL
- Update: already supported by existing STMT_EXPR
- Body: recursive analyze block with new scope

**Type System Status** ✅ Nearly complete:
- TYPE_BOOL exists
- type_compatible() handles bool checking
- is_numeric_type() helper available
- All operators properly typed

### 4.3 Expression Type Analysis

**File**: `src/semantics.c` (lines 50-180)

**Existing capabilities:**
- Binary operator type checking (arithmetic, comparison, logical)
- Unary operator validation
- Variable type resolution
- Function call validation

**What's needed:**
- Validate condition expressions are BOOL in control statements

---

## 5. CODE GENERATION PATTERNS

### 5.1 Current Statement Emission

**File**: `src/codegen.c` (lines 118-153)

```c
static void emit_statement(FILE* out, ASTStatement* stmt, int indent) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case STMT_VAR_DECL: /* ... */
        case STMT_EXPR: /* ... */
        case STMT_RETURN: /* ... */
    }
}
```

**Patterns Established:**
- `print_indent()` for proper formatting
- `emit_expression()` for nested expressions
- `emit_block()` for nested statements
- Proper C type mapping

### 5.2 Required Codegen for Control Flow

**For IF statements** (to C):
```c
if (<condition>) {
    <then_body_statements>
}
[else {
    <else_body_statements>
}]
```

**For WHILE statements** (to C):
```c
while (<condition>) {
    <body_statements>
}
```

**For FOR statements** (to C):
```c
for (<init>; <condition>; <update>) {
    <body_statements>
}
```

### 5.3 Implementation Pattern

Add to `emit_statement()` switch statement in `src/codegen.c`:

```c
case STMT_IF: {
    print_indent(out, indent);
    fprintf(out, "if (");
    emit_expression(out, stmt->as.if_stmt.condition);
    fprintf(out, ") {\n");
    emit_block(out, stmt->as.if_stmt.then_body, indent + 1);
    if (stmt->as.if_stmt.else_body) {
        print_indent(out, indent);
        fprintf(out, "} else {\n");
        emit_block(out, stmt->as.if_stmt.else_body, indent + 1);
    }
    print_indent(out, indent);
    fprintf(out, "}\n");
    break;
}

case STMT_WHILE: {
    print_indent(out, indent);
    fprintf(out, "while (");
    emit_expression(out, stmt->as.while_stmt.condition);
    fprintf(out, ") {\n");
    emit_block(out, stmt->as.while_stmt.body, indent + 1);
    print_indent(out, indent);
    fprintf(out, "}\n");
    break;
}

case STMT_FOR: {
    print_indent(out, indent);
    fprintf(out, "for (");
    if (stmt->as.for_stmt.init) {
        emit_statement(out, stmt->as.for_stmt.init, 0);  /* No indent, inline */
    }
    fprintf(out, "; ");
    if (stmt->as.for_stmt.condition) {
        emit_expression(out, stmt->as.for_stmt.condition);
    }
    fprintf(out, "; ");
    if (stmt->as.for_stmt.update) {
        // Emit expression without semicolon/newline
        emit_expression(out, stmt->as.for_stmt.update->as.expr_stmt.expr);
    }
    fprintf(out, ") {\n");
    emit_block(out, stmt->as.for_stmt.body, indent + 1);
    print_indent(out, indent);
    fprintf(out, "}\n");
    break;
}
```

---

## 6. PARSER IMPLEMENTATION STRATEGY

### 6.1 Current Expression Parsing Pipeline

**File**: `src/parser.c` (lines 119-481)

Operator precedence (lowest to highest):
1. Assignment: `=` (parse_assignment)
2. Logical OR: `||` (parse_logical_or)
3. Logical AND: `&&` (parse_logical_and)
4. Equality: `==`, `!=` (parse_equality)
5. Relational: `<`, `>`, `<=`, `>=` (parse_relational)
6. Additive: `+`, `-` (parse_additive)
7. Multiplicative: `*`, `/`, `%` (parse_multiplicative)
8. Unary: `-`, `!` (parse_unary)
9. Primary: literals, variables, function calls (parse_primary)

### 6.2 Parsing Strategy for Control Flow

**IF Statement Parsing:**
```
parse_if_statement():
  consume 'if'
  consume '('
  condition = parse_expression()
  consume ')'
  then_body = parse_block()
  if check(TOK_ELSE):
    consume 'else'
    else_body = parse_block()
  else:
    else_body = NULL
  return ASTIfStmt
```

**WHILE Statement Parsing:**
```
parse_while_statement():
  consume 'while'
  consume '('
  condition = parse_expression()
  consume ')'
  body = parse_block()
  return ASTWhileStmt
```

**FOR Statement Parsing:**
```
parse_for_statement():
  consume 'for'
  consume '('
  init = parse_for_init()  // Can be var_decl or expr
  consume ';'
  condition = parse_expression() or NULL
  consume ';'
  update = parse_expression() or NULL
  consume ')'
  body = parse_block()
  return ASTForStmt
```

### 6.3 Required Parser Helper Functions

New functions to add to `src/parser.c`:

```c
/* Forward declarations at top */
static ASTStatement* parse_if_statement(Parser* parser);
static ASTStatement* parse_while_statement(Parser* parser);
static ASTStatement* parse_for_statement(Parser* parser);
static ASTStatement* parse_for_init(Parser* parser);
```

---

## 7. EXAMPLE PROGRAMS & EXPECTED OUTPUT

### 7.1 IF Statement Example

**Input** (`examples/if_statement.csm`):
```c
i32 main() {
    i32 x = 5;
    if (x > 3) {
        return 1;
    } else {
        return 0;
    }
}
```

**Expected C Output:**
```c
#include <stdint.h>
#include <stdbool.h>

int32_t main(void) {
    int32_t x = 5;
    if ((x > 3)) {
        return 1;
    } else {
        return 0;
    }
}
```

**AST Structure:**
```
Block [
  VarDeclStmt(x: i32, init=5)
  IfStmt(
    condition: BinaryOp(>, x, 3)
    then_body: Block[ReturnStmt(1)]
    else_body: Block[ReturnStmt(0)]
  )
]
```

### 7.2 WHILE Loop Example

**Input** (`examples/while_loop.csm`):
```c
i32 main() {
    i32 i = 0;
    while (i < 10) {
        i = i + 1;
    }
    return i;
}
```

**Expected C Output:**
```c
#include <stdint.h>
#include <stdbool.h>

int32_t main(void) {
    int32_t i = 0;
    while ((i < 10)) {
        i = ((i + 1));
    }
    return i;
}
```

**Semantic Requirements:**
- Condition `(i < 10)` must evaluate to BOOL ✓
- Variable `i` must be declared in scope ✓
- Assignment `i = i + 1` must have compatible types ✓

### 7.3 FOR Loop Example

**Input** (`examples/for_loop.csm`):
```c
i32 main() {
    i32 sum = 0;
    i32 i = 0;
    for (i = 0; i < 10; i = i + 1) {
        sum = sum + i;
    }
    return sum;
}
```

**Expected C Output:**
```c
#include <stdint.h>
#include <stdbool.h>

int32_t main(void) {
    int32_t sum = 0;
    int32_t i = 0;
    for (i = 0; (i < 10); i = ((i + 1))) {
        sum = ((sum + i));
    }
    return sum;
}
```

**Semantic Challenges:**
- FOR init can reference previously declared variable (`i`)
- FOR condition must be BOOL
- FOR update is expression (not statement)
- FOR body has access to all previous scopes

### 7.4 Nested Control Flow Example

```c
i32 main() {
    i32 x = 5;
    if (x > 0) {
        i32 y = 0;
        while (y < x) {
            y = y + 1;
        }
        return y;
    } else {
        return 0;
    }
}
```

**Semantic Challenge**: Variable `y` declared in nested scope must not be accessible outside the if block.

---

## 8. IMPLEMENTATION CHECKLIST

### Phase 1: AST Extensions
- [ ] Add new StatementType enum values (STMT_IF, STMT_WHILE, STMT_FOR)
- [ ] Define ASTIfStmt structure
- [ ] Define ASTWhileStmt structure
- [ ] Define ASTForStmt structure
- [ ] Update ASTStatement union
- [ ] Add constructor/destructor helpers
- [ ] Update ast_statement_free_contents() to handle new types

### Phase 2: Parser Extensions
- [ ] Add parse_if_statement() function
- [ ] Add parse_while_statement() function
- [ ] Add parse_for_statement() function
- [ ] Add parse_for_init() function
- [ ] Update parse_statement() to call new functions
- [ ] Remove the error rejection block for control flow

### Phase 3: Semantic Analysis
- [ ] Update analyze_statement() for STMT_IF/WHILE/FOR
- [ ] Add condition type checking (must be BOOL)
- [ ] Add recursive block analysis for nested control flow
- [ ] Update analyze_block() to handle new scope for loop bodies
- [ ] Test with nested scopes

### Phase 4: Code Generation
- [ ] Update emit_statement() for STMT_IF
- [ ] Update emit_statement() for STMT_WHILE
- [ ] Update emit_statement() for STMT_FOR
- [ ] Handle FOR init/update expression emission
- [ ] Test indentation and formatting

### Phase 5: Testing
- [ ] Parse if_statement.csm successfully
- [ ] Parse while_loop.csm successfully
- [ ] Parse for_loop.csm successfully
- [ ] Generate correct C code for each
- [ ] Semantic validation of conditions
- [ ] Error handling for invalid conditions

---

## 9. CURRENT FILE CHANGES REQUIRED

### src/ast.h
- Add 3 new structures (ASTIfStmt, ASTWhileStmt, ASTForStmt)
- Add 3 new enum values to StatementType
- Update ASTStatement union
- Add forward declarations

### src/ast.c
- Add constructor/destructor helpers for new structures
- Update ast_statement_free_contents() for new types
- Add memory cleanup for nested blocks

### src/parser.c
- Add 4 new parsing functions
- Update parse_statement() to call new functions
- Remove control flow error rejection block

### src/semantics.c
- Update analyze_statement() switch for new types
- Add condition type validation
- Handle nested block analysis

### src/codegen.c
- Update emit_statement() switch for new types
- Add formatting for if/while/for C code

### Examples
- Already have if_statement.csm, while_loop.csm, for_loop.csm ready

---

## 10. COMPLEXITY ASSESSMENT

### Difficulty Levels by Component:

1. **AST Extension** (EASY - 1-2 hours)
   - Straightforward struct additions
   - Well-established patterns in codebase

2. **Parser** (MEDIUM - 3-4 hours)
   - FOR loop parsing most complex (3 parts: init, condition, update)
   - Need to handle optional/NULL expressions
   - Error recovery already in place

3. **Semantic Analysis** (EASY - 1-2 hours)
   - Simple type checking for conditions
   - Scope management already implemented
   - Recursive block analysis already done for functions

4. **Code Generation** (EASY - 1 hour)
   - Straightforward C code output
   - Expression emission already works
   - Block indentation already implemented

**Total Estimated Time: 6-9 hours** for complete implementation

---

## 11. BLOCKED ITEMS & CONSTRAINTS

### Currently Blocked:
1. **ALL control flow parsing** - Parser has explicit rejection
2. **All example programs using control flow** won't parse/run

### Parser Constraints to Respect:
- Maintain recursive descent structure
- Preserve error reporting with locations
- Keep statement/expression separation
- Support nested blocks

### Type System Constraints:
- Conditions MUST be TYPE_BOOL
- Cannot auto-convert numeric types to bool (C-like, but explicit)

### Code Generation Constraints:
- Must produce valid C99 code
- Must maintain proper indentation
- Must handle nested parentheses in expressions

---

## 12. RISK ASSESSMENT

### Low Risk Items:
- AST structure changes (mechanical)
- Code generation (straightforward mapping)
- Semantic condition checking

### Medium Risk Items:
- FOR loop parsing (3-part complexity)
- FOR loop scope/variable visibility
- Nested control flow scoping

### High Risk Items:
- None identified - control flow is straightforward

---

## CONCLUSION

The CASM compiler has excellent foundations for adding control flow:

✅ **Ready to Use:**
- Lexer tokenizes if/while/for/else
- Expression parsing with proper boolean operators
- Block structure supports nesting
- Semantic analysis framework established
- Code generation patterns established

⚠️ **Needs Implementation:**
- AST node types for if/while/for
- Parser functions for if/while/for
- Semantic condition validation
- Codegen statement emission

🚀 **Implementation Path:**
1. Add AST structures (~30 lines)
2. Add parser functions (~200 lines)
3. Add semantic analysis (~50 lines)
4. Add codegen (~100 lines)
5. Test and debug (~100 lines example programs)

The existing error rejection block (lines 508-535 of parser.c) is the key blocker that must be replaced with actual implementation.
