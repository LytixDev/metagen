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
#ifndef VM_H
#define VM_H

#include "compiler/comptime/bytecode.h"

#define STACK_MAX 1024

typedef enum {
    VM_FLAG_NEG = 1 << 0,
    VM_FLAG_ZERO = 1 << 1,
} VMFlags;

typedef struct {
    Bytecode b;
    u8 *ip;

    BytecodeWord stack[STACK_MAX];
    u8 *sp;
    u8 bp;

    VMFlags flags;
} MetagenVM;


u32 run(Bytecode b);

#endif /* VM_H */
