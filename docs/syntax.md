# Metagen Syntax
>This document is still WIP.

The idea is to have a very simple C-like syntax. This document is rough. Will fix later.

### Comments
We use // for comments

### Strings and chars
We use "" for strings and '' for chars. Strings support escape characters. We use `` for raw strings (these do not support escape characters).

### Numbers
As you'd expect. We will support binary and hex numbers as well using the 0b and 0x prefixes. The type of the number will be inferred.

### Variable declarations and assignment
Exactly the same as Odin, but we ommit multiple declarations and assignments for now. We will also ommit constants for now.

### For and while

for (EXPR; EXPR; EXPR;) STMT

while (EXPR) STMT

### If else

if (EXPR) STMT

else STMT

### Print

print(EXPR, EXPR, ...)


### Func, Struct, Enum
func(n: s32): s32 STMT

struct { n: s32, ... }

enum u8 {A, B, C}





TODO:
- Comments
- Strings
- Chars
- Declaration
- Assignment
- For
- While
- If
- Else
- Break, Continue
- Return



Backlog:
- Escape sequences
- Raw strings
- Constants
- Multiple declaration
- Multiple assignment
- Switch
