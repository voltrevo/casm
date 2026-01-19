(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $main (result i32) (local $a i32) (local $b i32) (local $c i32) (local $d i32)
    i32.const 100
    local.set $a
    local.get $a
    local.set $b
    local.get $a
    local.set $c
    local.get $a
    local.set $d
    i32.const 0
    i32.const 19
    call $debug_begin
    local.get $a
    call $debug_value_u32
    call $debug_end
    i32.const 19
    i32.const 19
    call $debug_begin
    local.get $b
    call $debug_value_u32
    call $debug_end
    i32.const 38
    i32.const 19
    call $debug_begin
    local.get $c
    call $debug_value_u32
    call $debug_end
    i32.const 57
    i32.const 19
    call $debug_begin
    local.get $d
    call $debug_value_u32
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:6:4: a = %" "test.csm:7:4: b = %" "test.csm:8:4: c = %" "test.csm:9:4: d = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
