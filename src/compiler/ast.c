/*
 *  Copyright (C) 2023-2024 Nicolai Brand (https://lytix.dev)
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
#include "ast.h"
#include "base/str.h"
#include "lex.h"
#include <stdio.h>

char *node_type_str_map[AST_NODE_TYPE_LEN] = {
    "EXPR_UNARY",        "EXPR_BINARY",       "EXPR_LITERAL",
    "EXPR_CALL",         "STMT_WHILE",        "STMT_IF",
    "STMT_ABRUPT",       "STMT_ABRUPT_BREAK", "STMT_ABRUPT_CONTINUE",
    "STMT_BREAK_RETURN", "STMT_PRINT",        "STMT_EXPR",
    "STMT_BLOCK",        "STMT_ASSIGNMENT",   "AST_FUNC",
    "AST_STRUCT",        "AST_ENUM",          "AST_LIST",
    "AST_NODE_VAR_LIST", "AST_ROOT",
};

/* Expressions */
AstExprUnary *make_unary(Arena *arena, AstExpr *expr, TokenType op)
{
    AstExprUnary *unary = m_arena_alloc(arena, sizeof(AstExprUnary));
    unary->type = EXPR_UNARY;
    unary->op = op;
    unary->expr = expr;
    return unary;
}

AstExprBinary *make_binary(Arena *arena, AstExpr *left, TokenType op, AstExpr *right)
{
    AstExprBinary *binary = m_arena_alloc(arena, sizeof(AstExprBinary));
    binary->type = EXPR_BINARY;
    binary->op = op;
    binary->left = left;
    binary->right = right;
    return binary;
}

AstExprLiteral *make_literal(Arena *arena, Token token)
{
    AstExprLiteral *literal = m_arena_alloc(arena, sizeof(AstExprLiteral));
    literal->type = EXPR_LITERAL;
    literal->literal = token.lexeme;
    if (token.type == TOKEN_NUM) {
        literal->lit_type = LIT_NUM;
    } else if (token.type == TOKEN_STR) {
        literal->lit_type = LIT_STR;
    } else {
        literal->lit_type = LIT_IDENT;
    }
    return literal;
}

AstExprCall *make_call(Arena *arena, Str8View identifier, AstNode *args)
{
    AstExprCall *call = m_arena_alloc(arena, sizeof(AstExprCall));
    call->type = EXPR_CALL;
    call->identifier = identifier;
    call->args = args;
    return call;
}

/* Statements */
AstStmtWhile *make_while(Arena *arena, AstExpr *condition, AstStmt *body)
{
    AstStmtWhile *stmt = m_arena_alloc(arena, sizeof(AstStmtWhile));
    stmt->type = STMT_WHILE;
    stmt->condition = condition;
    stmt->body = body;
    return stmt;
}

AstStmtIf *make_if(Arena *arena, AstExpr *condition, AstStmt *then, AstStmt *else_)
{
    AstStmtIf *stmt = m_arena_alloc(arena, sizeof(AstStmtIf));
    stmt->type = STMT_IF;
    stmt->condition = condition;
    stmt->then = then;
    stmt->else_ = else_;
    return stmt;
}

AstStmtSingle *make_single(Arena *arena, AstStmtType single_type, AstNode *print_list)
{
    AstStmtSingle *stmt = m_arena_alloc(arena, sizeof(AstStmtSingle));
    stmt->type = single_type;
    stmt->node = print_list;
    return stmt;
}

AstStmtBlock *make_block(Arena *arena, TypedVarList declarations, AstList *stmts)
{
    AstStmtBlock *stmt = m_arena_alloc(arena, sizeof(AstStmtBlock));
    stmt->type = STMT_BLOCK;
    stmt->declarations = declarations;
    stmt->stmts = stmts;
    return stmt;
}

AstStmtAssignment *make_assignment(Arena *arena, AstExpr *left, AstExpr *right)
{
    AstStmtAssignment *stmt = m_arena_alloc(arena, sizeof(AstStmtAssignment));
    stmt->type = STMT_ASSIGNMENT;
    stmt->left = left;
    stmt->right = right;
    return stmt;
}

