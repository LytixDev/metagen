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
#include "base/types.h"
#include "compiler/ast.h"
#include "compiler/type.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

char *op_code_str_map[OP_TYPE_LEN] = {
    "ADDW",  "SUBW", "MULW", "DIVW",  "LSHIFTW",    "RSHIFTW",  "GE",    "LE",
    "NOT",   "JMP",  "BIZ",  "BNZ",   "CONSTANTW", "PUSHNW",  "POPNW", "LDBPW",
    "STBPW", "LDAW", "STAW", "PRINT", "CALL",      "FUNCPRO", "RET",   "EXIT",
};


s64 debug_line = -1;
s64 lines_written = -1;
Str8 return_var_internal_name;


static uintptr_t align_forward(uintptr_t ptr, size_t align)
{
    // assert(is_power_of_two(align));

    uintptr_t p = ptr;
    uintptr_t a = (uintptr_t)align;
    uintptr_t modulo = p & (a - 1);

    if (modulo != 0) {
        p += a - modulo;
    }

    return p;
}

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
        printed_chars += printf(" %d", value + current_offset + 2);
    }; break;
    case OP_POPNW:
    case OP_PUSHNW:
    case OP_LDBPW:
    case OP_STBPW: {
        BytecodeImm value = *(BytecodeImm *)(b->code + b->code_offset);
        b->code_offset += sizeof(BytecodeImm);
        printed_chars += printf(" %d", value);
    }; break;
    // case OP_JMP:
    case OP_CONSTANTW: {
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
        printf("%-3zu", source_line + 1);
        /* Ensures we only write each source line once */
        if (source_line > lines_written) {
            /* Do not print indent */
            Str8 line = source_lines.strs[source_line - 1];
            size_t indents = 0;
            while (line.str[indents] == ' ') { // NOTE. What about tabs?
                indents++;
            }
            printf(" %.*s ", (int)(line.len - indents), line.str + indents);
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

static s64 bytes_to_words(s64 bytes)
{
    return (bytes + sizeof(BytecodeWord) - 1) / sizeof(BytecodeWord);
}

static StackVars *make_stack_vars(StackVars *parent)
{
    StackVars *locals = malloc(sizeof(StackVars));
    locals->parent = parent;
    hashmap_init(&locals->map);
    return locals;
}

static void free_all_stack_vars(StackVars *s)
{
    while (s != NULL) {
        hashmap_free(&s->map);
        StackVars *old = s;
        s = s->parent;
        free(old);
    }
}

static void stack_vars_set(StackVars *s, Str8 name, s64 bp_rel_offset)
{
    /*
     * For storing in the map, we ensure each value is larger than 0.
     * Otherwise, a bp_rel_offset of 0 would be treated as the NULL pointer.
     */
    bp_rel_offset += STACK_VAR_MAP_ADD;
    hashmap_put(&s->map, name.str, name.len, (void *)(bp_rel_offset), sizeof(void *), false);
}

static BytecodeImm stack_vars_get(BytecodeCompiler *bc, Str8 name)
{
    for (StackVars *locals = bc->stack_vars; locals; locals = locals->parent) {
        BytecodeImm *offset = hashmap_get(&locals->map, name.str, name.len);
        if (offset != NULL) {
            return (BytecodeImm)(((size_t)offset) - STACK_VAR_MAP_ADD);
        }
    }
    ASSERT_NOT_REACHED;
}

static void stack_vars_print(StackVars *s)
{
    if (s->parent != NULL) {
        stack_vars_print(s->parent);
    }

    HashMap m = s->map;
    for (int i = 0; i < N_BUCKETS(m.size_log2); i++) {
        struct hm_bucket_t *bucket = &m.buckets[i];
        for (int j = 0; j < HM_BUCKET_SIZE; j++) {
            struct hm_entry_t entry = bucket->entries[j];
            if (entry.key == NULL)
                continue;

            Str8 key = (Str8){ .str = entry.key, .len = entry.key_size };
            s64 val = ((s64)entry.value) - STACK_VAR_MAP_ADD;
            printf("%ld - %s\n", val, key.str);
        }
    }
}

static void func_register_start(BytecodeCompiler *bc, Str8 func_name)
{
    /* We add +1 to the code_offset so 0x0 is not treated as the NULL pointer */
    hashmap_put(&bc->functions, func_name.str, func_name.len,
                (void *)(size_t)(bc->bytecode.code_offset + 1), sizeof(void *), false);
}

static u32 func_get_start(BytecodeCompiler *bc, Str8 func_name)
{
    void *bytecode_offset = hashmap_get(&bc->functions, func_name.str, func_name.len);
    if (bytecode_offset == NULL) {
        /* Function not generated yet, needs to be patched later */
        PatchCall p = (PatchCall){ .offset = bc->bytecode.code_offset, .func_name = func_name };
        bc->patches[bc->calls_to_patch++] = p;
        return 0;
    }

    return (u32)((size_t)bytecode_offset) - 1;
}

static void bytecode_compiler_init(BytecodeCompiler *bc, SymbolTable symt_root)
{
    bc->bytecode.code_offset = 0;
    bc->symt_root = symt_root;
    bc->flags = BCF_LOAD_IDENT;
    bc->calls_to_patch = 0;
    hashmap_init(&bc->globals);
    hashmap_init(&bc->functions);
}

static void bytecode_compiler_free(BytecodeCompiler *bc)
{
    hashmap_free(&bc->globals);
    hashmap_free(&bc->functions);
    // for (StackVars *locals = compiler->stack_vars; locals != NULL;) {
    //     hashmap_free(&locals->map);
    //     StackVars *old = locals;
    //     locals = locals->parent;
    //     free(old);
    // }
}

static BytecodeImm new_stack_vars_from_block(BytecodeCompiler *bc, SymbolTable *symt)
{
    bc->stack_vars = make_stack_vars(bc->stack_vars);
    s64 bp_offset_pre = bc->bp_stack_offset;
    for (u32 i = 0; i < symt->sym_len; i++) {
        Symbol *sym = symt->symbols[i];
        if (sym->kind == SYMBOL_LOCAL_VAR) {
            stack_vars_set(bc->stack_vars, sym->name, bc->bp_stack_offset);
            bc->bp_stack_offset += type_info_byte_size(sym->type_info);

            /*
             * NOTE:
             * The Bytecode compiler aligns every local stack variable to sizeof(BytecodeWord) 
             * boundary (8 bytes). This results in easy codegen but is quite wasteful. 
             * Possible improvents is to try to group local variables so they naturally align, for 
             * instance storing two s32's next to eachother. Then when loading, we have two load 
             * the entire word (64 bits) and emit mask and shift instructions to obtain the s32 we
             * want.
             */
            bc->bp_stack_offset = (s64)align_forward(bc->bp_stack_offset, sizeof(BytecodeWord));
        }
    }
    s64 bp_added_offset = bc->bp_stack_offset - bp_offset_pre;
    s64 var_space_in_words = bytes_to_words(bp_added_offset);
    // TODO: if var_space_in_words not addressable by s64, error
    return (BytecodeImm)var_space_in_words;
}

static void ast_expr_access_struct_member(BytecodeCompiler *bc, AstBinary *expr)
{
    // TODO: struct packing ! Also, struct member offsets are in bits !
    assert(expr->op == TOKEN_DOT);
    assert(expr->left->kind == EXPR_LITERAL); // TODO: Later on, this can be anything
    assert(expr->right->kind == EXPR_LITERAL);

    AstLiteral *struct_lit = (AstLiteral *)expr->left;
    AstLiteral *struct_member = (AstLiteral *)expr->right;
    TypeInfoStruct *struct_type = (TypeInfoStruct *)struct_lit->type;

    s64 member_offset = -1;
    for (u32 i = 0; i < struct_type->members_len; i++) {
        TypeInfoStructMember *member = struct_type->members[i];
        if (STR8_EQUAL(member->name, struct_member->sym->name)) {
            member_offset = member->offset;
        }
    }
    assert(member_offset != -1);

    BytecodeImm bp_offset = stack_vars_get(bc, struct_lit->sym->name);
    bp_offset += member_offset / 8;
    write_instruction(&bc->bytecode, bc->flags == BCF_STORE_IDENT ? OP_STBPW : OP_LDBPW,
                      debug_line);
    writei(&bc->bytecode, bp_offset);
}

static void ast_expr_to_bytecode(BytecodeCompiler *bc, AstExpr *head)
{
    switch (head->kind) {
    default:
        printf("Ast expr not handled\n");
        break;
    case EXPR_CALL: {
        AstCall *call = AS_CALL(head);
        if (call->is_resolved) {
            ast_expr_to_bytecode(bc, (AstExpr *)call->resolved_node);
            break;
        }
        Symbol *callee = get_sym_by_name(&bc->symt_root, call->identifier);
        TypeInfoFunc *callee_type = (TypeInfoFunc *)callee->type_info;

        s64 arg_space_words = 0;
        for (u32 i = 0; i < callee_type->n_params; i++) {
            TypeInfo *param_type = callee_type->param_types[i];
            arg_space_words += bytes_to_words(type_info_byte_size(param_type));
        }
        BytecodeWord return_type_space = bytes_to_words(type_info_byte_size(call->type));

        /* Make stack space for return value */
        write_instruction(&bc->bytecode, OP_PUSHNW, debug_line);
        writei(&bc->bytecode, return_type_space);

        /* Push args */
        if (call->args) {
            for (AstListNode *n = call->args->head; n != NULL; n = n->next) {
                ast_expr_to_bytecode(bc, (AstExpr *)n->this);
            }
        }

        /* Push return address */
        write_instruction(&bc->bytecode, OP_CONSTANTW, debug_line);
        writew(&bc->bytecode, (BytecodeWord)func_get_start(bc, call->identifier));

        /* Call */
        write_instruction(&bc->bytecode, OP_CALL, debug_line);

        /* Reclaim stack space */
        // NOTE/TODO: we never reclaim the return value as we expect it to be used in an assignment
        write_instruction(&bc->bytecode, OP_POPNW, debug_line);
        writei(&bc->bytecode, arg_space_words);
    } break;
    case EXPR_BINARY: {
        AstBinary *expr = AS_BINARY(head);
        // NOTE: we only support integers right now
        assert(expr->type->kind == TYPE_INTEGER);
        if (expr->op == TOKEN_DOT) {
            ast_expr_access_struct_member(bc, expr);
            break;
        }
        ast_expr_to_bytecode(bc, expr->right);
        ast_expr_to_bytecode(bc, expr->left);
        switch (expr->op) {
        default:
            printf("Binary op not handled\n");
            break;
        case TOKEN_PLUS:
            write_instruction(&bc->bytecode, OP_ADDW, debug_line);
            break;
        case TOKEN_MINUS:
            write_instruction(&bc->bytecode, OP_SUBW, debug_line);
            break;
        case TOKEN_STAR:
            write_instruction(&bc->bytecode, OP_MULW, debug_line);
            break;
        case TOKEN_SLASH:
            write_instruction(&bc->bytecode, OP_DIVW, debug_line);
            break;
        case TOKEN_LSHIFT:
            write_instruction(&bc->bytecode, OP_LSHIFTW, debug_line);
            break;
        case TOKEN_RSHIFT:
            write_instruction(&bc->bytecode, OP_RSHIFTW, debug_line);
            break;
        case TOKEN_EQ:
            write_instruction(&bc->bytecode, OP_SUBW, debug_line);
            write_instruction(&bc->bytecode, OP_NOT, debug_line);
            break;
        case TOKEN_NEQ:
            write_instruction(&bc->bytecode, OP_SUBW, debug_line);
            break;
        case TOKEN_GREATER:
            write_instruction(&bc->bytecode, OP_GE, debug_line);
            break;
        case TOKEN_LESS:
            write_instruction(&bc->bytecode, OP_LE, debug_line);
            break;
        }
    } break;
    case EXPR_LITERAL: {
        AstLiteral *expr = AS_LITERAL(head);
        if (expr->lit_type == LIT_NUM) {
            u32 literal = str_view_to_u32(expr->literal, NULL);
            write_instruction(&bc->bytecode, OP_CONSTANTW, debug_line);
            writew(&bc->bytecode, literal);
        } else if (expr->lit_type == LIT_IDENT) {
            write_instruction(&bc->bytecode, bc->flags == BCF_STORE_IDENT ? OP_STBPW : OP_LDBPW,
                              debug_line);
            writei(&bc->bytecode, stack_vars_get(bc, expr->sym->name));
        } else {
            printf("Ast literal expr kind not handled\n");
        }
    } break;
    };
}

static void ast_stmt_to_bytecode(BytecodeCompiler *bc, AstStmt *head)
{
    debug_line = head->line;
    switch (head->kind) {
    default:
        printf("Ast stmt %d not handled\n", head->kind);
        break;
    case STMT_ASSIGNMENT: {
        AstAssignment *assignment = AS_ASSIGNMENT(head);
        ast_expr_to_bytecode(bc, assignment->right);
        bc->flags = BCF_STORE_IDENT;
        ast_expr_to_bytecode(bc, assignment->left);
        bc->flags = BCF_LOAD_IDENT;
    } break;
    case STMT_IF: {
        AstIf *if_ = AS_IF(head);
        u32 endif_target;
        ast_expr_to_bytecode(bc, if_->condition);
        /* If false, jump to the else branch */
        u32 else_target = write_instruction(&bc->bytecode, OP_BIZ, head->line);
        writei(&bc->bytecode, 0);
        /* If branch */
        ast_stmt_to_bytecode(bc, if_->then);
        /* Skip the else branch */
        if (if_->else_) {
            endif_target = write_instruction(&bc->bytecode, OP_CONSTANTW, head->line);
            writew(&bc->bytecode, 0);
            write_instruction(&bc->bytecode, OP_JMP, head->line);
        }
        /* Else branch */
        patchi(&bc->bytecode, else_target,
               bc->bytecode.code_offset - else_target - sizeof(BytecodeImm));
        if (if_->else_) {
            ast_stmt_to_bytecode(bc, if_->else_);
            /* Path the jump to end target */
            patchw(&bc->bytecode, endif_target, bc->bytecode.code_offset);
        }

    } break;
    case STMT_WHILE: {
        AstWhile *while_ = AS_WHILE(head);
        u32 condition_target = bc->bytecode.code_offset;
        ast_expr_to_bytecode(bc, while_->condition);
        /* If condition is zero, skip body */
        u32 end_target = write_instruction(&bc->bytecode, OP_BIZ, head->line);
        writei(&bc->bytecode, 0);
        /* Loop body */
        ast_stmt_to_bytecode(bc, while_->body);
        /* Jump back to the condition */
        write_instruction(&bc->bytecode, OP_CONSTANTW, head->line);
        writew(&bc->bytecode, (BytecodeWord)condition_target);
        write_instruction(&bc->bytecode, OP_JMP, head->line);
        /* Patch the skip body jump */
        patchi(&bc->bytecode, end_target,
               bc->bytecode.code_offset - end_target - sizeof(BytecodeImm));
    } break;
    case STMT_BLOCK: {
        AstBlock *block = AS_BLOCK(head);
        bool new_vars = block->symt_local->sym_len > 0;
        BytecodeImm var_space_in_words = 0;
        if (new_vars) {
            var_space_in_words = new_stack_vars_from_block(bc, block->symt_local);
            // stack_vars_print(bc->stack_vars);
            write_instruction(&bc->bytecode, OP_PUSHNW, head->line);
            writei(&bc->bytecode, var_space_in_words);
        }

        for (AstListNode *n = block->stmts->head; n != NULL; n = n->next) {
            ast_stmt_to_bytecode(bc, (AstStmt *)n->this);
        }

        if (new_vars) {
            write_instruction(&bc->bytecode, OP_POPNW, head->line);
            writei(&bc->bytecode, var_space_in_words);
            /* Delete stack vars from current block */
            StackVars *old = bc->stack_vars;
            bc->stack_vars = old->parent;
            hashmap_free(&old->map);
            free(old);
        }
    } break;
    case STMT_PRINT: {
        AstList *stmt = AS_LIST(head);
        u8 n_args = 0;
        for (AstListNode *n = stmt->head; n != NULL; n = n->next) {
            n_args++;
            ast_expr_to_bytecode(bc, (AstExpr *)n->this);
        }
        write_instruction(&bc->bytecode, OP_PRINT, head->line);
        write_instruction(&bc->bytecode, n_args, head->line);
    } break;
    case STMT_RETURN: {
        // NOTE: right we force returns to have values, later we may want to change this
        AstSingle *stmt = AS_SINGLE(head);
        ast_expr_to_bytecode(bc, (AstExpr *)stmt->node);
        /* Now, the return value is on the stack. We need to store it in the retun value slot. */
        BytecodeImm return_slot_offset = stack_vars_get(bc, return_var_internal_name);
        write_instruction(&bc->bytecode, OP_STBPW, debug_line);
        writei(&bc->bytecode, return_slot_offset);
        write_instruction(&bc->bytecode, OP_RET, (s64)-1);
    } break;
    }
}

void ast_func_to_bytecode(BytecodeCompiler *bc, AstFunc *func, bool is_main)
{
    assert(func->body != NULL);
    Symbol *sym = get_sym_by_name(&bc->symt_root, func->name);
    assert(sym != NULL && sym->kind == SYMBOL_FUNC && sym->type_info != NULL);

    TypeInfoFunc *func_type = (TypeInfoFunc *)sym->type_info;
    if (func_type->is_comptime) {
        return;
    }

    func_register_start(bc, func->name);
    bc->stack_vars = make_stack_vars(NULL);
    bc->bp_stack_offset = 0;

    // NOTE: "Temporary" stack alignment, see new_stack_vars_from_block()

    /* Space for function parameters */
    s64 params_space = 0;
    SymbolTable params = sym->symt_local;
    for (u32 i = 0; i < params.sym_len; i++) {
        Symbol *param = params.symbols[i];
        params_space += type_info_byte_size(param->type_info);
        params_space = (s64)align_forward(params_space, sizeof(BytecodeWord));
    }
    // *2 because space for return address and the old bp itself
    s64 stack_space_before_bp = sizeof(BytecodeWord) * 2;
    stack_space_before_bp += params_space;
    stack_space_before_bp += bytes_to_words(type_info_byte_size(func_type->return_type));
    stack_space_before_bp = (s64)align_forward(stack_space_before_bp, sizeof(BytecodeWord));

    /*
     * Determine bp-relative offset of return value and arguments
     */
    s64 current_bp_offset = -stack_space_before_bp;
    stack_vars_set(bc->stack_vars, return_var_internal_name, current_bp_offset);
    current_bp_offset += type_info_byte_size(func_type->return_type);
    current_bp_offset = (s64)align_forward(current_bp_offset, sizeof(BytecodeWord));
    for (u32 i = 0; i < params.sym_len; i++) {
        Symbol *param = params.symbols[i];
        stack_vars_set(bc->stack_vars, param->name, current_bp_offset);
        current_bp_offset += type_info_byte_size(param->type_info);
        current_bp_offset = (s64)align_forward(current_bp_offset, sizeof(BytecodeWord));
    }
    //stack_vars_print(bc->stack_vars);

    /* Function prologue instruction : push bp, set bp = sp */
    write_instruction(&bc->bytecode, OP_FUNCPRO, (s64)-1);
    ast_stmt_to_bytecode(bc, func->body);

    /* Function epilogue */
    if (is_main) {
        write_instruction(&bc->bytecode, OP_EXIT, (s64)-1);
    } else {
        write_instruction(&bc->bytecode, OP_RET, (s64)-1);
    }

    free_all_stack_vars(bc->stack_vars);
    bc->stack_vars = NULL;
}


Bytecode ast_call_to_bytecode(SymbolTable symt_root, AstRoot *root, AstCall *call)
{
    // NOTE: hardcoded for @eval() case

    // Figure out which functions we need to generate
    // Generate all relevant functions

    // Assume comptime call is eval, so hardcode it to call the first parameter
    // of the call

    BytecodeCompiler bytecode_compiler;
    bytecode_compiler_init(&bytecode_compiler, symt_root);

    AstExpr *expr = (AstExpr *)call->args->head->this;
    ast_expr_to_bytecode(&bytecode_compiler, expr);
    write_instruction(&bytecode_compiler.bytecode, OP_EXIT, (s64)-1);

    /* Patch calls */
    for (u32 i = 0; i < bytecode_compiler.calls_to_patch; i++) {
        PatchCall patch = bytecode_compiler.patches[i];
        u32 actual = func_get_start(&bytecode_compiler, patch.func_name);
        patchw(&bytecode_compiler.bytecode, patch.offset, (BytecodeWord)actual);
    }

    bytecode_compiler_free(&bytecode_compiler);
    return bytecode_compiler.bytecode;
}

Bytecode ast_root_to_bytecode(SymbolTable symt_root, AstRoot *root)
{
    assert(root->funcs.head != NULL);
    return_var_internal_name = STR8_LIT("__RETURN__VAR__");
    BytecodeCompiler bytecode_compiler;
    bytecode_compiler_init(&bytecode_compiler, symt_root);

    /* TODO: Make space for global varibales */

    /* Generate main function */
    if (root->main_function == NULL) {
        fprintf(stderr, "no main function, panic!"); // TODO: better error handling
    } else {
        // NOTE: right now we assume the main function takes no arguments
        ast_func_to_bytecode(&bytecode_compiler, root->main_function, true);
    }

    /* Generate all other functions */
    for (AstListNode *node = root->funcs.head; node != NULL; node = node->next) {
        AstFunc *func = AS_FUNC(node->this);
        if (!STR8_EQUAL(func->name, STR8_LIT("main"))) {
            ast_func_to_bytecode(&bytecode_compiler, func, false);
        }
    }

    /* Patch calls */
    for (u32 i = 0; i < bytecode_compiler.calls_to_patch; i++) {
        PatchCall patch = bytecode_compiler.patches[i];
        u32 actual = func_get_start(&bytecode_compiler, patch.func_name);
        patchw(&bytecode_compiler.bytecode, patch.offset, (BytecodeWord)actual);
    }


    bytecode_compiler_free(&bytecode_compiler);
    return bytecode_compiler.bytecode;
}
