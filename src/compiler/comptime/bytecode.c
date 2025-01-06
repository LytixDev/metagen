/*
 *  Copyright (C) 2024-2025 Nicolai Brand (https://lytix.dev)
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
#include "compiler/comptime/bytecode.h"
#include "compiler/ast.h"
#include "compiler/type.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

char *op_code_str_map[OP_TYPE_LEN] = {
    "OP_ADDW", "OP_SUBW",  "OP_MULW",  "OP_DIVW", "OP_LSHIFT", "OP_RSHIFT", "OP_JMP",   "OP_BIZ",
    "OP_BNZ",  "OP_CONSW", "OP_PUSHN", "OP_POPN", "OP_LOADL",  "OP_STOREL", "OP_PRINT", "OP_RETURN",
};


/* Bytecode dissasembler */
static void disassemble_instruction(Bytecode *b)
{
    OpCode instruction = b->code[b->code_offset];
    printf("%04d %s", b->code_offset, op_code_str_map[instruction]);
    b->code_offset++;
    switch (instruction) {
    case OP_PRINT: {
        u8 n_args = b->code[b->code_offset];
        b->code_offset++;
        printf(" args %d", n_args);
    }; break;
    case OP_BIZ:
    case OP_BNZ:
    case OP_POPN:
    case OP_PUSHN:
    case OP_LOADL:
    case OP_STOREL: {
        BytecodeImm value = *(BytecodeImm *)(b->code + b->code_offset);
        b->code_offset += sizeof(BytecodeImm);
        printf(" %d", value);
    }; break;
    // case OP_JMP:
    case OP_CONSW: {
        BytecodeWord value = *(BytecodeWord *)(b->code + b->code_offset);
        b->code_offset += sizeof(BytecodeWord);
        printf(" %ld", value);
    }; break;
    default:
        break;
    }
}

void disassemble(Bytecode b)
{
    printf("--- bytecode ---\n");
    u32 code_offset_end = b.code_offset;
    b.code_offset = 0;
    while (b.code_offset < code_offset_end) {
        disassemble_instruction(&b);
        putchar('\n');
    }
    printf("--- bytecode end ---\n");
}


/* Bytecode assembler */
static u32 writeu8(Bytecode *b, u8 byte)
{
    b->code[b->code_offset] = byte;
    b->code_offset++;
    return b->code_offset;
}

static u32 writew(Bytecode *b, BytecodeWord v)
{
    *(BytecodeWord *)(b->code + b->code_offset) = v;
    b->code_offset += sizeof(BytecodeWord);
    return b->code_offset;
}

static void writei(Bytecode *b, BytecodeImm v)
{
    *(BytecodeImm *)(b->code + b->code_offset) = v;
    b->code_offset += sizeof(BytecodeImm);
}

static void patchw(Bytecode *b, u32 offset, BytecodeWord value)
{
    *(BytecodeWord *)(b->code + offset) = value;
}

static void patchi(Bytecode *b, u32 offset, BytecodeImm value)
{
    *(BytecodeImm *)(b->code + offset) = value;
}

static Locals *make_locals(Locals *parent)
{
    Locals *locals = malloc(sizeof(Locals));
    locals->parent = parent;
    hashmap_init(&locals->map);
    return locals;
}

static BytecodeImm find_ident_offset(BytecodeCompiler *compiler, Str8 ident)
{
    for (Locals *locals = compiler->locals; locals; locals = locals->parent) {
        void *offset = hashmap_get(&locals->map, ident.str, ident.len);
        if (offset != NULL) {
            return (BytecodeImm)offset - 1;
        }
    }
    ASSERT_NOT_REACHED;
}

static void bytecode_compiler_init(BytecodeCompiler *compiler)
{
    compiler->flags = BCF_LOAD_IDENT;
    compiler->bytecode.code_offset = 0;
    compiler->locals = make_locals(NULL);
}

