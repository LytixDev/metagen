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

#include "type.h"

typedef s64 BytecodeWord;
typedef s16 BytecodeQuarter;

/*
 * The instruction set for a super simple 64-bit stack-based bytecode language.
 * Every instruction is encoded as a single byte. Some instructions encode immediate data in the
 * instruction stream. Either a full BytecodeWord (64-bit), or BytecodeQuarter (16-bit).
 */

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
    OP_ADD, // pop a, pop b, push a + b
    OP_SUB, // pop a, pop b, push a - b
    OP_MUL, // pop a, pop b, push a * b
    OP_DIV, // pop a, pop b, push a / b
    OP_LSHIFT, // pop a, pop b, push a << b
    OP_RSHIFT, // pop a, pop b, push a >> b
    OP_GE, // pop a, pop b, push 1 if a >= b
    OP_LE, // pop a, pop b, push 1 if a <= b
    OP_NOT, // pop a, push !a

    /* Branching */
    // TODO: Think more clearly about semantics here
    OP_JMP, // pop a, set pc = a
    OP_BIZ, // read q, pop a, if a == 0 set pc += q
    OP_BNZ, // read q, pop a, if a != 0 set pc += q

    /* stack operations */
    OP_LI, // read w, push w
    OP_PUSHN, // read q, push q words (making space for q words on the stack)
    OP_POPN, // read q, pop q words (inverse of pushn)
    OP_LDBP, // read q, load bp + q as a, push a
    OP_STBP, // read q, pop a, store a at bp + q
    OP_LDA, // TODO
    OP_STA, // TODO

    OP_PRINT, // read b, pop b words, print popped words
    OP_CALL, // read w, push pc, set pc = w
    OP_FUNCPRO, // push bp, set bp = sp
    OP_RET, // set sp = bp, pop as a, set bp = a, pop as b, set pc = b
    OP_EXIT, // Halt the execution

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


#endif /* BYTECODE_H */
