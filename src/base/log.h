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
#ifndef LOG_H
#define LOG_H

#include "base/str.h"

#include <stdarg.h>

typedef enum {
    LOG_ERR = 0,
    LOG_WARN = 1,
    LOG_ALL = 2 /* Debug mode */
} LogLevel;


typedef struct logger_t {
    LogLevel log_level;
    // TODO: Also store the logged messages??
} Logger;


/* Global logging */
void log_init_global(LogLevel log_level);

#define LOG_WARN(fmt, ...) log_bad_event(LOG_WARN, "WARNING", (fmt), ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...) log_bad_event(LOG_ERR, "ERROR", (fmt), ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...) log_bad_event(LOG_ERR, "FATAL", (fmt), ##__VA_ARGS__)
#define LOG_WARN_NOARG(msg) log_bad_event(LOG_WARN, "WARNING", "%s", msg)
#define LOG_ERROR_NOARG(msg) log_bad_event(LOG_ERR, "ERROR", "%s", msg)
#define LOG_FATAL_NOARG(msg) log_bad_event(LOG_ERR, "FATAL", "%s", msg)
void log_bad_event(LogLevel level, char *prefix, char *fmt, ...);

#define LOG_DEBUG_NOARG(msg) log_debug(__FILE__, __LINE__, false, msg)
#define LOG_FIXME_NOARG(msg) log_debug(__FILE__, __LINE__, true, msg)
#define LOG_DEBUG(msg, ...) log_debug(__FILE__, __LINE__, false, (msg), ##__VA_ARGS__)
#define LOG_FIXME(msg, ...) log_debug(__FILE__, __LINE__, true, (msg), ##__VA_ARGS__)
void log_debug(char *file, size_t line, bool is_a_fixme, char *fmt, ...);
void log_debug_str8(Str8 msg, char *file, size_t line, bool is_a_fixme);

#endif /* LOG_H */
