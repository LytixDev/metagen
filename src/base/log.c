/*
 *  Copyright (C) 2025 Nicolai Brand (https://lytix.dev)
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

#include "log.h"
#include "base/str.h"

#include <stdio.h>

Logger global_logger = { LOG_WARN };

void log_init_global(LogLevel log_level)
{
    global_logger.log_level = log_level;
}

static void log_msg(FILE *out, char *prefix, char *fmt, va_list args)
{
    fprintf(out, "[%s] ", prefix);
    vfprintf(out, fmt, args);
    fprintf(out, "\n");
}

void log_bad_event(LogLevel level, char *prefix, char *fmt, ...)
{
    if (global_logger.log_level > level) {
        return;
    }
    va_list args;
    va_start(args, fmt);
    log_msg(stderr, prefix, fmt, args);
    va_end(args);
}

void log_debug(char *file, size_t line, bool is_a_fixme, char *fmt, ...)
{
    if (global_logger.log_level != LOG_ALL) {
        return;
    }

    va_list args;
    va_start(args, fmt);
    fprintf(stdout, "[%s] %s@%zu | ", is_a_fixme ? "FIXME" : "DEBUG", file, line);
    vfprintf(stdout, fmt, args);
    fprintf(stdout, "\n");
    va_end(args);
}

void log_debug_str8(Str8 msg, char *file, size_t line, bool is_a_fixme)
{
    if (global_logger.log_level != LOG_ALL) {
        return;
    }
    printf("[%s] %s:%zu | %s\n", is_a_fixme ? "FIXME" : "DEBUG", file, line, msg.str);
}
