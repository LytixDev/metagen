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

typedef s64 BytecodeWord;
typedef u16 BytecodeImm;

typedef enum {
    /* arithmetic */
    OP_ADDW,
    OP_SUBW,
    OP_MULW,
    OP_DIVW,
    OP_LSHIFT,
    OP_RSHIFT,

    /* Branching */
    OP_JMP, // pop and update ip
    OP_BIZ, // pop and jump to imm if popped value is zero
    OP_BNZ, // pop and jump to imm if popped value is not zero

    /* stack operations */
    OP_CONSW, // push next word
    OP_PUSHN, // make space for n words words
    OP_POPN, // remove space for n words on
    OP_LOADL, // push value at next imm + bp
    OP_STOREL, // pop and store value at next imm + bp

    OP_PRINT,
    OP_RETURN,

    OP_TYPE_LEN,
} OpCode;


extern char *op_code_str_map[OP_TYPE_LEN];


typedef struct {
    // TODO: Pool allocated or something
    u8 code[4096];
    u32 code_offset;
} Bytecode;

typedef struct locals_t Locals;
struct locals_t {
    HashMap map; // Key: identfier, Value: offset + 1 (so we can use 0x0 as NULL).
    Locals *parent;
};

typedef enum {
    BCF_STORE_IDENT = 1,
    BCF_LOAD_IDENT = 2,
} BytecodeCompilerFlags;

typedef struct {
    Bytecode bytecode;
    Locals *locals;
    BytecodeCompilerFlags flags;
} BytecodeCompiler;


Bytecode ast_to_bytecode(AstRoot *root);
void disassemble(Bytecode b);

Bytecode bytecode_test(void);
Bytecode fib_test(void);

#endif /* BYTECODE_H */
