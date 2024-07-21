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
#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "base/base.h"
#include "base/sac_single.h"
#include "lex.h"
#include "parser.h"

TokenType token_precedences[TOKEN_TYPE_ENUM_COUNT] = {
    0, // TOKEN_ERR,
    0, // TOKEN_NUM,
    0, // TOKEN_STR,
    1, // TOKEN_ASSIGNMENT,
    5, // TOKEN_PLUS,
    5, // TOKEN_MINUS,
    10, // TOKEN_STAR,
    10, // TOKEN_SLASH,
    4, // TOKEN_LSHIFT,
    4, // TOKEN_RSHIFT,
    3, // TOKEN_EQ,
    3, // TOKEN_NEQ,
    3, // TOKEN_LESS,
    3, // TOKEN_GREATER,
    0, // TOKEN_LPAREN,
    0, // TOKEN_RPAREN,
    0, // TOKEN_SEMICOLON,
    0, // TOKEN_EOF,
    0, // TOKEN_COMMA,
    0, // TOKEN_IDENTIFIER,
    0, // TOKEN_FUNC,
    0, // TOKEN_BEGIN,
    0, // TOKEN_END,
    0, // TOKEN_RETURN,
    0, // TOKEN_PRINT,
    0, // TOKEN_BREAK,
    0, // TOKEN_IF,
    0, // TOKEN_THEN,
    0, // TOKEN_ELSE,
    0, // TOKEN_WHILE,
    0, // TOKEN_DO,
    0, // TOKEN_VAR,
};

char *PARSE_ERROR_MSGS[PET_LEN] = {
    "Expected ')' to terminate the group expression", /* PET_EXPECTED_RPAREN */
    "", /* PET_CUSTOME */
};

static AstExpr *parse_expr(Parser *parser, u32 precedence);

/* Wrapper so we can print the token in debug mode */
static inline Token next_token(Parser *parser)
{
    Token token = lex_next(&parser->arena, &parser->lexer);
#ifdef DEBUG
    token_print(token);
#endif
    return token;
}


static ParseError *make_parse_error(Arena *arena, Token *failed, ParseErrorType pet, char *msg)
{
    ParseError *parse_error = m_arena_alloc(arena, sizeof(ParseError));
    if (msg != NULL) {
        size_t msg_len = strlen(msg) + 1; // TODO: avoid strlen?
        parse_error->msg = m_arena_alloc(arena, msg_len);
        memcpy(parse_error->msg, msg, msg_len);
    } else {
        parse_error->msg = NULL;
    }

    parse_error->type = pet;
    parse_error->failed = failed;
    parse_error->next = NULL;
    return parse_error;
}

static void parse_error_append(Parser *parser, Token *failed, ParseErrorType pet, char *msg)
{
    ParseError *parse_error = make_parse_error(&parser->arena, failed, pet, msg);
    if (parser->err_head == NULL) {
        parser->err_head = parse_error;
    } else {
        parser->err_tail->next = parse_error;
    }
    parser->err_tail = parse_error;

    parser->n_errors++;
}

static AstExprUnary *make_unary(Arena *arena, AstExpr *expr, TokenType op)
{
    AstExprUnary *unary = m_arena_alloc(arena, sizeof(AstExprUnary));
    unary->type = EXPR_UNARY;
    unary->op = op;
    unary->expr = expr;
    return unary;
}

static AstExprBinary *make_binary(Arena *arena, AstExpr *left, TokenType op, AstExpr *right)
{
    AstExprBinary *binary = m_arena_alloc(arena, sizeof(AstExprBinary));
    binary->type = EXPR_BINARY;
    binary->op = op;
    binary->left = left;
    binary->right = right;
    return binary;
}

static AstExprLiteral *make_literal(Arena *arena, Token token)
{
    AstExprLiteral *literal = m_arena_alloc(arena, sizeof(AstExprLiteral));
    literal->type = EXPR_LITERAL;
    if (token.type == TOKEN_NUM) {
        literal->lit_type = LIT_NUM;
        literal->num_value = token.num_value;
    } else {
        literal->lit_type = LIT_STR;
        literal->str_list_idx = token.str_list_idx;
    }
    return literal;
}

