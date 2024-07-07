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

#include <assert.h>
#include <stdbool.h>

#include "base/sac_single.h"
#include "interpreter/ast.h"
#include "interpreter/parser.h"
#include "tests.h"


static bool ast_cmp(AstExpr *a, AstExpr *b)
{
	if (a->type != b->type)
		return false;

	switch (a->type) {
	case EXPR_LITERAL: {
		if (AS_LITERAL(a)->lit_type != AS_LITERAL(b)->lit_type)
			return false;
		// TODO: also consider other literal types
		return AS_LITERAL(a)->num_value == AS_LITERAL(b)->num_value;
	};
	case EXPR_UNARY: {
		// TODO: ...
		return true;
	};
	case EXPR_BINARY: {
		AstExprBinary *A = AS_BINARY(a);
		AstExprBinary *B = AS_BINARY(b);
		return A->op == B->op && ast_cmp(A->left, B->left) && ast_cmp(A->right, B->right);
	};
	}

	return true;
}

void test_binary_precedence(void)
{
	char *input = "4 * 3 + 7;";

	AstExprBinary expected;
	expected.type = EXPR_BINARY;
	expected.op = TOKEN_PLUS;

	AstExprLiteral right = { .type = EXPR_LITERAL, .lit_type = LIT_NUM, .num_value = 7 };
	AstExprBinary left = { .type = EXPR_BINARY, .op = TOKEN_STAR };
	AstExprLiteral left_left = { .type = EXPR_LITERAL, .lit_type = LIT_NUM, .num_value = 4 };
	AstExprLiteral left_right = { .type = EXPR_LITERAL, .lit_type = LIT_NUM, .num_value = 3 };
	left.left = (AstExpr *)&left_left;
	left.right = (AstExpr *)&left_right;
	expected.right = (AstExpr *)&right;
	expected.left = (AstExpr *)&left;

	AstExpr *output = parse(input);
	assert(ast_cmp((AstExpr *)&expected, output));
}
