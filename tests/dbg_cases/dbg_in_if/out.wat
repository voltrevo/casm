(module
  (func $main (result i32) (local $x i32)
    i32.const 10
    local.set $x
    local.get $x
    i32.const 5
    i32.gt_s
    if
      i32.const 0
      local.get $x
      call $__casm_dbg_i32
    else
      i32.const 0
      i32.const 0
      call $__casm_dbg_i32
    end
    i32.const 0
    return
  )
)
