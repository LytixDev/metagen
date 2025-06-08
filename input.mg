//struct Pair := a: s32, b: s32
//
//func main(): s32
//begin
//    var pair: Pair
//    pair.a := 10
//    print pair.a
//end


func fib(n: s32): s32
begin
    if n = 0 then return 0
    if n = 1 then return 1
    return fib(n - 1) + fib(n - 2)
end

func main(): s32
begin
    var fib_20: s32

    fib_20 := @eval(fib(20))
    print fib_20

    return 0
end

// func zero(): s32
// begin
//     return 0
// end
// 
// 
// func fib(n: s32): s32
// begin
//     if n = 0 then return @eval(zero())
//     if n = 1 then return 1
//     return fib(n - 1) + fib(n - 2)
// end
// 
// func main(): s32
// begin
//     var fib_10: s32, fib_20: s32
//     fib_10 := @eval(fib(10))
//     fib_20 := @eval(fib(20))
//     print fib_10, fib_20
//     return 0
// end
