(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $main (result i32) (local $max_val i32) (local $min_val i32) (local $zero i32)
    i32.const 2147483647
    local.set $max_val
    i32.const 2147483648
    i32.const 0
    i32.sub
    local.set $min_val
    i32.const 0
    local.set $zero
    i32.const 0
    i32.const 48
    call $debug_begin
    local.get $max_val
    call $debug_value_i32
    local.get $min_val
    call $debug_value_i32
    local.get $zero
    call $debug_value_i32
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:6:4: max_val = %, min_val = %, zero = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
