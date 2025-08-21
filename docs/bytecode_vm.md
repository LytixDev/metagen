# On the Metagen Bytecode and VM 

Stack-based VM. No registers.

Goals:
- Simplicity
- Easy to extend
- Interop with compiler internals
- Move most of the complexity to the bytecode compiler.

## Bytecode:

Layout:
- Setup code for global variables
- Main function
- Every other function

NOTE: The VM currently only supports load and stores in 8-byte chunks (sizeof(BytecodeWord)). Therefore, every variable is aligned to the 8-byte boundary.


## VM:
The VM executes bytecode.

The VM consits of four mutable properties:
1. instruction pointer / program counter (pc)
2. stack pointer (sp)
3. base pointer (bp)
4. stack

### Execution
The VM executes one instruction at a time until it reaches the OP_EXIT instruction. The next instruction is determined by the pc. The pc is incremented upon reading an instruction, but can also be changed by branch and jump instructions.

### Function Prologue and Epilogue

Prologue:
Caller:
Make room on stack
- Space for return value (if present)
- Push arguments
- Push return address
- Call the function (jump to it)
Callee:
- Push base pointer
- Set base pointer = stack pointer
Caller:
- Clean up stack

Epilogue:
Callee:
- Write return value into reserved slot in the stack (if present)
- Restore stack pointer by setting it to the base pointer (deallocates space on the stack used by the function)
- Pop old base pointer
- Pop return address as PC
