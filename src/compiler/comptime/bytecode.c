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
#include "base/str.h"
#include "compiler/ast.h"
#include "compiler/compiler.h"
#include "compiler/type.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

char *op_code_str_map[OP_TYPE_LEN] = {
    "OP_ADDW", "OP_SUBW",  "OP_MULW",   "OP_DIVW",  "OP_LSHIFT", "OP_RSHIFT", "OP_GE",
    "OP_LE",   "OP_NOT",   "OP_JMP",    "OP_BIZ",   "OP_BNZ",    "OP_CONSW",  "OP_PUSHN",
    "OP_POPN", "OP_LOADI", "OP_STOREI", "OP_PRINT", "OP_RETURN",
};


s64 debug_line = -1;
s64 lines_written = -1;


/* Bytecode dissasembler */
static void disassemble_instruction(Bytecode *b, Str8List source_lines)
{
    OpCode instruction = b->code[b->code_offset];
    s64 source_line = b->source_lines[b->code_offset];
    s64 printed_chars = 0;
    printed_chars += printf("%04d %s", b->code_offset, op_code_str_map[instruction]);
    b->code_offset++;

    switch (instruction) {
    case OP_PRINT: {
        u8 n_args = b->code[b->code_offset];
        b->code_offset++;
        printed_chars += printf(" args %d", n_args);
    }; break;
    case OP_BIZ:
    case OP_BNZ: {
        u32 current_offset = b->code_offset;
        BytecodeImm value = *(BytecodeImm *)(b->code + b->code_offset);
        b->code_offset += sizeof(BytecodeImm);
        printed_chars += printf(" %d", value + current_offset + 1);
    }; break;
    case OP_POPN:
    case OP_PUSHN:
    case OP_LOADI:
    case OP_STOREI: {
        BytecodeImm value = *(BytecodeImm *)(b->code + b->code_offset);
        b->code_offset += sizeof(BytecodeImm);
        printed_chars += printf(" %d", value);
    }; break;
    // case OP_JMP:
    case OP_CONSW: {
        BytecodeWord value = *(BytecodeWord *)(b->code + b->code_offset);
        b->code_offset += sizeof(BytecodeWord);
        printed_chars += printf(" %ld", value);
    }; break;
    default:
        break;
    }

    for (s64 i = printed_chars; i < 24; i++) {
        printf(" ");
    }

    if (source_line != -1) {
        printf("%zu", source_line + 1);
        /* Ensures we only write each source line once */
        if (source_line > lines_written) {
            /* Do not print indent */
            Str8 line = source_lines.strs[source_line];
            size_t indents = 0;
            while (line.str[indents] == ' ') { // NOTE. What about tabs?
                indents++;
            }
            printf(": %.*s ", (int)(line.len - indents), line.str + indents);
        }
        lines_written = source_line;
    }
}

void disassemble(Bytecode b, Str8 source)
{
    Str8List source_lines = str_list_from_split(source, '\n');
    printf("--- bytecode ---\n");
    u32 code_offset_end = b.code_offset;
    b.code_offset = 0;
    while (b.code_offset < code_offset_end) {
        disassemble_instruction(&b, source_lines);
        putchar('\n');
    }
    printf("--- bytecode end ---\n");

    str_list_free(&source_lines);
}


/* Bytecode assembler */
static u32 write_instruction(Bytecode *b, OpCode byte, s64 debug_source_line)
{
    b->source_lines[b->code_offset] = debug_source_line;
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
        BytecodeImm *offset = hashmap_get(&locals->map, ident.str, ident.len);
        if (offset != NULL) {
            return (BytecodeImm)offset - 1;
        }
    }
    ASSERT_NOT_REACHED;
}

static void bytecode_compiler_init(BytecodeCompiler *compiler, SymbolTable symt_root)
{
    compiler->bytecode.code_offset = 0;
    compiler->symt_root = symt_root;
    compiler->flags = BCF_LOAD_IDENT;
    compiler->locals = make_locals(NULL);
}