static AstExprListNode *make_list_node(Arena *arena, AstExpr *this)
{
    AstExprListNode *node = m_arena_alloc(arena, sizeof(AstExprListNode));
    node->this = this;
    node->next = NULL;
    return node;
}

static AstExprList *make_list(Arena *arena, AstExpr *head)
{
    AstExprList *list = m_arena_alloc(arena, sizeof(AstExprList));
    list->type = EXPR_LIST;
    AstExprListNode head_node = { .this = head, .next = NULL };
    list->head = head_node;
    list->tail = &list->head;
    return list;
}

static bool is_bin_op(Token token)
{
    switch (token.type) {
    case TOKEN_PLUS:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_SLASH:
    case TOKEN_LSHIFT:
    case TOKEN_RSHIFT:
    case TOKEN_EQ:
    case TOKEN_NEQ:
    case TOKEN_LESS:
    case TOKEN_GREATER:
        return true;
    default:
        return false;
    }
}

static Token consume_or_err(Parser *parser, TokenType expected, ParseErrorType pet)
{
    Token token = next_token(parser);
    if (token.type != expected) {
        parse_error_append(parser, &token, pet, NULL);
        return (Token){ .type = TOKEN_ERR };
    }
    return token;
}

static AstExpr *parse_primary(Parser *parser)
{
    Token token = next_token(parser);
    switch (token.type) {
    case TOKEN_LPAREN: {
        AstExpr *expr = parse_expr(parser, 0);
        Token err = consume_or_err(parser, TOKEN_RPAREN, PET_EXPECTED_RPAREN);
        if (err.type == TOKEN_ERR) {
            // TODO: gracefully continue
            fprintf(stderr, "err");
        }
        return expr;
    }
    case TOKEN_MINUS: {
        /* Unary minus */
        AstExpr *expr = parse_expr(parser, 0);
        return (AstExpr *)make_unary(&parser->arena, expr, TOKEN_MINUS);
    }
    case TOKEN_NUM:
    case TOKEN_STR:
    case TOKEN_IDENTIFIER:
        return (AstExpr *)make_literal(&parser->arena, token);
    default:
        ASSERT_NOT_REACHED;
    }
}

static AstExpr *parse_increasing_precedence(Parser *parser, AstExpr *left, u32 precedence)
{
    Token next = lex_peek(&parser->arena, &parser->lexer);
    if (!is_bin_op(next))
        return left;

    u32 next_precedence = token_precedences[next.type];
    if (precedence >= next_precedence)
        return left;

    next_token(parser);
    AstExpr *right = parse_expr(parser, next_precedence);
    return (AstExpr *)make_binary(&parser->arena, left, next.type, right);
}

static AstExpr *parse_expr(Parser *parser, u32 precedence)
{
    AstExpr *left = parse_primary(parser);
    AstExpr *expr;
    while (1) {
        expr = parse_increasing_precedence(parser, left, precedence);
        if (expr == left) // pointer comparison
            break;

        left = expr;
    }

    return left;
}

static AstExpr *parse_expr_list(Parser *parser)
{
    AstExpr *expr = parse_expr(parser, 0);
    if (!(lex_peek(&parser->arena, &parser->lexer).type == TOKEN_COMMA)) {
        return expr;
    }

    AstExprList *list = make_list(&parser->arena, expr);
    AstExprListNode *list_node;
    do {
        next_token(parser);
        expr = parse_expr(parser, 0);
        list_node = make_list_node(&parser->arena, expr);
        list->tail->next = list_node;
        list->tail = list_node;
    } while (lex_peek(&parser->arena, &parser->lexer).type == TOKEN_COMMA);

    return (AstExpr *)list;
}

ParseResult parse(char *input)
{
    Parser parser = {
        .n_errors = 0,
        .err_head = NULL,
        .err_tail = NULL,
    };
    lex_init(&parser.lexer, input);
    m_arena_init_dynamic(&parser.arena, 2, 512);

    AstExpr *head = parse_expr_list(&parser);
    return (ParseResult){
        .n_errors = parser.n_errors,
        .err_head = parser.err_head,
        .head = head,
        .str_list = parser.lexer.str_list,
        .str_list_len = parser.lexer.str_list_len,
    };
}
