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

#include "base/types.h"
#include "lex.h"
#include "ast.h"
#include "base/str.h"
#include "log.h"

/*
 * One global logger
 * - mostly for debug stuff
 *
 * For global logs we want:
 * - where: __LINE__ and __POS__ in source code that triggered log
 * - rough description
 *
 * One local logger per source file
 *  - for errors, warnings, and debug
 *
 * For local logs we want:
 * - where
 * - what / kind
 * - nice description
 *
 *   Example:
 *
 *   src/test.mg | Error in function 'foo':
 *     >foo := 10 + "lol"
 *             ~~~~~~~~~
 *   Type mismtach. Binary operator '+' not defined for u32 and Str.
 */


Logger global_logger = { LOG_ERROR, true };


void logger_init(LogLevel log_level)
{
    global_logger.log_level = log_level;
}


void logger_set_source(char *source_file_name, char *source_code)
{
    global_logger.source_code = source_code;
    global_logger.source_file_name = source_file_name;
}

void log_msg(LogLevel log_level, Str8 msg);