static void bytecode_compiler_free(BytecodeCompiler *compiler)
{
    for (Locals *locals = compiler->locals; locals != NULL;) {
        hashmap_free(&locals->map);
        Locals *old = locals;
        locals = locals->parent;
        free(old);
    }
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
        ast_expr_to_bytecode(compiler, expr->right);
        ast_expr_to_bytecode(compiler, expr->left);
        switch (expr->op) {
        default:
            printf("Binary op not handled\n");
            break;
        case TOKEN_PLUS:
            write_instruction(&compiler->bytecode, OP_ADDW, debug_line);
            break;
        case TOKEN_MINUS:
            write_instruction(&compiler->bytecode, OP_SUBW, debug_line);
            break;
        case TOKEN_EQ:
            write_instruction(&compiler->bytecode, OP_SUBW, debug_line);
            write_instruction(&compiler->bytecode, OP_NOT, debug_line);
            break;
        case TOKEN_NEQ:
            write_instruction(&compiler->bytecode, OP_SUBW, debug_line);
            break;
        case TOKEN_GREATER:
            write_instruction(&compiler->bytecode, OP_GE, debug_line);
            break;
        case TOKEN_LESS:
            write_instruction(&compiler->bytecode, OP_LE, debug_line);
            break;
        }
    } break;
    case EXPR_LITERAL: {
        AstLiteral *expr = AS_LITERAL(head);
        if (expr->lit_type == LIT_NUM) {
            u32 literal = str_view_to_u32(expr->literal, NULL);
            write_instruction(&compiler->bytecode, OP_CONSW, debug_line);
            writew(&compiler->bytecode, literal);
        } else if (expr->lit_type == LIT_IDENT) {
            write_instruction(&compiler->bytecode,
                              compiler->flags == BCF_STORE_IDENT ? OP_STOREI : OP_LOADI,
                              debug_line);
            writei(&compiler->bytecode, find_ident_offset(compiler, expr->sym->name));
        } else {
            printf("Ast literal expr kind not handled\n");
        }
    } break;
    };
}