/* Other nodes */
AstFunc *make_function(Arena *arena, Str8View name, TypedVarList parameters, AstStmt *body,
                       AstTypeInfo return_type)
{
    AstFunc *func = m_arena_alloc(arena, sizeof(AstFunc));
    func->type = AST_FUNC;
    func->name = name;
    func->parameters = parameters;
    func->return_type = return_type;
    func->body = body;
    return func;
}

AstStruct *make_struct(Arena *arena, Str8View name, TypedVarList members)
{
    AstStruct *struct_decl = m_arena_alloc(arena, sizeof(AstStruct));
    struct_decl->type = AST_STRUCT;
    struct_decl->name = name;
    struct_decl->members = members;
    return struct_decl;
}

AstEnum *make_enum(Arena *arena, Str8View name, TypedVarList values)
{
    AstEnum *enum_decl = m_arena_alloc(arena, sizeof(AstEnum));
    enum_decl->type = AST_ENUM;
    enum_decl->name = name;
    enum_decl->members = values;
    return enum_decl;
}

AstListNode *make_list_node(Arena *arena, AstNode *this)
{
    AstListNode *node = m_arena_alloc(arena, sizeof(AstListNode));
    node->this = this;
    node->next = NULL;
    return node;
}

AstList *make_list(Arena *arena, AstNode *head)
{
    AstList *list = m_arena_alloc(arena, sizeof(AstList));
    list->type = AST_LIST;
    list->head = make_list_node(arena, head);
    list->tail = list->head;
    return list;
}

void ast_list_push_back(AstList *list, AstListNode *node)
{
    list->tail->next = node;
    list->tail = node;
}

AstNodeVarList *make_node_var_list(Arena *arena, TypedVarList vars)
{
    AstNodeVarList *node_var_list = m_arena_alloc(arena, sizeof(AstNodeVarList));
    node_var_list->type = AST_NODE_VAR_LIST;
    node_var_list->vars = vars;
    return node_var_list;
}

AstRoot *make_root(Arena *arena, AstList declarations, AstList functions, AstList structs,
                   AstList enums)
{
    AstRoot *root = m_arena_alloc(arena, sizeof(AstRoot));
    root->type = AST_ROOT;
    root->declarations = declarations;
    root->functions = functions;
    root->structs = structs;
    root->enums = enums;
    return root;
}


static void print_indent(u32 indent)
{
    for (u32 i = 0; i < indent; i++) {
        putchar(' ');
    }
}

static void ast_print_typed_var_list(TypedVarList vars)
{
    for (u32 i = 0; i < vars.len; i++) {
        TypedVar var = vars.vars[i];
        if (var.ast_type_info.is_array) {
            printf("%.*s: %.*s[%d]", STR8VIEW_PRINT(var.name),
                   STR8VIEW_PRINT(var.ast_type_info.name), var.ast_type_info.elements);
        } else {
            printf("%.*s: %.*s", STR8VIEW_PRINT(var.name), STR8VIEW_PRINT(var.ast_type_info.name));
        }
        if (i != vars.len - 1) {
            printf(", ");
        }
    }
}

static void ast_print_expr(AstExpr *head, u32 indent)
{
    putchar('\n');
    print_indent(indent);
    printf("%s ", node_type_str_map[head->type]);

    switch (head->type) {
    case EXPR_UNARY: {
        AstExprUnary *unary = AS_UNARY(head);
        char *op_text_repr = token_type_str_map[unary->op];
        printf("%s", op_text_repr);
        ast_print_expr(unary->expr, indent + 1);
    } break;
    case EXPR_BINARY: {
        AstExprBinary *binary = AS_BINARY(head);
        char *op_text_repr = token_type_str_map[binary->op];
        putchar('\n');
        print_indent(indent + 1);
        printf("op: %s", op_text_repr);
        ast_print_expr(binary->left, indent + 1);
        ast_print_expr(binary->right, indent + 1);
    } break;
    case EXPR_LITERAL: {
        AstExprLiteral *lit = AS_LITERAL(head);
        printf("%.*s", STR8VIEW_PRINT(lit->literal));
    } break;
    case EXPR_CALL: {
        AstExprCall *call = AS_CALL(head);
        printf("%.*s", STR8VIEW_PRINT(call->identifier));
        if (call->args) {
            ast_print(call->args, indent + 1);
        }
    } break;
    default:
        printf("Ast type not handled ...\n");
    }
}

