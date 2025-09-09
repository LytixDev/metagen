/*
 *  Copyright (C) 2023-2025 Nicolai Brand (https://lytix.dev)
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

#include "base.h"
#include "ast_new.h"

// NOTE: Right now we allocate based on the max possible size an ast node can take.
//       A different approach that would use less memory is to allocate based on the memory 
//       the node actually needs.
AstNode *alloc_ast_node(Arena *arena, AstKind kind)
{
    AstNode *node = m_arena_alloc(arena, sizeof(AstNode));
    node->kind = kind;
    return node;
}
