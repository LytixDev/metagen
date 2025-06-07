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
#include "ast.h"
#include "base.h"
#include "lex.h"
#include "type.h"
#include <stdio.h>

char *node_kind_str_map[AST_NODE_TYPE_LEN] = {
    "EXPR_UNARY", "EXPR_BINARY", "EXPR_LITERAL",         "EXPR_CALL",   "STMT_WHILE",
    "STMT_IF",    "STMT_BREAK",  "STMT_CONTINUE",        "STMT_RETURN", "STMT_EXPR",
    "STMT_PRINT", "STMT_BLOCK",  "STMT_ASSIGNMENT",      "AST_FUNC",    "AST_STRUCT",
    "AST_ENUM",   "AST_LIST",    "AST_TYPED_IDENT_LIST", "AST_ROOT",
};

/* Expressions */
AstUnary *make_unary(Arena *a, AstExpr *expr, TokenKind op)
{
    AstUnary *unary = m_arena_alloc(a, sizeof(AstUnary));
    unary->kind = EXPR_UNARY;
    unary->op = op;
    unary->expr = expr;
    return unary;
}

AstBinary *make_binary(Arena *a, AstExpr *left, TokenKind op, AstExpr *right)
{
    AstBinary *binary = m_arena_alloc(a, sizeof(AstBinary));
    binary->kind = EXPR_BINARY;
    binary->op = op;
    binary->left = left;
    binary->right = right;
    return binary;
}

AstLiteral *make_literal(Arena *a, Token token)
{
    AstLiteral *literal = m_arena_alloc(a, sizeof(AstLiteral));
    literal->kind = EXPR_LITERAL;
    literal->literal = token.lexeme;
    if (token.kind == TOKEN_NUM) {
        literal->lit_type = LIT_NUM;
    } else if (token.kind == TOKEN_STR) {
        literal->lit_type = LIT_STR;
    } else if (token.kind == TOKEN_NULL) {
        literal->lit_type = LIT_NULL;
    } else {
        literal->lit_type = LIT_IDENT;
    }
    return literal;
}

AstCall *make_call(Arena *a, bool is_comptime, Str8View identifier, AstList *args)
{
    AstCall *call = m_arena_alloc(a, sizeof(AstCall));
    call->kind = EXPR_CALL;
    call->identifier = identifier;
    call->args = args;
    call->is_comptime = is_comptime;
    call->is_resolved = false;
    return call;
}

/* Statements */
AstWhile *make_while(Arena *a, AstExpr *condition, AstStmt *body, s64 line)
{
    AstWhile *stmt = m_arena_alloc(a, sizeof(AstWhile));
    stmt->kind = STMT_WHILE;
    stmt->line = line;
    stmt->condition = condition;
    stmt->body = body;
    return stmt;
}

AstIf *make_if(Arena *a, AstExpr *condition, AstStmt *then, AstStmt *else_, s64 line)
{
    AstIf *stmt = m_arena_alloc(a, sizeof(AstIf));
    stmt->kind = STMT_IF;
    stmt->line = line;
    stmt->condition = condition;
    stmt->then = then;
    stmt->else_ = else_;
    return stmt;
}

AstSingle *make_single(Arena *a, AstStmtKind single_type, AstNode *node, s64 line)
{
    AstSingle *stmt = m_arena_alloc(a, sizeof(AstSingle));
    stmt->kind = single_type;
    stmt->line = line;
    stmt->node = node;
    return stmt;
}

AstBlock *make_block(Arena *a, TypedIdentList declarations, AstList *stmts, s64 line)
{
    AstBlock *stmt = m_arena_alloc(a, sizeof(AstBlock));
    stmt->kind = STMT_BLOCK;
    stmt->line = line;
    stmt->declarations = declarations;
    stmt->stmts = stmts;
    return stmt;
}

AstAssignment *make_assignment(Arena *a, AstExpr *left, AstExpr *right, s64 line)
{
    AstAssignment *stmt = m_arena_alloc(a, sizeof(AstAssignment));
    stmt->kind = STMT_ASSIGNMENT;
    stmt->line = line;
    stmt->left = left;
    stmt->right = right;
    return stmt;
}

/* Other nodes */
AstFunc *make_func(Arena *a, Str8View name, TypedIdentList params, AstStmt *body,
                   AstTypeInfo return_type)
{
    AstFunc *func = m_arena_alloc(a, sizeof(AstFunc));
    func->kind = AST_FUNC;
    func->name = name;
    func->parameters = params;
    func->ast_return_type = return_type;
    func->body = body;
    return func;
}

AstStruct *make_struct(Arena *a, Str8View name, TypedIdentList members)
{
    AstStruct *struct_decl = m_arena_alloc(a, sizeof(AstStruct));
    struct_decl->kind = AST_STRUCT;
    struct_decl->name = name;
    struct_decl->members = members;
    return struct_decl;
}