static void ast_stmt_to_bytecode(BytecodeCompiler *compiler, AstStmt *head)
{
    debug_line = head->line;
    switch (head->kind) {
    default:
        printf("Ast stmt %d not handled\n", head->kind);
        break;
    case STMT_ASSIGNMENT: {
        AstAssignment *assignment = AS_ASSIGNMENT(head);
        ast_expr_to_bytecode(compiler, assignment->right);
        compiler->flags = BCF_STORE_IDENT;
        ast_expr_to_bytecode(compiler, assignment->left);
        compiler->flags = BCF_LOAD_IDENT;
    } break;
    case STMT_IF: {
        AstIf *if_ = AS_IF(head);
        u32 endif_target;
        ast_expr_to_bytecode(compiler, if_->condition);
        /* If false, jump to the else branch */
        u32 else_target = write_instruction(&compiler->bytecode, OP_BIZ, head->line);
        writei(&compiler->bytecode, 0);
        /* If branch */
        ast_stmt_to_bytecode(compiler, if_->then);
        /* Skip the else branch */
        if (if_->else_) {
            endif_target = write_instruction(&compiler->bytecode, OP_CONSW, head->line);
            writew(&compiler->bytecode, 0);
            write_instruction(&compiler->bytecode, OP_JMP, head->line);
        }
        /* Else branch */
        patchi(&compiler->bytecode, else_target,
               compiler->bytecode.code_offset - else_target - sizeof(BytecodeImm));
        if (if_->else_) {
            ast_stmt_to_bytecode(compiler, if_->else_);
            /* Path the jump to end target */
            patchw(&compiler->bytecode, endif_target, compiler->bytecode.code_offset);
        }

    } break;
    case STMT_WHILE: {
        AstWhile *while_ = AS_WHILE(head);
        u32 condition_target = compiler->bytecode.code_offset;
        ast_expr_to_bytecode(compiler, while_->condition);
        /* If condition is zero, skip body */
        u32 end_target = write_instruction(&compiler->bytecode, OP_BIZ, head->line);
        writei(&compiler->bytecode, 0);
        /* Loop body */
        ast_stmt_to_bytecode(compiler, while_->body);
        /* Jump back to the condition */
        write_instruction(&compiler->bytecode, OP_CONSW, head->line);
        writew(&compiler->bytecode, (BytecodeWord)condition_target);
        write_instruction(&compiler->bytecode, OP_JMP, head->line);
        /* Patch the skip body jump */
        patchi(&compiler->bytecode, end_target,
               compiler->bytecode.code_offset - end_target - sizeof(BytecodeImm));
    } break;
    case STMT_BLOCK: {
        AstBlock *block = AS_BLOCK(head);
        bool no_new_syms = block->symt_local->sym_len == 0;
        u32 var_space_in_words = 0;
        if (!no_new_syms) {
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
                    // TODO: align? Question of performance.
                    var_space += type_info_bit_size(sym->type_info);
                }
            }
            var_space_in_words = (var_space + sizeof(BytecodeWord) - 1) / sizeof(BytecodeWord);
            write_instruction(&compiler->bytecode, OP_PUSHN, head->line);
            writei(&compiler->bytecode, (BytecodeImm)var_space_in_words);
        }

        AstList *stmt = block->stmts;
        for (AstListNode *n = stmt->head; n != NULL; n = n->next) {
            ast_stmt_to_bytecode(compiler, (AstStmt *)n->this);
        }

        if (!no_new_syms) {
            write_instruction(&compiler->bytecode, OP_POPN, head->line);
            writei(&compiler->bytecode, (BytecodeImm)var_space_in_words);
            Locals *old = compiler->locals;
            compiler->locals = old->parent;
            hashmap_free(&old->map);
            free(old);
        }
    } break;
    case STMT_PRINT: {
        AstList *stmt = AS_LIST(head);
        u8 n_args = 0;
        for (AstListNode *n = stmt->head; n != NULL; n = n->next) {
            n_args++;
            ast_expr_to_bytecode(compiler, (AstExpr *)n->this);
        }
        write_instruction(&compiler->bytecode, OP_PRINT, head->line);
        write_instruction(&compiler->bytecode, n_args, head->line);
    } break;
    }
}

void ast_func_to_bytecode(BytecodeCompiler *compiler, AstFunc *func)
{
    assert(func->body != NULL);
    Symbol *sym = get_sym_by_name(&compiler->symt_root, func->name);
    assert(sym != NULL && sym->kind == SYMBOL_FUNC);

    // SymbolTable params = sym->symt_local;
    // u32 params_space = 0;
    // for (u32 i = 0; i < params.sym_len; i++) {
    //     Symbol *param = params.symbols[i];
    //     hashmap_put(&compiler->locals->map, param->name.str, param->name.len,
    //                 (void *)(compiler->bytecode.code_offset + params_space + 1),
    //                 sizeof(void *), false);
    //     params_space += type_info_bit_size(param->type_info);
    // }


    ast_stmt_to_bytecode(compiler, func->body);

    /* Function epilogue */
    write_instruction(&compiler->bytecode, OP_RETURN, (s64)-1);
}

Bytecode ast_to_bytecode(SymbolTable symt_root, AstRoot *root)
{
    assert(root->funcs.head != NULL);
    BytecodeCompiler compiler;
    bytecode_compiler_init(&compiler, symt_root);

    for (AstListNode *node = root->funcs.head; node != NULL; node = node->next) {
        ast_func_to_bytecode(&compiler, AS_FUNC(node->this));
    }

    bytecode_compiler_free(&compiler);
    return compiler.bytecode;
}
