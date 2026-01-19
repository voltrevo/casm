(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $square (param $x i32) (result i32)
    local.get $x
    local.get $x
    i32.mul
    return
  )

  (func $cube (param $x i32) (result i32)
    local.get $x
    local.get $x
    i32.mul
    local.get $x
    i32.mul
    return
  )

  (func $main (result i32) (local $x i32)
    i32.const 3
    local.set $x
    i32.const 0
    i32.const 26
    call $debug_begin
    local.get $x
    call $square
    call $debug_value_i32
    call $debug_end
    i32.const 26
    i32.const 24
    call $debug_begin
    local.get $x
    call $cube
    call $debug_value_i32
    call $debug_end
    i32.const 50
    i32.const 26
    call $debug_begin
    i32.const 2
    call $cube
    call $square
    call $debug_value_i32
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:6:4: square() = %" "test.csm:7:4: cube() = %" "test.csm:8:4: square() = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
