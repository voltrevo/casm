(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $add_i16 (param $a i32) (param $b i32) (result i32) (local $result i32)
    local.get $a
    local.get $b
    i32.add
    local.set $result
    local.get $result
    return
  )

  (func $main (result i32) (local $x i32) (local $y i32) (local $result i32)
    i32.const 10
    local.set $x
    i32.const 20
    local.set $y
    local.get $x
    local.get $y
    call $add_i16
    local.set $result
    i32.const 0
    i32.const 25
    call $debug_begin
    local.get $result
    call $debug_value_i32
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:10:4: result = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
