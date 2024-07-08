/*
 *  Copyright (C) 2024 Nicolai Brand (lytix.dev)
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
#include "sac_single.h"
#include "types.h"
#include "str.h"


StrBuilder make_str_builder(Arena *arena) {
    StrBuilder sb = {
        .arena = arena,
        .str = (Str8){ .len = 0 },
        .cap = 16,
    };

    sb.str.str = m_arena_alloc(arena, sb.cap);
    return sb;
}

void str_builder_append_u8(StrBuilder *sb, u8 c)
{
    if (sb->str.len == sb->cap) {
        /* Doubled the allocation */
        (void)m_arena_alloc(sb->arena, sb->cap);
        sb->cap *= 2;
    }

    sb->str.str[sb->str.len++] = c;
}
