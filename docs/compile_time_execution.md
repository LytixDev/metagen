# On compile time execution in Metagen

Metagen is a language that "lets you modify a typechecked AST of the source program at compile-time." What this means and why its intersting is a topic for another day. This document will describe how Metagen achieves compile time execution of typechecked AST nodes.

Note that all code shown in this document is pseudo code and does not parse.

## Idea

Many languages, like C, have textual macros: text in -> text out. Some languages, like Rust, support macros directly on the AST.

Like Rust procedural macros, Metagen compile time functions takes an AST node as input and produces an AST node as output. The AST nodes that Metagen compile time functions operate on is fully typechecked. Furthermore, these compile time functions are written exactly the same as any other function. This way we get meta-programming baked directly into the language - no macros needed.

Below are a few examples that illustrates this idea.


### GenEnumNames
Whenever I define an enum in C, I nearly always create a corresponding list of its names to help with debugging. This approach is error prone as every time I add a new enum member, remove one, or switch the order, I need to remember to update the accompanying list of names. The GenEnumNames function does this automatically.

```
enum Fruit {
    APPLE = 0,
    BANANA,
    PEAR,
    DRAGON_FRUIT,
}

// creates the array ["APPLE", "BANANA", "PEAR", "DRAGON_FRUIT" ]
fruit_names = @GenEnumNames(Fruit)
```
The '@'-sign is syntax to call the function GenEnumNames at compile time. GenEnumNames is a function which has an AST node as its first and only argument. It has one return value which is a new AST node.

The implementation of GenEnumNames is just like any other function in the language. There is really nothing special about it, only that it follows the compile time function interface (AST in -> AST out). Its implementation could look something like this:

```
// In this example, the parameter is passed as the name of the enum, which is a Type.
// Types are first-class. This way of passing a type, by directly writing its name, becomes a 
// Literal node.
func GenEnumNames(enum_type: AstLiteral) -> AstLiteral 
{
    if (enum_type.lit_type != LIT_TYPE) {
        // Error: Expected a type, but got ... instead
    }

    array_literal: AstLiteral
    array_literal.lit_type = LIT_ARRAY

    for each member of enum_type.type
        // get member name     
        // append member name to array_literal initialiser

    return array_literal
}
```


### Parallel
What if we could get parallelisation with very little effort? In c and c++ we can use OpenMP, which from my pov is a compiler extension that nicely wraps pthreads for you (alltough its quite a lot more).

```
@@Parallel(4)
for (int i = 0; i < 1_000_00; i++) {
    // heavy computation, each iteration independent of the previous 
}
```
Again, Parallel would be like any other function in the language. However, this way of calling it is a slighlty different than what we did earlier. Using the '@@' syntax, the compiler will automatically insert the preceding AST node as the **first** parameter to the function. The output of the Parallel function will automatically **replace** the preceding AST node. This means that the for loop we see is replaced by whater Parallel spits out.

### Eval
Eval will evalute its input at compile time using the compilers built in bytecode interpreter.

```
fib_20 = @Eval(fib(20))
```

A proof of concept of Eval already works.


### JSONSerialize
The JSONSerialize function is similar to Parallel in that the compiler will automatically insert the preceding AST node and automatically replace it with the functions output. JSONSerialize expects a function type, which the below node is, and uses it to deduce what function to create.
```
struct User {
    String name;
    int age;
    bool is_admin;
}

@@JSONSerialize()
func UserToJSON(User) -> JSON
```

## How it works
This is how compile time execution fits into the structure of the compiler.
```
[lex]  ->  [parse]  ->  [type & symbol gen]  -> [typecheck]  ->  ...  ->  [code gen]
                      ^                                     v
                      ^   Run compile time function calls   v
                      ^   <---<---<---<---<---<---<---<---  v
```
After typechecking, the compiler will run functions that are marked to be called at compile time. These functions may modify the AST. This means we may have new unresolved types, symbols and AST nodes. Furthermore, a compile time call may have resulted in creating new compile time calls. Therefore we must take the altered AST and go back to type & symbol generation. Once the AST reaches a steady-state, we can continue the compilation pipeline and eventually produce a native executable.


Questions to be solved:
1. What order are compile time functions ran in?
Once the dependency graph is computed, this should be quite simple. The question is computing the dependency graph. For instance, the Eval function will evalute an AST node at compile. Okay, easy, Eval depends on any potention compile-time calls that its parameters depend on. But this is a specific rule for Eval, which may not hold true for all functions. How do we generalize and make a sensible set of rules to compute the dependency graph?

2. How do we figure out the minimum set of "things" needed to compile a function?
May only be the functions that are called. If so, quite straight forward.

3. How do we statically figure out if a compile time call is impossible to resolve?
For instance if it depends on something which can not be known at compile time, or if two compile time calls call one another. What about global state?

4. What if a compile time function makes an OS specific call?

5. For the bytecode VM, how does it figure out what to return back to the compiler after a compile time call?
For expressions, it can return the result of the expression which size in bytes we know from its type. 

6. How can we reuse as much information as possible between typegen and type infer passes? 
Same can be said for compilation of functions. If 10 compile time calls are for the same functions, we should cache these.
