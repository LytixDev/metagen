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
#include <getopt.h>
#include <stdio.h>
#include <sys/stat.h>

#include "compiler/ast.h"
#include "compiler/codegen/gen.h"
#include "compiler/compiler.h"
#include "compiler/comptime/bytecode.h"
#include "compiler/comptime/vm.h"
#include "compiler/error.h"
#include "compiler/parser.h"
#include "compiler/type.h"

#include "base/log.h"
#include "base/str.h"
#define NICC_IMPLEMENTATION
#include "base/nicc.h"
#define SAC_IMPLEMENTATION
#include "base/sac_single.h"


typedef struct {
    u32 log_level; /* 0: Only log erros. 1: Log warnings. 2: Log everything. */
    bool parse_only; /* Stops after parsing and prints the syntax tree */
    bool bytecode_backend; /* Emits bytecode instead of the default C backend */
    bool run_bytecode; /* Treat the input as bytecode and run it */
} MetagenOptions;

MetagenOptions options = { 0 };


typedef void (*CompilerPass)(Compiler *c, AstRoot *root);

bool run_compiler_pass(Compiler *c, AstRoot *root, CompilerPass pass, char *name)
{
    LOG_DEBUG("Running compiler pass '%s'", name);
    m_arena_clear(c->pass_arena);
    pass(c, root);
    for (CompilerError *err = c->e->head; err != NULL; err = err->next) {
        printf("%s\n", err->msg.str);
    }
    return c->e->n_errors > 0;
}

u32 compile(char *file_name, char *input)
{
    Arena lex_arena;
    Arena persist_arena; /* Data which should persist throughout the lifetime of the compiler */
    Arena pass_arena; /* Data which that needs to persist for the lifetime of a compiler pass */
    m_arena_init_dynamic(&lex_arena, 1, 512);
    m_arena_init_dynamic(&persist_arena, 2, 512);
    m_arena_init_dynamic(&pass_arena, 1, 512);

    ErrorHandler e;
    error_handler_init(&e, input, file_name);

    Compiler compiler = { .persist_arena = &persist_arena, .pass_arena = &pass_arena, .e = &e };
    arraylist_init(&compiler.struct_types, sizeof(TypeInfoStruct *));
    arraylist_init(&compiler.all_types, sizeof(TypeInfo *));

    /* Frontend */
    AstRoot *ast_root = parse(&persist_arena, &lex_arena, &e, input);
    for (CompilerError *err = e.head; err != NULL; err = err->next) {
        printf("%s\n", err->msg.str);
    }
    LOG_DEBUG("Parsing complete, %d errors", e.n_errors);
    if (e.n_errors != 0) {
        goto done;
    }
    if (options.parse_only) {
        ast_print((AstNode *)ast_root, 0);
        putchar('\n');
        goto done;
    }

    /* Middle end */
    if (run_compiler_pass(&compiler, ast_root, typegen, "typegen")) {
        goto done;
    }
    if (run_compiler_pass(&compiler, ast_root, infer, "type infer")) {
        goto done;
    }
    if (run_compiler_pass(&compiler, ast_root, typecheck, "typecheck")) {
        goto done;
    }

    /* Backend */
    if (options.bytecode_backend && options.run_bytecode) {
        LOG_DEBUG_NOARG("Generating bytecode");
        Bytecode bytecode = ast_to_bytecode(compiler.symt_root, ast_root);
        disassemble(bytecode);
        run(bytecode);
    } else {
        LOG_DEBUG_NOARG("Generating c-code");
        transpile_to_c(&compiler);
        LOG_DEBUG_NOARG("Compiling c-code");
        system("gcc out.c");
        LOG_DEBUG_NOARG("Executing c-code");
        system("./a.out");
    }

done:
    for (CompilerError *err = e.head; err != NULL; err = err->next) {
        LOG_ERROR("%s", err->msg.str);
    }
    // We could be "good citizens" and release the memory here, but the OS is going to do it
    // anyways on the process terminating, so it doesn't really make a difference.
    // arraylist_free ...
    error_handler_release(&e);
    m_arena_release(&persist_arena);
    m_arena_release(&lex_arena);
    return e.n_errors;
}

int main(int argc, char *argv[])
{
    /*
     * Arg parse :
     * option range description
     * -l     0-2   Log level
     * -p           Parse only. Prints the syntax tree.
     * -b           Emits bytecode instead of the default C backend.
     * -r           Treat the input as bytecode and run it. If used with -b then it runs the
     *              bytecode directly.
     */
    int opt;
    while ((opt = getopt(argc, argv, "l:pbr")) != -1) {
        switch (opt) {
        case 'l': {
            int log_level = atoi(optarg);
            if (log_level < 0 || log_level > 2) {
                fprintf(stderr, "Error: Log level must be between 0 and 2.\n");
                return EXIT_FAILURE;
            }
            options.log_level = log_level;
        } break;
        case 'p':
            options.parse_only = true;
            break;
        case 'b':
            options.bytecode_backend = true;
            break;
        case 'r':
            options.run_bytecode = true;
            break;
        case '?':
            fprintf(stderr, "Error: Bad usage.\n");
            return EXIT_FAILURE;
        }
    }

    /* Set up global logger */
    log_init_global((LogLevel)options.log_level);

    /* Check if there are any remaining input arguments (used as the input file) */
    char *input_file;
    if (optind < argc) {
        input_file = argv[optind];
    } else {
        LOG_FATAL_NOARG("No input file specified");
        return EXIT_FAILURE;
    }

    /* Read input file in its entirety */
    struct stat st;
    if (stat(input_file, &st) != 0) {
        LOG_FATAL("Could not find file '%s'", input_file);
        return EXIT_FAILURE;
    }
    size_t input_size = st.st_size;
    char *input = malloc(sizeof(char) * (input_size + 1));
    input[input_size] = 0;
    FILE *fp = fopen(input_file, "r");
    if (fp == NULL) {
        LOG_FATAL("Could not open file '%s'", input_file);
        free(input);
        fclose(fp);
        return EXIT_FAILURE;
    }
    if (fread(input, sizeof(char), st.st_size, fp) != input_size) {
        LOG_FATAL("Could not read file '%s'", input_file);
        free(input);
        fclose(fp);
        return EXIT_FAILURE;
    }
    fclose(fp);

    u32 n_errors = compile(input_file, input);
    free(input);
    if (n_errors == 0) {
        return 0;
    }
    return 1;
}
