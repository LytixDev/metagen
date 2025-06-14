# Metagen

### Idea for a language that:
Lets you modify a typechecked AST of the source program at compile-time.

### How?
We can write functions that will be ran at compile-time. These functions will take in the preceding statement/declaration as input in AST-form and will return a modified AST.

### Structure:
```
[lex]  ->  [parse]  ->  [type & symbol gen]  -> [typecheck]  ->  ...  ->  [code gen]
                      ^                                     v
                      ^   Run compile time function calls   v
                      ^   <---<---<---<---<---<---<---<---  v
```
After typechecking, run all functions that are marked to be called at compile time. These functions may modify the AST. This means we may have new unresolved types, symbols and AST nodes. We may also have new function calls that should be ran at compile time. Therefore we must take the altered AST and go back to type & symbol generation. Once the AST reaches a steady-state, we can continue the compilation pipeline and eventually produce a native executable

### Why is this useful?
Extremely powerful **Metaprogramming** with no preprocessor or a separate macro/meta language. Metaprogramming would be done in the same language as everything else.

Examples:
#### OpenMP-style parallelism without the need for a seperate compiler extension:
```
@Parallel(4)
for (int i = 0; i < 1_000_00; i++) {
    // heavy computation, each iteration independent of the previous 
}
```
Parallel would be like any other function in the language. It takes an AST as input and returns a modified AST as output. Calling the function with the `@`-symbol marks the call for compile-time execution. The compiler will insert the preceding node in the AST (a for-loop in this case) as the input to the call, After the call, the compiler will replace the old for-loop AST node with whatever the function returned.

The function itself may look something like this (note that this is just made up syntax):
```
func Parallel(AstNode *node, int n_threads) -> AstNode * {
	// do work
}
```
#### Enum name generator
```
// creates the array: FruitNames = ["APPLE", "BANANA", "PEAR", "DRAGON_FRUIT" ]
@GenEnumNames(var_name = FruitNames)
enum Fruit {
    APPLE = 0,
    BANANA,
    PEAR,
    DRAGON_FRUIT,
}
```
#### JSON Serialization
```
// creates the functions: func UserToJson(User) -> String and JsonToUser(String) -> User
@JSONSerialize()
struct User {
    String name;
    int age;
    bool is_admin;
}
```

### Towards a prototype
- Will transpile to C
- Simple stack-based vm to run functions at compile-time.

### Status
- [x] Frontend
- [x] Type inference & Typecheck
- [x] C backend
- [x] Bytecode backend
- [ ] Compile-time execution
- [ ] AST modification at compile-time

### Language Features
The language is just a vehicle for experimentation. Syntax and language features, beside the main experiment (that being compile-time modification of a typechecked AST), isn't really that important.

| Feature | Frontend | C backend | Bytecode backend |
|----------|----------|----------|----------|
| If else | ✅ | ✅ | ✅  |
| Switch | ❌ | ❌ | ❌ |
| While loops | ✅ | ✅ | ✅ |
| For loops | ❌ | ❌ | ❌ |
| Foreach loops | ❌ | ❌ | ❌ |
| Structs | ✅ | ⏳ | ❌ |
| Enums | ✅ | ⏳ | ❌ |
| Strings | ✅ | ❌ | ❌ |
| Arrays | ✅ | ✅ | ✅  |
| Slices | ❌ | ❌ | ❌ |
| Functions | ✅ | ✅ | ✅   |
| Ints & Floats | ⏳ | ❌ | ❌ |
| Booleans | ❌ | ❌ | ❌ |
| Arithmetic | ✅  | ✅  | ✅  |

misc:
- more comparison operators (>, >=)
- solution for integral literal deduction
- modulo operators
- first-class functions
