(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $base_add (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.add
    return
  )

  (func $level1_add_one (param $x i32) (result i32)
    local.get $x
    i32.const 1
    call $base_add
    return
  )

  (func $level2_add_two (param $x i32) (result i32)
    local.get $x
    call $level1_add_one
    call $level1_add_one
    return
  )

  (func $main (result i32) (local $result i32)
    i32.const 5
    call $level2_add_two
    local.set $result
    i32.const 0
    i32.const 24
    call $debug_begin
    local.get $result
    call $debug_value_i32
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:6:4: result = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
