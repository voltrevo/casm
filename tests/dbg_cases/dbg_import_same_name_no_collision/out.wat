(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $module_a_helper (param $x i32) (result i32)
    local.get $x
    i32.const 10
    i32.add
    return
  )

  (func $process_a (param $n i32) (result i32)
    local.get $n
    call $module_a_helper
    return
  )

  (func $module_b_helper (param $x i32) (result i32)
    local.get $x
    i32.const 5
    i32.mul
    return
  )

  (func $process_b (param $n i32) (result i32)
    local.get $n
    call $module_b_helper
    return
  )

  (func $main (result i32) (local $result_a i32) (local $result_b i32)
    i32.const 3
    call $process_a
    local.set $result_a
    i32.const 3
    call $process_b
    local.set $result_b
    i32.const 0
    i32.const 27
    call $debug_begin
    local.get $result_a
    call $debug_value_i32
    call $debug_end
    i32.const 27
    i32.const 27
    call $debug_begin
    local.get $result_b
    call $debug_value_i32
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:12:4: result_a = %" "test.csm:13:4: result_b = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
