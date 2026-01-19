(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $main (result i32) (local $x i32) (local $b i32)
    i32.const 5
    local.set $x
    i32.const 1
    local.set $b
    i32.const 0
    i32.const 34
    call $debug_begin
    local.get $x
    i32.const 0
    i32.sub
    call $debug_value_i32
    local.get $b
    i32.eqz
    call $debug_value_bool
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:5:4: -expr = %, !expr = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