AstEnum *make_enum(Arena *a, Str8View name, TypedIdentList values)
{
    AstEnum *enum_decl = m_arena_alloc(a, sizeof(AstEnum));
    enum_decl->kind = AST_ENUM;
    enum_decl->name = name;
    enum_decl->members = values;
    return enum_decl;
}

AstListNode *make_list_node(Arena *a, AstNode *this)
{
    AstListNode *node = m_arena_alloc(a, sizeof(AstListNode));
    node->this = this;
    node->next = NULL;
    return node;
}

AstList *make_list(Arena *a, AstNode *head)
{
    AstList *list = m_arena_alloc(a, sizeof(AstList));
    list->kind = AST_LIST;
    list->head = make_list_node(a, head);
    list->tail = list->head;
    return list;
}

void ast_list_push_back(AstList *list, AstListNode *node)
{
    if (list->head == NULL) {
        list->head = node;
        list->tail = node;
    } else {
        list->tail->next = node;
        list->tail = node;
    }
}

AstTypedIdentList *make_typed_ident_list(Arena *a, TypedIdentList vars)
{
    AstTypedIdentList *node_var_list = m_arena_alloc(a, sizeof(AstTypedIdentList));
    node_var_list->kind = AST_TYPED_IDENT_LIST;
    node_var_list->idents = vars;
    return node_var_list;
}

AstRoot *make_root(Arena *a, AstList vars, AstList funcs, AstList structs, AstList enums,
                   AstList calls)
{
    AstRoot *root = m_arena_alloc(a, sizeof(AstRoot));
    root->kind = AST_ROOT;
    root->main_function = NULL;
    root->vars = vars;
    root->funcs = funcs;
    root->structs = structs;
    root->enums = enums;
    root->comptime_calls = calls;
    return root;
}

/* AST Print */
// TODO:
// AstRoot *ast_from_str(Str8 *dump)
static void print_indent(Str8Builder *sb, u32 indent)
{
    for (u32 i = 0; i < indent; i++) {
        str_builder_append_u8(sb, ' ');
    }
}

static void ast_print_typed_var_list(Str8Builder *sb, TypedIdentList vars)
{
    for (u32 i = 0; i < vars.len; i++) {
        TypedIdent var = vars.vars[i];
        str_builder_append_str8(sb, var.name);
        str_builder_append_u8(sb, ':');
        str_builder_append_u8(sb, ' ');
        for (s32 j = 0; j < var.ast_type_info.pointer_indirection; j++) {
            str_builder_append_u8(sb, '^');
        }
        // TODO: create AstTypeInfo to str func
        str_builder_append_str8(sb, var.ast_type_info.name);
        if (i != vars.len - 1) {
            str_builder_append_str8(sb, STR8_LIT(", "));
        }
    }
}

