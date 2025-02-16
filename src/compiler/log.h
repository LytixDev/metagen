/*
 *  Copyright (C) 2025 Nicolai Brand (https://lytix.dev)
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
#ifndef LOG_H
#define LOG_H

#include "base/types.h"
#include "lex.h"
#include "ast.h"
#include "base/str.h"

typedef enum {
    LOG_ERROR = 0,
    LOG_WARN = 1,
    LOG_ALL = 2
} LogLevel;


typedef struct logger_t {
    LogLevel log_level;
    bool print_immeditately;

    char *source_code;
    char *source_file_name;
} Logger;



void logger_init(LogLevel log_level);
void logger_set_source(char *source_file_name, char *source_code);

void log_msg(LogLevel log_level, Str8 msg);
void log_lex_err(Str8 msg, char *source, Point start, Point end);
void log_parse_err(Str8 *msg, Token guilty);
void log_ast_err(Str8 *msg, AstNode *guilty);
//void error_sym(ErrorHandler *e, char *msg, Str8 sym_name);
//void error_typecheck_binary(ErrorHandler *e, char *msg, AstNode *guilty, TypeInfo *l, TypeInfo *r);
// void error_type_unresolved(ErrorHandler *e, Str8List list, char *msg, Str8 type_name);




#endif /* LOG_H */
