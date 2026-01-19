(module
  (func $main (result i32) (local $x i32)
    i32.const 10
    local.set $x
    local.get $x
    i32.const 5
    i32.gt_s
    if
      i32.const 0
      i32.const 19
      call $debug_begin
      local.get $x
      call $debug_value_i32
      call $debug_end
    else
      i32.const 19
      i32.const 19
      call $debug_begin
      i32.const 0
      call $debug_value_i32
      call $debug_end
    end
    i32.const 0
    return
  )
  (export "main" (func $main))
)
