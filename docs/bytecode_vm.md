# On the Metagen Bytecode and VM 

Stack-based VM. No registers.

Goals:
- Simplicity
- Easy to extend
- Interop with compiler internals

## Bytecode:

Layout:
- Setup of global variables
- Main function
- Every other function

Todo:
- Args
- Struct member padding
- Struct member access (load and store)
- Global variables



## VM:
The VM executes bytecode.

The VM consits of four mutable properties:
1. instruction pointer (ip)
2. stack pointer (sp)
3. base pointer (bp)
4. stack
5. flags

### Execution
The VM executes one instruction at a time until it reaches the OP_EXIT instruction. The next instruction is determined by the ip. The ip is incremented upon reading an instruction, but can also be changed by branch and jump instructions.

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




TODO:
- Global variables
- Entrypoint and OP_EXIT
- Support functions
- Dynamic stack


