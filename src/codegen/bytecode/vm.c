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
#include "codegen/bytecode/vm.h"
#include "codegen/bytecode/bytecode.h"
#include <assert.h>
#include <stdio.h>


static BytecodeWord nextw(MetagenVM *vm)
{
    BytecodeWord value = *(BytecodeWord *)vm->pc;
    vm->pc += sizeof(BytecodeWord);
    return value;
}

static BytecodeQuarter nextq(MetagenVM *vm)
{
    BytecodeQuarter value = *(BytecodeQuarter *)vm->pc;
    vm->pc += sizeof(BytecodeQuarter);
    return value;
}

static u8 next_u8(MetagenVM *vm)
{
    u8 value = *vm->pc;
    vm->pc++;
    return value;
}

/* Stack operations */
static void pushn(MetagenVM *vm, BytecodeQuarter n)
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

static BytecodeWord popn(MetagenVM *vm, BytecodeQuarter n)
{
    vm->sp -= sizeof(BytecodeWord) * n;
    return *vm->sp;
}

static BytecodeWord ldw(MetagenVM *vm, BytecodeWord byte_offset)
{
    // return vm->stack[offset];
    return *(BytecodeWord *)(vm->stack + byte_offset);
}

static void stw(MetagenVM *vm, BytecodeWord byte_offset, BytecodeWord value)
{
    // vm->stack[offset] = value;
    *(BytecodeWord *)(vm->stack + byte_offset) = value;
}

static void dump_stack(MetagenVM vm, OpCode instruction)
{
    printf("Step %zu : %s, bp : %zu (%zu)\n", vm.instructions_executed - 1,
           op_code_str_map[instruction], vm.bp, vm.bp / 8);
    for (s32 i = 0; i < (vm.sp - vm.ss); i++) {
        // printf("%02x ", vm.stack[i]);
        if ((i + 1) % 8 == 0) {
            u8 *chunk = &vm.stack[i - 7]; // Start of this 8-byte block
            s64 as_s64 = *(s64 *)chunk;
            s32 low = *(s32 *)chunk;
            s32 high = *(s32 *)(chunk + 4);
            // printf(" | s64: %lld | s32s: %d, %d", (long long)as_s64, low, high);
            printf("%d: %lld", i / 8, (long long)as_s64);
            printf("\n");
        }
    }
    printf("\n");
}

BytecodeWord run(Bytecode bytecode, bool debug)
{
    MetagenVM vm = { 0 };
    vm.b = bytecode;
    vm.pc = bytecode.code;
    vm.sp = (u8 *)vm.stack;
    vm.ss = vm.sp;
    vm.bp = 0;
    vm.instructions_executed = 0;
    // vm.flags = 0;

    while (1) {
        vm.instructions_executed++;

        // printf("ip:%d sp:%d, bp:%d %s\n", vm.ip - bytecode.code, vm.sp - vm.ss, vm.bp,
        // op_code_str_map[*vm.ip]);
        OpCode instruction;

        switch (instruction = *vm.pc++) {
        /* Arithmetic */
        case OP_ADD:
            pushw(&vm, popw(&vm) + popw(&vm));
            break;
        case OP_SUB:
            pushw(&vm, popw(&vm) - popw(&vm));
            break;
        case OP_MUL:
            pushw(&vm, popw(&vm) * popw(&vm));
            break;
        case OP_DIV:
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
            vm.pc = bytecode.code + popw(&vm);
            break;
        case OP_BIZ: {
            BytecodeQuarter target = nextq(&vm);
            if (popw(&vm) == 0) {
                vm.pc += target;
            }
        } break;
        case OP_BNZ: {
            BytecodeQuarter target = nextq(&vm);
            if (popw(&vm) != 0) {
                vm.pc += target;
            }
        } break;

        /* Memory manipulation */
        case OP_LI: {
            BytecodeWord value = nextw(&vm);
            pushw(&vm, value);
        }; break;
        case OP_PUSHN: {
            BytecodeQuarter n_words = nextq(&vm);
            pushn(&vm, n_words);
        } break;
        case OP_POPN: {
            BytecodeQuarter n_words = nextq(&vm);
            popn(&vm, n_words);
        } break;
        case OP_LDBP: {
            BytecodeQuarter bp_offset = nextq(&vm);
            pushw(&vm, ldw(&vm, vm.bp + bp_offset));
        } break;
        case OP_STBP: {
            BytecodeQuarter bp_offset = nextq(&vm);
            BytecodeWord value = popw(&vm);
            stw(&vm, vm.bp + bp_offset, value);
        } break;
        case OP_LDA: {
            BytecodeWord mem_pos = nextw(&vm);
            pushw(&vm, ldw(&vm, mem_pos));
        } break;
        case OP_STA: {
            BytecodeWord mem_pos = nextw(&vm);
            BytecodeWord value = popw(&vm);
            stw(&vm, mem_pos, value);
        } break;
        case OP_LDI: {
            BytecodeWord mem_pos = popw(&vm);
            BytecodeWord value = ldw(&vm, mem_pos);
            //pushw(&vm, ldw(&vm, mem_pos));
            pushw(&vm, value);
        } break;
        case OP_STI: {
            BytecodeWord mem_pos = popw(&vm);
            BytecodeWord value = popw(&vm);
            stw(&vm, mem_pos, value);
        } break;

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
        case OP_FUNCPRO:
            pushw(&vm, vm.bp);
            vm.bp = (BytecodeWord)(vm.sp - vm.ss);
            break;
        case OP_RET:
            vm.sp = (u8 *)(vm.bp + vm.ss);
            vm.bp = popw(&vm);
            vm.pc = (u8 *)popw(&vm);
            break;
        case OP_CALL: {
            BytecodeWord callee_offset = popw(&vm);
            pushw(&vm, (BytecodeWord)vm.pc);
            vm.pc = bytecode.code + callee_offset;
        } break;
        case OP_NOP:
            break;

        case OP_EXIT:
            goto vm_loop_done;
            break;

        default:
            printf("Unknown opcode %d\n", instruction);
            goto vm_loop_done;
        }


        if (debug) {
            dump_stack(vm, instruction);
        }
    }

    BytecodeWord final;

vm_loop_done:
    final = popw(&vm);
    return final;
}
