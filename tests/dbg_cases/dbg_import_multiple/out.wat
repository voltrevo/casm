(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $add (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.add
    return
  )

  (func $multiply (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.mul
    return
  )

  (func $main (result i32) (local $sum i32) (local $product i32)
    i32.const 10
    i32.const 5
    call $add
    local.set $sum
    i32.const 3
    i32.const 4
    call $multiply
    local.set $product
    i32.const 0
    i32.const 21
    call $debug_begin
    local.get $sum
    call $debug_value_i32
    call $debug_end
    i32.const 21
    i32.const 25
    call $debug_begin
    local.get $product
    call $debug_value_i32
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:7:4: sum = %" "test.csm:8:4: product = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
