(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $main (result i32) (local $x i32) (local $y i32)
    i32.const 10
    local.set $x
    i32.const 20
    local.set $y
    i32.const 0
    i32.const 66
    call $debug_begin
    local.get $x
    local.get $y
    i32.eq
    call $debug_value_bool
    local.get $x
    local.get $y
    i32.lt_s
    call $debug_value_bool
    local.get $x
    local.get $y
    i32.gt_s
    call $debug_value_bool
    local.get $x
    local.get $y
    i32.ne
    call $debug_value_bool
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:5:4: expr(==) = %, expr(<) = %, expr(>) = %, expr(!=) = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