static void ast_expr_to_bytecode(BytecodeCompiler *compiler, AstExpr *head)
{
    switch (head->kind) {
    default:
        printf("Ast expr not handled\n");
        break;
    case EXPR_BINARY: {
        AstBinary *expr = AS_BINARY(head);
        // NOTE: we only support integers right now
        assert(expr->type->kind == TYPE_INTEGER);
        ast_expr_to_bytecode(compiler, expr->left);
        ast_expr_to_bytecode(compiler, expr->right);
        switch (expr->op) {
        default:
            printf("Binary op not handled\n");
            break;
        case TOKEN_PLUS:
            writeu8(&compiler->bytecode, OP_ADDW);
            break;
        case TOKEN_LESS:
            writeu8(&compiler->bytecode, OP_SUBW);
            break;
        }
    } break;
    case EXPR_LITERAL: {
        AstLiteral *expr = AS_LITERAL(head);
        if (expr->lit_type == LIT_NUM) {
            u32 literal = str_view_to_u32(expr->literal, NULL);
            writeu8(&compiler->bytecode, OP_CONSW);
            writew(&compiler->bytecode, literal);
        } else if (expr->lit_type == LIT_IDENT) {
            writeu8(&compiler->bytecode, compiler->flags == BCF_STORE_IDENT ? OP_STOREL : OP_LOADL);
            writei(&compiler->bytecode, find_ident_offset(compiler, expr->sym->name));
        } else {
            printf("Ast literal expr kind not handled\n");
        }
    }; break;
    };
}

static void ast_stmt_to_bytecode(BytecodeCompiler *compiler, AstStmt *head)
{
    switch (head->kind) {
    default:
        printf("Ast stmt %d not handled\n", head->kind);
        break;
    case STMT_ASSIGNMENT: {
        AstAssignment *assignment = AS_ASSIGNMENT(head);
        // store some value at some stack position
        ast_expr_to_bytecode(compiler, assignment->right);
        compiler->flags = BCF_STORE_IDENT;
        ast_expr_to_bytecode(compiler, assignment->left);
        compiler->flags = BCF_LOAD_IDENT;
    } break;
    case STMT_WHILE: {
        AstWhile *while_ = AS_WHILE(head);
        u32 condition_target = compiler->bytecode.code_offset;
        ast_expr_to_bytecode(compiler, while_->condition);
        /* If condition is zero, skip body */
        u32 end_target = writeu8(&compiler->bytecode, OP_BIZ);
        writei(&compiler->bytecode, 0);
        /* Loop body */
        ast_stmt_to_bytecode(compiler, while_->body);
        /* Jump back to the condition */
        writeu8(&compiler->bytecode, OP_CONSW);
        writew(&compiler->bytecode, (BytecodeWord)condition_target);
        writeu8(&compiler->bytecode, OP_JMP);
        /* Patch the skip body jump */
        patchi(&compiler->bytecode, end_target,
               compiler->bytecode.code_offset - end_target - sizeof(BytecodeImm));
    } break;
    case STMT_BLOCK: {
        AstBlock *block = AS_BLOCK(head);
        // TODO: if the block defines no locals we can skip this
        compiler->locals = make_locals(compiler->locals);

        /* Make space for each local variable */
        u32 var_space = 0;
        SymbolTable *symt = block->symt_local;
        for (u32 i = 0; i < symt->sym_len; i++) {
            Symbol *sym = symt->symbols[i];
            if (sym->kind == SYMBOL_LOCAL_VAR) {
                hashmap_put(&compiler->locals->map, sym->name.str, sym->name.len,
                            (void *)(compiler->bytecode.code_offset + var_space + 1),
                            sizeof(void *), false);
                // TODO: align?
                var_space += type_info_bit_size(sym->type_info);
            }
        }
        u32 var_space_in_words = (var_space + sizeof(BytecodeWord) - 1) / sizeof(BytecodeWord);
        if (var_space_in_words != 0) {
            writeu8(&compiler->bytecode, OP_PUSHN);
            writei(&compiler->bytecode, (BytecodeImm)var_space_in_words);
        }

        AstList *stmt = block->stmts;
        for (AstListNode *n = stmt->head; n != NULL; n = n->next) {
            ast_stmt_to_bytecode(compiler, (AstStmt *)n->this);
        }

        if (var_space_in_words != 0) {
            writeu8(&compiler->bytecode, OP_POPN);
            writei(&compiler->bytecode, (BytecodeImm)var_space_in_words);
        }

        Locals *old = compiler->locals;
        compiler->locals = old->parent;
        hashmap_free(&old->map);
        free(old);
    } break;
    case STMT_PRINT: {
        AstList *stmt = AS_LIST(head);
        u8 n_args = 0;
        for (AstListNode *n = stmt->head; n != NULL; n = n->next) {
            n_args++;
            ast_expr_to_bytecode(compiler, (AstExpr *)n->this);
        }
        writeu8(&compiler->bytecode, OP_PRINT);
        writeu8(&compiler->bytecode, n_args);
    } break;
    }
}

