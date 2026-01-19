(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $main (result i32) (local $i i32) (local $squared i32)
    i32.const 0
    local.set $i
    i32.const 0
    local.set $i
    block $break
    loop $continue
      local.get $i
      i32.const 3
      i32.lt_s
      i32.eqz
      br_if $break
      local.get $i
      local.get $i
      i32.mul
      local.set $squared
      i32.const 0
      i32.const 32
      call $debug_begin
      local.get $i
      call $debug_value_i32
      local.get $squared
      call $debug_value_i32
      call $debug_end
      local.get $i
      i32.const 1
      i32.add
      local.set $i
      br $continue
    end
    end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:6:8: i = %, squared = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
