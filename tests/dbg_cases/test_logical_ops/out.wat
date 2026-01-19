(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $main (result i32) (local $b1 i32) (local $b2 i32) (local $b3 i32) (local $b4 i32) (local $b5 i32) (local $b6 i32)
    i32.const 1
    local.set $b1
    i32.const 0
    local.set $b2
    local.get $b1
    local.get $b2
    i32.and
    local.set $b3
    local.get $b1
    local.get $b2
    i32.or
    local.set $b4
    local.get $b1
    i32.eqz
    local.set $b5
    local.get $b2
    i32.eqz
    local.set $b6
    i32.const 0
    i32.const 20
    call $debug_begin
    local.get $b3
    call $debug_value_bool
    call $debug_end
    i32.const 20
    i32.const 20
    call $debug_begin
    local.get $b4
    call $debug_value_bool
    call $debug_end
    i32.const 40
    i32.const 21
    call $debug_begin
    local.get $b5
    call $debug_value_bool
    call $debug_end
    i32.const 61
    i32.const 21
    call $debug_begin
    local.get $b6
    call $debug_value_bool
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:8:4: b3 = %" "test.csm:9:4: b4 = %" "test.csm:10:4: b5 = %" "test.csm:11:4: b6 = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
