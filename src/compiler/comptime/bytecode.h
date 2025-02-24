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
#ifndef BYTECODE_H
#define BYTECODE_H


#include "base/nicc.h"
#include "base/types.h"
#include "compiler/ast.h"
#include "compiler/type.h"

typedef s64 BytecodeWord;
typedef s16 BytecodeImm; // Value immeditely preceeding certain instructions

/*
 * Function prologue:
 * - Push old base pointer (bp)
 * - Load current stack pointer into base pointer
 * - Allocate space for paramaters on the stack stack
 * - Fill the parameters with correct values
 * - Jump to function
 *
 * Function epilogue:
 * - Deallocate paramaters
 * - Load old base pointer
 * - Push return value (if present)
 */

typedef enum {
    /* arithmetic */
    OP_ADDW,
    OP_SUBW,
    OP_MULW,
    OP_DIVW,
    OP_LSHIFT,
    OP_RSHIFT,
    OP_GE, // pop a and pop b. Push 1 if a >= b
    OP_LE, // pop a and pop b. Push 1 if a <= b
    OP_NOT,

    /* Branching */
    OP_JMP, // pop and uppdate ip
    OP_BIZ, // pop and add imm to ip if popped value is zero
    OP_BNZ, // pop and add imm to ip if popped value is not zero

    /* stack operations */
    OP_CONSTANTW, // push next word
    OP_PUSHNW, // make space for n words
    OP_POPNW, // remove space for n words
    OP_LDBP, // bp-relative + immediate stack load
    OP_STBP, // bp-relative + immediate stack store
    OP_LDA, // stack load @ next word
    OP_STA, // stack store @ next word

    OP_PRINT,
    OP_CALL, // TODO
    OP_FUNCPRO, // TODO
    OP_RET, // TODO

    OP_TYPE_LEN,
} OpCode;


extern char *op_code_str_map[OP_TYPE_LEN];


typedef struct {
    // TODO: Pool allocated or something
    u8 code[4096];
    u32 code_offset;

    /* Debug */
    s64 source_lines[4094];
} Bytecode;

/*
 * Each function gets their own.
 * Each scope gets their own.
 */
typedef struct stack_vars_t StackVars;
struct stack_vars_t {
    HashMap map; // Key: Symbol name, Value: offset from bp
    StackVars *parent;
};
/*
 * For storing in the map, we ensure each value is larger than 0.
 * Otherwise, a bp_rel_offset of 0 would be treated as the NULL pointer.
 */
#define STACK_VAR_MAP_ADD (S16_MAX + 2)

typedef enum {
    BCF_STORE_IDENT = 1,
    BCF_LOAD_IDENT = 2,
} BytecodeCompilerFlags;

typedef struct {
    SymbolTable symt_root;
    Bytecode bytecode;

    StackVars *stack_vars;
    s64 bp_stack_offset;

    HashMap globals; // Key: Symbol name, Value: Absolute position in the stack
    HashMap functions; // Key: Symbol name, Value: Absolute position of first instruction in code

    BytecodeCompilerFlags flags;
} BytecodeCompiler;


Bytecode ast_to_bytecode(SymbolTable symt_root, AstRoot *root);
void disassemble(Bytecode b, Str8 source);

Bytecode fib_test(void);

#endif /* BYTECODE_H */
