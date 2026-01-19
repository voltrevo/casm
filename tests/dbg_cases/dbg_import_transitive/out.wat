(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $add (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.add
    return
  )

  (func $increment (param $x i32) (result i32)
    local.get $x
    i32.const 1
    call $add
    return
  )

  (func $double_value (param $x i32) (result i32)
    local.get $x
    local.get $x
    call $add
    return
  )

  (func $main (result i32) (local $x i32) (local $inc_x i32) (local $double_x i32)
    i32.const 5
    local.set $x
    local.get $x
    call $increment
    local.set $inc_x
    local.get $x
    call $double_value
    local.set $double_x
    i32.const 0
    i32.const 23
    call $debug_begin
    local.get $inc_x
    call $debug_value_i32
    call $debug_end
    i32.const 23
    i32.const 26
    call $debug_begin
    local.get $double_x
    call $debug_value_i32
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:8:4: inc_x = %" "test.csm:9:4: double_x = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
