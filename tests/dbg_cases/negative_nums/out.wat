(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $main (result i32) (local $neg i32) (local $pos i32)
    i32.const 100
    i32.const 0
    i32.sub
    local.set $neg
    i32.const 100
    local.set $pos
    i32.const 0
    i32.const 43
    call $debug_begin
    local.get $neg
    call $debug_value_i32
    local.get $pos
    call $debug_value_i32
    local.get $neg
    local.get $pos
    i32.add
    call $debug_value_i32
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:5:4: neg = %, pos = %, expr(+) = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
