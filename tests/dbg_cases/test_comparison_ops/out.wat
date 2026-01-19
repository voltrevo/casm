(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $main (result i32) (local $x i32) (local $y i32) (local $b1 i32) (local $b2 i32) (local $b3 i32) (local $b4 i32) (local $b5 i32) (local $b6 i32)
    i32.const 10
    local.set $x
    i32.const 5
    local.set $y
    local.get $x
    local.get $y
    i32.gt_s
    local.set $b1
    local.get $x
    local.get $y
    i32.lt_s
    local.set $b2
    local.get $x
    local.get $y
    i32.eq
    local.set $b3
    local.get $x
    local.get $y
    i32.ne
    local.set $b4
    local.get $x
    local.get $y
    i32.ge_s
    local.set $b5
    local.get $x
    local.get $y
    i32.le_s
    local.set $b6
    i32.const 0
    i32.const 21
    call $debug_begin
    local.get $b1
    call $debug_value_bool
    call $debug_end
    i32.const 21
    i32.const 21
    call $debug_begin
    local.get $b2
    call $debug_value_bool
    call $debug_end
    i32.const 42
    i32.const 21
    call $debug_begin
    local.get $b3
    call $debug_value_bool
    call $debug_end
    i32.const 63
    i32.const 21
    call $debug_begin
    local.get $b4
    call $debug_value_bool
    call $debug_end
    i32.const 84
    i32.const 21
    call $debug_begin
    local.get $b5
    call $debug_value_bool
    call $debug_end
    i32.const 105
    i32.const 21
    call $debug_begin
    local.get $b6
    call $debug_value_bool
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:10:4: b1 = %" "test.csm:11:4: b2 = %" "test.csm:12:4: b3 = %" "test.csm:13:4: b4 = %" "test.csm:14:4: b5 = %" "test.csm:15:4: b6 = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
