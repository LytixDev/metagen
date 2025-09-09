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
#ifndef PARSER_NEW_H
#define PARSER_NEW_H

#include "ast_new.h"
#include "base.h"
#include "error.h"
#include "lex.h"

typedef struct {
    Arena *arena;
    ErrorHandler *e;
    ArrayList tokens;
    u32 token_pos;

    //Lexer lexer;
    /*
     * When unlex is true we do not invoke the lexer in lex_next() and instead return
     * the previously lexed token
     */
    //bool unlex;
    //Token previous;

    //u32 stmt_lineno; // The first line number in the source code of the current stmt
} Parser;

AstRoot *parse(Arena *arena, ArrayList tokens, ErrorHandler *e);

#endif /* PARSER_NEW_H */
