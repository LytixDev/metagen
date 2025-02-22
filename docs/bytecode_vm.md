# On the Metagen Bytecode and VM 

Stack-based VM. No registers.

Goals:
- Simplicity
- Easy to extend
- Interop with compiler internals


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


TODO:
- Be able to see what ast node is responsible for generating certain bytecode instruction


var i: s32
i := 0
while i != 10 do
begin
    i := i + 1
    print i
end

0000 OP_PUSHN 4        ; 1: var i: s32
0003 OP_CONSW 0        ; 2: i := 0
0012 OP_STOREI 0       ; 2
0015 OP_CONSW 10       ; 3: while i != 10 do
0024 OP_LOADI 0        ; 3
0027 OP_SUBW           ; 3
0028 OP_BIZ 61         ; 3 
0031 OP_CONSW 1        ; 4: i := i + 1
0040 OP_LOADI 0        ; 4
0043 OP_ADDW           ; 4
0044 OP_STOREI 0       ; 4
0047 OP_LOADI 0        ; 5: print i
0050 OP_PRINT args 1   ; 5
0052 OP_CONSW 15       ; 3
0061 OP_JMP            ; 3
0062 OP_POPN 4         ; 1



- Support functions
- OP_EXIT
- Growing stack
- Global variables
- Fix hashmap, I think it leaks?


