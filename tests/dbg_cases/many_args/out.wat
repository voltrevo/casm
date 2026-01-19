(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $main (result i32) (local $a i32) (local $b i32) (local $c i32) (local $d i32) (local $e i32) (local $f i32) (local $g i32) (local $h i32)
    i32.const 1
    local.set $a
    i32.const 2
    local.set $b
    i32.const 3
    local.set $c
    i32.const 4
    local.set $d
    i32.const 5
    local.set $e
    i32.const 6
    local.set $f
    i32.const 7
    local.set $g
    i32.const 8
    local.set $h
    i32.const 0
    i32.const 69
    call $debug_begin
    local.get $a
    call $debug_value_i32
    local.get $b
    call $debug_value_i32
    local.get $c
    call $debug_value_i32
    local.get $d
    call $debug_value_i32
    local.get $e
    call $debug_value_i32
    local.get $f
    call $debug_value_i32
    local.get $g
    call $debug_value_i32
    local.get $h
    call $debug_value_i32
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:11:4: a = %, b = %, c = %, d = %, e = %, f = %, g = %, h = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
