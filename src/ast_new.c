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

#include "base.h"
#include <sys/types.h>
#include "ast_new.h"


char *ast_node_kind_str_map[AST_NODE_COUNT] = {
    "AST_EXPR_UNARY", "AST_EXPR_BINARY", "AST_EXPR_LITERAL", "AST_EXPR_CALL", "AST_WHILE", "AST_IF",
    "AST_BLOCK", "AST_ASSIGNMENT", "AST_FUNC", "AST_STRUCT", "AST_ENUM", "AST_VAR", "AST_ROOT",
};

#define FOR_EACH(type, var, list) \
    for (size_t _i = 0; _i < (list).size; _i++) \
        for (type var = *(type*)arraylist_get(&(list), _i); var; var = NULL)

// NOTE: Right now we allocate based on the max possible size an ast node can take.
//       A different approach that would use less memory is to allocate based on the memory 
//       the node actually needs.
AstNode *alloc_ast_node(Arena *arena, AstKind kind)
{
    AstNode *node;
    if (kind == AST_ROOT) {
        node = m_arena_alloc(arena, sizeof(AstRoot));
    } else {
        node = m_arena_alloc(arena, sizeof(AstNode));
    }
    node->kind = kind;
    return node;
}


/* === to dot stuff === */

static u32 dot_node_counter = 0;

static u32 ast_to_dot_impl(Str8Builder *sb, AstNode *node)
{
    if (node == NULL) return 0;
    u32 node_id = ++dot_node_counter;
    
    switch (node->kind) {
    case AST_VAR: {
        AstVar var = node->var;
        str_builder_sprintf(sb, "  node%d [label=\"VAR: %s", 2, node_id, var.identifier.lexeme.str);
        if (var.typed_lit.raw_type.type_name.len > 0) {
            str_builder_sprintf(sb, ": %s", 2, var.typed_lit.raw_type.type_name.str);
        }
        str_builder_append_str8(sb, STR8_LIT("\"];\n"));
    } break;
    case AST_ROOT: {
        AstRoot *root = AS_ROOT(node);
        str_builder_sprintf(sb, "  node%d [label=\"ROOT\"];\n", 1, node_id);
        
        FOR_EACH(AstNode *, child, root->global_variables) {
            u32 child_id = ast_to_dot_impl(sb, child);
            if (child_id > 0) {
                str_builder_sprintf(sb, "  node%d -> node%d;\n", 2, node_id, child_id);
            }
        }
        FOR_EACH(AstNode *, child, root->global_functions) {
            u32 child_id = ast_to_dot_impl(sb, child);
            if (child_id > 0) {
                str_builder_sprintf(sb, "  node%d -> node%d;\n", 2, node_id, child_id);
            }
        }
        FOR_EACH(AstNode *, child, root->global_structs) {
            u32 child_id = ast_to_dot_impl(sb, child);
            if (child_id > 0) {
                str_builder_sprintf(sb, "  node%d -> node%d;\n", 2, node_id, child_id);
            }
        }
        FOR_EACH(AstNode *, child, root->global_enums) {
            u32 child_id = ast_to_dot_impl(sb, child);
            if (child_id > 0) {
                str_builder_sprintf(sb, "  node%d -> node%d;\n", 2, node_id, child_id);
            }
        }
    } break;
    default:
        return 0;
    }
    
    return node_id;
}

void ast_to_dot(Str8Builder *sb, AstNode *node)
{
    dot_node_counter = 0;
    str_builder_append_str8(sb, STR8_LIT("digraph AST {\n"));
    str_builder_append_str8(sb, STR8_LIT("  rankdir=TB;\n"));
    ast_to_dot_impl(sb, node);
    str_builder_append_str8(sb, STR8_LIT("}\n"));
}

/* === printing stuff === */
static void print_indent(Str8Builder *sb, u32 indent)
{
    for (u32 i = 0; i < indent; i++) {
        str_builder_append_u8(sb, ' ');
    }
}

static void print_type_info_raw(Str8Builder *sb, TypeInfoRaw raw_type)
{
    str_builder_sprintf(sb, "%s", 1, raw_type.type_name.str);
}

static void print_literal(Str8Builder *sb, LiteralType lit_type, Str8View literal)
{
    if (lit_type != LIT_NONE) {
        str_builder_append_str8(sb, literal);
    }
}

static void print_typed_literal(Str8Builder *sb, TypedLiteral typed_lit)
{
    print_type_info_raw(sb, typed_lit.raw_type);
    str_builder_append_str8(sb, STR8_LIT(" = "));
    print_literal(sb, typed_lit.lit_type, typed_lit.literal);
}

static void ast_node_to_str(Str8Builder *sb, AstNode *head, u32 indent)
{
    if (head == NULL) return;
    if (indent != 0) str_builder_append_u8(sb, '\n');
    print_indent(sb, indent);
    str_builder_append_u8(sb, '(');
    str_builder_sprintf(sb, "%s ", 1, ast_node_kind_str_map[head->kind]);

    // AST_EXPR_UNARY = 0,
    // AST_EXPR_BINARY,
    // AST_EXPR_LITERAL,
    // AST_EXPR_CALL,
    // AST_WHILE,
    // AST_IF,
    // AST_BLOCK,
    // AST_ASSIGNMENT,
    // AST_FUNC,
    // AST_STRUCT,
    // AST_ENUM,
    // AST_VAR,

    // ArrayList list;
    // AstNode *it;
    // for (size_t i = 0; i < list.size; i++) {
    //     it = *(AstNode **)arraylist_get(&list, i);
    // }

    // TODO: Use macros to generate the lists we need.

    switch (head->kind) {
    case AST_VAR: {
        AstVar var = head->var;
        str_builder_append_str8(sb, var.identifier.lexeme);
        str_builder_append_str8(sb, STR8_LIT(": "));
        print_typed_literal(sb, var.typed_lit);
    }; break;
    case AST_ROOT: {
        AstRoot *root = AS_ROOT(head);
        FOR_EACH(AstNode *, it, root->global_variables) {
            ast_node_to_str(sb, it, indent + 1);
        }
        FOR_EACH(AstNode *, it, root->global_functions) {
            ast_node_to_str(sb, it, indent + 1);
        }
        FOR_EACH(AstNode *, it, root->global_structs) {
            ast_node_to_str(sb, it, indent + 1);
        }
        FOR_EACH(AstNode *, it, root->global_enums) {
            ast_node_to_str(sb, it, indent + 1);
        }
    }; break;
    }

    str_builder_append_u8(sb, ')');
}

void ast_to_str(Str8Builder *sb, AstNode *root)
{
    ast_node_to_str(sb, (AstNode *)root, 0);
    str_builder_end(sb, true);
}
