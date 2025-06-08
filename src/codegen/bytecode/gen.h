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
#ifndef GEN_BYTECODE_H
#define GEN_BYTECODE_H

#include "ast.h"
#include "base.h"
#include "codegen/bytecode/bytecode.h"
#include "type.h"

Bytecode ast_call_to_bytecode(SymbolTable symt_root, AstRoot *root, AstCall *call);
Bytecode ast_root_to_bytecode(SymbolTable symt_root, AstRoot *root);
void disassemble(Bytecode b, Str8 source);

// Bytecode fib_test(void);

#endif /* GEN_BYTECODE_H */
