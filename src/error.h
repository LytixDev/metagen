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
#ifndef ERROR_H
#define ERROR_H

#include "ast.h"
#include "base.h"
#include "lex.h"
#include "type.h"

// TODO: This file should call functions from log.c

#define ERROR_HANDLER_MAX_ERRORS 64 // Just give up after this

typedef struct compiler_error_t CompilerError;
struct compiler_error_t {
    CompilerError *next;
    // ErrorCode code;
    Str8 msg;
};

typedef struct error_handler_t {
    Arena arena;
    char *input;
    char *file_name;
    u32 n_errors;
    CompilerError *head;
    CompilerError *tail;
} ErrorHandler;

void error_handler_init(ErrorHandler *e, u8 *input, char *file_name);
void error_handler_release(ErrorHandler *e);
void error_handler_reset(ErrorHandler *e);

void error_msg_str8(ErrorHandler *e, Str8 msg);
void error_lex(ErrorHandler *e, char *msg, Point start, Point end);
void error_parse(ErrorHandler *e, char *msg, Token guilty);
void error_node(ErrorHandler *e, char *msg, AstNode *guilty);
void error_sym(ErrorHandler *e, char *msg, Str8 sym_name);
void error_typecheck_binary(ErrorHandler *e, char *msg, AstNode *guilty, TypeInfo *l, TypeInfo *r);
// void error_type_unresolved(ErrorHandler *e, Str8List list, char *msg, Str8 type_name);

#endif /* ERROR_H */
