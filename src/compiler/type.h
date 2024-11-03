/*
 *  Copyright (C) 2024 Nicolai Brand (https://lytix.dev)
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
#ifndef TYPE_H
#define TYPE_H

#include "ast.h"
#include "base/nicc.h"

typedef struct compiler_t Compiler; // forward decl from compiler.h

typedef enum {
    TYPE_INTEGER = 0,
    TYPE_BOOL,
    TYPE_STRUCT,
    TYPE_ENUM,
    TYPE_FUNC,
    TYPE_ARRAY,
    TYPE_POINTER,
    TYPE_INFO_KIND_LEN
} TypeInfoKind;

typedef struct {
    TypeInfoKind kind;
    bool is_resolved;
    Str8 generated_by_name; // Not used by TYPE_ARRAY
} TypeInfo;

typedef struct {
    TypeInfo info;
    u32 size;
    bool is_signed;
} TypeInfoInteger;

typedef struct {
    TypeInfo info;
} TypeInfoBool;

// NOTE: Not an actual TypeInfo kind
typedef struct {
    bool is_resolved;
    Str8 name;
    u32 offset;
    union {
        TypeInfo *type;
        AstTypeInfo ati;
    };
} TypeInfoStructMember;

typedef struct {
    TypeInfo info;
    u32 struct_id; // Useful for graph algorithms
    u32 members_len;
    TypeInfoStructMember **members;
} TypeInfoStruct;

typedef struct {
    TypeInfo info;
    u32 members_len;
    Str8 *member_names;
} TypeInfoEnum;

typedef struct {
    TypeInfo info;
    u32 n_params;
    Str8 *param_names;
    TypeInfo **param_types;
    TypeInfo *return_type;
} TypeInfoFunc;

typedef struct {
    TypeInfo info;
    TypeInfo *element_type;
    s32 elements; // -1 then array is dynamic
} TypeInfoArray;

typedef struct {
    TypeInfo info;
    TypeInfo *pointer_to;
} TypeInfoPointer;


/* Symbol tuff */

typedef enum {
    // SYMBOL_GLOBAL_ARRAY,
    SYMBOL_TYPE, // Structs and enums
    SYMBOL_FUNC,
    SYMBOL_GLOBAL_VAR,
    SYMBOL_LOCAL_VAR,
    SYMBOL_PARAM,
    SYMBOL_MEMBER,

    SYMBOL_TYPE_LEN,
} SymbolKind;


typedef struct symbol_table_t SymbolTable;
typedef struct symbol_t Symbol;

struct symbol_table_t {
    Symbol **symbols;
    u32 sym_len;
    u32 sym_cap;

    TypeInfo **types;
    u32 type_len;
    u32 type_cap;

    u32 struct_count;

    HashMap map; // Key: string (u8 *), Value: *Symbol
    SymbolTable *parent; // @NULLABLE
};

struct symbol_t {
    SymbolKind kind;
    u32 seq_no; // Sequence number in the symbol table this symbol belongs to
    Str8 name;
    TypeInfo *type_info; // @NULLABLE
    AstNode *node; // @NULLABLE. Node which defined this symbol. If NULL then defined by compiler
    SymbolTable symt_local; // FUNC and TYPE (structs and enums) create local symbol tables
};


void symbol_generate(Compiler *compiler, AstRoot *root);

#endif /* TYPE_H */