void ast_print_stmt(AstStmt *head, u32 indent)
{
    if (indent != 0) {
        putchar('\n');
    }
    print_indent(indent);
    printf("%s", node_type_str_map[head->type]);
    switch (head->type) {
    case STMT_WHILE: {
        AstStmtWhile *stmt = AS_WHILE(head);
        ast_print_expr(stmt->condition, indent + 1);
        ast_print_stmt(stmt->body, indent + 1);
    }; break;
    case STMT_IF: {
        AstStmtIf *stmt = AS_IF(head);
        ast_print_expr(stmt->condition, indent + 1);
        ast_print_stmt(stmt->then, indent + 1);
        if (stmt->else_ != NULL) {
            ast_print_stmt(stmt->else_, indent + 1);
        }
    }; break;
    case STMT_ABRUPT_BREAK:
    case STMT_ABRUPT_CONTINUE:
    case STMT_ABRUPT_RETURN:
    case STMT_EXPR:
    case STMT_PRINT: {
        AstStmtSingle *stmt = AS_SINGLE(head);
        if (stmt->node != NULL) {
            ast_print(stmt->node, indent + 1);
        }
    }; break;
    case STMT_BLOCK: {
        AstStmtBlock *stmt = AS_BLOCK(head);
        printf(" vars=");
        ast_print_typed_var_list(stmt->declarations);
        ast_print((AstNode *)stmt->stmts, indent + 1);
    }; break;
    case STMT_ASSIGNMENT: {
        AstStmtAssignment *stmt = AS_ASSIGNMENT(head);
        ast_print_expr(stmt->left, indent + 1);
        ast_print_expr(stmt->right, indent + 1);
    }; break;
    default:
        printf("NOT HANDLED");
    }
}

void ast_print(AstNode *head, u32 indent)
{
    if ((u32)head->type < (u32)EXPR_TYPE_LEN) {
        ast_print_expr((AstExpr *)head, indent);
        return;
    } else if ((u32)head->type < (u32)STMT_TYPE_LEN) {
        ast_print_stmt((AstStmt *)head, indent);
        return;
    }

    if (indent != 0) {
        putchar('\n');
    }
    print_indent(indent);
    printf("%s ", node_type_str_map[head->type]);
    switch (head->type) {
    case AST_ROOT: {
        AstRoot *root = AS_ROOT(head);
        ast_print((AstNode *)(&root->declarations), indent + 1);
        ast_print((AstNode *)(&root->functions), indent + 1);
        ast_print((AstNode *)(&root->structs), indent + 1);
        ast_print((AstNode *)(&root->enums), indent + 1);
    }; break;
    case AST_FUNC: {
        AstFunc *func = AS_FUNC(head);
        printf("name=%.*s", STR8VIEW_PRINT(func->name));
        printf(" parameters=");
        ast_print_typed_var_list(func->parameters);
        ast_print_stmt(func->body, indent + 1);
    }; break;
    case AST_STRUCT: {
        AstStruct *struct_decl = AS_STRUCT(head);
        printf("name=%.*s", STR8VIEW_PRINT(struct_decl->name));
        printf(" members=");
        ast_print_typed_var_list(struct_decl->members);
    }; break;
    case AST_ENUM: {
        AstEnum *enum_decl = AS_ENUM(head);
        printf("name=%.*s", STR8VIEW_PRINT(enum_decl->name));
        printf(" values=");
        for (u32 i = 0; i < enum_decl->members.len; i++) {
            TypedVar var = enum_decl->members.vars[i];
            printf("%.*s", STR8VIEW_PRINT(var.name));
            if (i != enum_decl->members.len - 1) {
                printf(", ");
            }
        }
    }; break;
    case AST_LIST: {
        AstList *list = AS_LIST(head);
        for (AstListNode *node = list->head; node != NULL; node = node->next) {
            ast_print(node->this, indent + 1);
        }
    }; break;
    case AST_NODE_VAR_LIST: {
        AstNodeVarList *node_var_list = AS_NODE_VAR_LIST(head);
        ast_print_typed_var_list(node_var_list->vars);
    }; break;
    default:
        printf("NOT HANDLED");
    }
}