void ast_func_to_bytecode(BytecodeCompiler *compiler, AstFunc *func)
{
    assert(func->body != NULL);
    ast_stmt_to_bytecode(compiler, func->body);
    writeu8(&compiler->bytecode, OP_RETURN);
}

Bytecode ast_to_bytecode(AstRoot *root)
{
    assert(root->funcs.head != NULL);
    BytecodeCompiler compiler;
    bytecode_compiler_init(&compiler);
    ast_func_to_bytecode(&compiler, AS_FUNC(root->funcs.head->this));
    return compiler.bytecode;
}

Bytecode bytecode_test(void)
{
    /*
     * var a: s32, b: s32, c: s32
     * a = 1
     * b = 2
     * c = a + b
     * print c
     */
    Bytecode b = { 0 };

    writeu8(&b, OP_PUSHN);
    writei(&b, 3);

    // a = 1
    writeu8(&b, OP_CONSW);
    writew(&b, 1);
    writeu8(&b, OP_STOREL);
    writei(&b, 0);

    // b = 2
    writeu8(&b, OP_CONSW);
    writew(&b, 2);
    writeu8(&b, OP_STOREL);
    writei(&b, 1);

    // c = a + b
    writeu8(&b, OP_LOADL);
    writei(&b, 1);
    writeu8(&b, OP_LOADL);
    writei(&b, 0);
    writeu8(&b, OP_ADDW);
    writeu8(&b, OP_STOREL);
    writei(&b, 2);

    // print c
    writeu8(&b, OP_LOADL);
    writei(&b, 2);
    writeu8(&b, OP_PRINT);
    writeu8(&b, 1);

    writeu8(&b, OP_RETURN);
    return b;
}


Bytecode fib_test(void)
{
    /*
    func fib(n: s32): s32
    begin
        if n = 0 then return 0
        if n = 1 then return 1
        return fib(n - 1) + fib(n - 2)
    end

    func main(): s32
    begin
        print fib(20)
        return 0
    end
    */

    Bytecode b = { 0 };
    /*
       var i: s32
        i := 0
        while i < 10 do
        begin
            print i
            i := i + 1
        end
     */

    // var i: s32
    writeu8(&b, OP_PUSHN);
    writei(&b, 1);
    // i := 0
    writeu8(&b, OP_CONSW);
    writew(&b, 0);
    writeu8(&b, OP_STOREL);
    writei(&b, 0);

    // i < 10 -> 10 - i == 0
    u32 if_start = b.code_offset;
    writeu8(&b, OP_LOADL);
    writei(&b, 0);
    writeu8(&b, OP_CONSW);
    writew(&b, 10);
    writeu8(&b, OP_SUBW);
    u32 else_target = writeu8(&b, OP_BIZ);
    writei(&b, 0);

    // print i
    writeu8(&b, OP_LOADL);
    writei(&b, 0);
    writeu8(&b, OP_PRINT);
    writeu8(&b, 1);

    // i := i + 1
    writeu8(&b, OP_LOADL);
    writei(&b, 0);
    writeu8(&b, OP_CONSW);
    writew(&b, 1);
    writeu8(&b, OP_ADDW);
    writeu8(&b, OP_STOREL);
    writei(&b, 0);

    writeu8(&b, OP_CONSW);
    writew(&b, (BytecodeWord)if_start);
    writeu8(&b, OP_JMP);
    patchi(&b, else_target, b.code_offset - else_target - sizeof(BytecodeImm));

    writeu8(&b, OP_RETURN);
    return b;
}
