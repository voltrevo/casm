(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $main (result i32) (local $x i32) (local $y i32) (local $a i32) (local $b i32) (local $c i32) (local $d i32) (local $e i32)
    i32.const 5
    local.set $x
    i32.const 3
    local.set $y
    local.get $x
    local.get $y
    i32.add
    local.set $a
    local.get $x
    local.get $y
    i32.sub
    local.set $b
    local.get $x
    local.get $y
    i32.mul
    local.set $c
    local.get $x
    local.get $y
    i32.div_s
    local.set $d
    local.get $x
    local.get $y
    i32.rem_s
    local.set $e
    i32.const 0
    i32.const 19
    call $debug_begin
    local.get $a
    call $debug_value_i32
    call $debug_end
    i32.const 19
    i32.const 20
    call $debug_begin
    local.get $b
    call $debug_value_i32
    call $debug_end
    i32.const 39
    i32.const 20
    call $debug_begin
    local.get $c
    call $debug_value_i32
    call $debug_end
    i32.const 59
    i32.const 20
    call $debug_begin
    local.get $d
    call $debug_value_i32
    call $debug_end
    i32.const 79
    i32.const 20
    call $debug_begin
    local.get $e
    call $debug_value_i32
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:9:4: a = %" "test.csm:10:4: b = %" "test.csm:11:4: c = %" "test.csm:12:4: d = %" "test.csm:13:4: e = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
