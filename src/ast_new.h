/*
 *  Copyright (C) 2023-2025 Nicolai Brand (https://lytix.dev)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef AST_NEW_H
#define AST_NEW_H

#include "base.h"
#include "lex.h"

typedef struct symbol_t Symbol; // forward from type.h
typedef struct symbol_table_t SymbolTable; // forward from type.h
typedef struct type_info_t TypeInfo; // forward from type.h


typedef struct {
    Str8 type_name;
    bool is_array;
    s32 num_elements;
    s32 num_pointer_indirection; // 0 means that this is not a pointer
} TypeInfoRaw;

typedef enum {
    LIT_IDENT,
    LIT_STR,
    LIT_NUM,
    LIT_NULL,
} LiteralType;

typedef struct {
    TypeInfoRaw type_raw;
    LiteralType lit_type;
    Str8View literal; // Guranteed to be zero-terminated for STR and IDENT aka Str8
} TypedLiteral;

typedef struct {
    TypedLiteral *typed_lits;
    u32 len;
    u32 cap;
} TypedLiteralList;


typedef enum {
    AST_EXPR_UNARY = 0,
    AST_EXPR_BINARY,
    AST_EXPR_LITERAL,
    AST_EXPR_CALL,
    AST_WHILE,
    AST_IF,
    AST_BLOCK,
    AST_ASSIGNMENT,
    AST_FUNC,
    AST_STRUCT,
    AST_ENUM,

    //AST_ROOT,

    AST_NODE_COUNT,
} AstKind;

typedef struct ast_node_t AstNode;

/* Payload for expressions and statements */
typedef struct {
    AstNode **nodes;
    u32 len;
    u32 cap;
} AstList;

typedef struct {
    TokenKind op; AstNode *expr;
} AstUnaryExpr;

typedef struct {
    TokenKind op; AstNode *left; AstNode *right;
} AstBinaryExpr;

typedef struct {
    Symbol *sym; // @NULLABLE. After type checking, each TOKEN_IDENT is bound to a symbol
    LiteralType lit_type; // TOKEN_NUM, TOKEN_STR or TOKEN_IDENT
    Str8View literal; // Guranteed to be zero-terminated for STR and IDENT aka Str8
} AstLiteralExpr;

typedef struct {
    Str8 identifier;
    AstList args;

    bool is_comptime;
    bool is_resolved;
    // AstNode *resolved_node; // @NULLABLE. Points to the newly created node at comptime.
} AstCallExpr;

typedef struct {
    AstNode *condition;  AstNode *body;
} AstWhile;

typedef struct {
    AstNode *condition;
    AstNode *then;
    AstNode *else_;
} AstIf;

typedef struct {
    AstNode *node; // @NULLABLE
} AstSingle;

typedef struct {
    TypedLiteralList declarations;
    AstList stmts;
    // NOTE: Is this where this should be?
    SymbolTable *symt_local; // @NULLABLE. Set after bind_stmt(), just before typecheck
} AstBlock;

typedef struct {
    AstNode *left; // Identifier literal, array indexing, dereference or struct member access
    AstNode *right;
} AstAssignment;

typedef struct {
    Str8 name;
    TypedLiteralList parameters;
    TypeInfoRaw ast_return_type;
    AstNode *body; // @NULLABLE. If NULL then the function is a compiler
} AstFunc;

typedef struct {
    Str8 name;
    TypedLiteralList members;
} AstStruct;

typedef struct {
    Str8 name;
    TypedLiteralList members;
} AstEnum;

struct ast_node_t {
    AstKind kind;
    Token start;
    Token end;
    // @NULLABLE for statements. NOTE: Set after type-checking.
    TypeInfo *t;
    
    union {
        // Expressions
        AstUnaryExpr unary;
        AstBinaryExpr binary;
        AstLiteralExpr literal;
        AstCallExpr call;
        // Statements
        AstWhile while_;
        AstIf if_;
        AstBlock block;
        AstAssignment assignment;
        // Definitions
        AstFunc func;
        AstStruct struct_;
        AstEnum enum_;
    };
};

typedef struct {
    //AstKind kind;

    AstList global_variables;
    AstList global_functions;
    AstList global_structs;
    AstList global_enums;
    // Pointers to AstCallExpr
    AstList comptime_calls;
} AstRoot;

AstNode *alloc_ast_node(Arena *arena, AstKind kind);

void ast_to_dot(AstNode *node);


#endif /* AST_NEW_H */