static void ast_node_to_str(Str8Builder *sb, AstNode *head, u32 indent)
{
    if (head == NULL) {
        return;
    }
    if (indent != 0) {
        str_builder_append_u8(sb, '\n');
    }
    print_indent(sb, indent);
    str_builder_append_u8(sb, '(');
    str_builder_sprintf(sb, "%s ", 1, node_kind_str_map[head->kind]);

    if (AST_IS_EXPR(head)) {
        /* Expressions */
        switch (AS_EXPR(head)->kind) {
        default:
            ASSERT_NOT_REACHED;
        case EXPR_UNARY: {
            AstUnary *unary = AS_UNARY(head);
            char *op_text_repr = token_type_str_map[unary->op];
            str_builder_sprintf(sb, "%s", 1, op_text_repr);
            ast_node_to_str(sb, AS_NODE(unary->expr), indent + 1);
        } break;
        case EXPR_BINARY: {
            AstBinary *binary = AS_BINARY(head);
            char *op_text_repr = token_type_str_map[binary->op];
            str_builder_sprintf(sb, "%s", 1, op_text_repr);
            ast_node_to_str(sb, AS_NODE(binary->left), indent + 1);
            ast_node_to_str(sb, AS_NODE(binary->right), indent + 1);
        } break;
        case EXPR_LITERAL: {
            AstLiteral *lit = AS_LITERAL(head);
            str_builder_append_str8(sb, lit->literal);
        } break;
        case EXPR_CALL: {
            AstCall *call = AS_CALL(head);
            if (call->is_comptime) {
                str_builder_append_u8(sb, '@');
            }
            str_builder_sprintf(sb, "\"%s\"", 1, call->identifier.str);
            if (call->args) {
                ast_node_to_str(sb, (AstNode *)call->args, indent + 1);
            }
        } break;
        }
    } else if (AST_IS_STMT(head)) {
        /* Statements */
        switch (AS_STMT(head)->kind) {
        default:
            ASSERT_NOT_REACHED;
        case STMT_WHILE: {
            AstWhile *stmt = AS_WHILE(head);
            ast_node_to_str(sb, AS_NODE(stmt->condition), indent + 1);
            ast_node_to_str(sb, AS_NODE(stmt->body), indent + 1);
        }; break;
        case STMT_IF: {
            AstIf *stmt = AS_IF(head);
            ast_node_to_str(sb, AS_NODE(stmt->condition), indent + 1);
            ast_node_to_str(sb, AS_NODE(stmt->then), indent + 1);
            if (stmt->else_ != NULL) {
                ast_node_to_str(sb, AS_NODE(stmt->else_), indent + 1);
            }
        }; break;
        case STMT_BREAK:
        case STMT_CONTINUE:
        case STMT_RETURN:
        case STMT_EXPR: {
            AstSingle *stmt = AS_SINGLE(head);
            // NOTE: Can this ever be NULL???
            if (stmt->node != NULL) {
                ast_node_to_str(sb, AS_NODE(stmt->node), indent + 1);
            }
        }; break;
        case STMT_PRINT: {
            AstList *list = AS_LIST(head);
            for (AstListNode *node = list->head; node != NULL; node = node->next) {
                ast_node_to_str(sb, node->this, indent + 1);
            }
        }; break;
        case STMT_BLOCK: {
            AstBlock *stmt = AS_BLOCK(head);
            str_builder_append_str8(sb, STR8_LIT(" vars="));
            ast_print_typed_var_list(sb, stmt->declarations);
            ast_node_to_str(sb, AS_NODE(stmt->stmts), indent + 1);
        }; break;
        case STMT_ASSIGNMENT: {
            AstAssignment *stmt = AS_ASSIGNMENT(head);
            ast_node_to_str(sb, AS_NODE(stmt->left), indent + 1);
            ast_node_to_str(sb, AS_NODE(stmt->right), indent + 1);
        }; break;
        }
    } else {
        /* Declarations and containers */
        switch (head->kind) {
        default:
            ASSERT_NOT_REACHED;
        case AST_ROOT: {
            AstRoot *root = AS_ROOT(head);
            ast_node_to_str(sb, (AstNode *)(&root->vars), indent + 1);
            ast_node_to_str(sb, (AstNode *)(&root->funcs), indent + 1);
            ast_node_to_str(sb, (AstNode *)(&root->structs), indent + 1);
            ast_node_to_str(sb, (AstNode *)(&root->enums), indent + 1);
            ast_node_to_str(sb, (AstNode *)(&root->comptime_calls), indent + 1);
        }; break;
        case AST_FUNC: {
            AstFunc *func_decl = AS_FUNC(head);
            if (func_decl->body == NULL) {
                str_builder_append_str8(sb, STR8_LIT("compiler internal "));
            }
            str_builder_sprintf(sb, "\"%s\"", 1, func_decl->name.str);
            str_builder_append_str8(sb, STR8_LIT(" params="));
            ast_print_typed_var_list(sb, func_decl->parameters);
            if (func_decl->body != NULL) {
                ast_node_to_str(sb, AS_NODE(func_decl->body), indent + 1);
            }
        }; break;
        case AST_STRUCT: {
            AstStruct *struct_decl = AS_STRUCT(head);
            str_builder_sprintf(sb, "\"%s\"", 1, struct_decl->name.str);
            str_builder_append_str8(sb, STR8_LIT(" members="));
            ast_print_typed_var_list(sb, struct_decl->members);
        }; break;
        case AST_ENUM: {
            AstEnum *enum_decl = AS_ENUM(head);
            str_builder_sprintf(sb, "\"%s\"", 1, enum_decl->name.str);
            str_builder_append_str8(sb, STR8_LIT(" members="));
            for (u32 i = 0; i < enum_decl->members.len; i++) {
                TypedIdent var = enum_decl->members.vars[i];
                str_builder_append_str8(sb, var.name);
                if (i != enum_decl->members.len - 1) {
                    str_builder_append_u8(sb, ',');
                    str_builder_append_u8(sb, ' ');
                }
            }
        }; break;
        case AST_LIST: {
            AstList *list = AS_LIST(head);
            for (AstListNode *node = list->head; node != NULL; node = node->next) {
                ast_node_to_str(sb, node->this, indent + 1);
            }
        }; break;
        case AST_TYPED_IDENT_LIST: {
            AstTypedIdentList *node_var_list = AS_TYPED_IDENT_LIST(head);
            ast_print_typed_var_list(sb, node_var_list->idents);
        }; break;
        }
    }

    // str_builder_append_u8(sb, '\n');
    // print_indent_2(sb, indent);
    str_builder_append_u8(sb, ')');
}

void ast_to_str(Str8Builder *sb, AstRoot *root)
{
    ast_node_to_str(sb, (AstNode *)root, 0);
    str_builder_end(sb, true);
}
