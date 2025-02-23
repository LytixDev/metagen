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
#include "compiler/comptime/vm.h"
#include "compiler/comptime/bytecode.h"
#include <assert.h>
#include <stdio.h>


static BytecodeWord nextw(MetagenVM *vm)
{
    BytecodeWord value = *(BytecodeWord *)vm->ip;
    vm->ip += sizeof(BytecodeWord);
    return value;
}

static BytecodeImm nexti(MetagenVM *vm)
{
    BytecodeImm value = *(BytecodeImm *)vm->ip;
    vm->ip += sizeof(BytecodeImm);
    return value;
}

static u8 next_u8(MetagenVM *vm)
{
    u8 value = *vm->ip;
    vm->ip++;
    return value;
}

/* Stack operations */
static void pushn(MetagenVM *vm, BytecodeImm n)
{
    // TODO: zero init?
    vm->sp += sizeof(BytecodeWord) * n;
}

static void pushw(MetagenVM *vm, BytecodeWord value)
{
    *(BytecodeWord *)vm->sp = value;
    vm->sp += sizeof(BytecodeWord);
}

static BytecodeWord popw(MetagenVM *vm)
{
    vm->sp -= sizeof(BytecodeWord);
    return *(BytecodeWord *)vm->sp;
}

static BytecodeWord popn(MetagenVM *vm, BytecodeImm n)
{
    vm->sp -= sizeof(BytecodeWord) * n;
    return *vm->sp;
}

static BytecodeWord loadw(MetagenVM *vm, BytecodeImm bp_offset)
{
    return vm->stack[vm->bp + bp_offset];
}

static void storew(MetagenVM *vm, BytecodeImm bp_offset, BytecodeWord value)
{
    vm->stack[vm->bp + bp_offset] = value;
}

u32 run(Bytecode bytecode)
{
    MetagenVM vm;
    vm.b = bytecode;
    vm.ip = bytecode.code;
    vm.sp = (u8 *)vm.stack;
    // vm.bp = 0;
    // vm.flags = 0;

    while (1) {
        // printf(">%d %s\n", vm.ip - bytecode.code, op_code_str_map[*vm.ip]);
        OpCode instruction;
        switch (instruction = *vm.ip++) {
        /* Arithmetic */
        case OP_ADDW:
            pushw(&vm, popw(&vm) + popw(&vm));
            break;
        case OP_SUBW:
            pushw(&vm, popw(&vm) - popw(&vm));
            break;
        case OP_MULW:
            pushw(&vm, popw(&vm) * popw(&vm));
            break;
        case OP_DIVW:
            pushw(&vm, popw(&vm) / popw(&vm));
            break;
        case OP_LSHIFT:
            pushw(&vm, popw(&vm) << popw(&vm));
            break;
        case OP_RSHIFT:
            pushw(&vm, popw(&vm) >> popw(&vm));
            break;
        case OP_GE:
            pushw(&vm, popw(&vm) > popw(&vm));
            break;
        case OP_LE:
            pushw(&vm, popw(&vm) < popw(&vm));
            break;
        case OP_NOT:
            pushw(&vm, !popw(&vm));
            break;

        /* Jumps and branches */
        case OP_JMP:
            vm.ip = bytecode.code + popw(&vm);
            break;
        case OP_BIZ: {
            BytecodeImm target = nexti(&vm);
            if (popw(&vm) == 0) {
                vm.ip += target;
            }
        } break;
        case OP_BNZ: {
            BytecodeImm target = nexti(&vm);
            if (popw(&vm) != 0) {
                vm.ip += target;
            }
        } break;
        case OP_RET: {
            goto vm_loop_done;
        }

        /* Stack manipulation */
        case OP_CONSTANTW: {
            BytecodeWord value = nextw(&vm);
            pushw(&vm, value);
        }; break;
        case OP_PUSHNW: {
            BytecodeImm n_words = nexti(&vm);
            pushn(&vm, n_words);
        } break;
        case OP_POPNW: {
            BytecodeImm n_words = nexti(&vm);
            popn(&vm, n_words);
        } break;
        case OP_LDBP: {
            BytecodeImm bp_offset = nexti(&vm);
            pushw(&vm, loadw(&vm, bp_offset));
        } break;
        case OP_STBP: {
            BytecodeImm bp_offset = nexti(&vm);
            BytecodeWord value = popw(&vm);
            storew(&vm, bp_offset, value);
        } break;

        /* Higher-level language support */
        case OP_PRINT: {
            u8 n_args = next_u8(&vm);
            /* Must first pop unto an array to maintain correct printing order */
            BytecodeWord args[n_args];
            for (u8 i = 0; i < n_args; i++) {
                BytecodeWord value = popw(&vm);
                args[i] = value;
            }
            for (u8 i = n_args; i > 0; i--) {
                printf("%ld ", args[i - 1]);
            }
            printf("\n");
        } break;

        default:
            printf("Unknown opcode %d\n", instruction);
            goto vm_loop_done;
        }
    }

vm_loop_done:
    // final = stack_pop(&vm);
    // printf("%d\n", final);
    return 0;
}
