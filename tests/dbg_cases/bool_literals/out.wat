(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $main (result i32)
    i32.const 0
    i32.const 33
    call $debug_begin
    i32.const 1
    call $debug_value_bool
    i32.const 0
    call $debug_value_bool
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:3:4: true = %, false = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
