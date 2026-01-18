(module
  (import "env" "_casm_dbg_i32" (func $__casm_dbg_i32 (param i32 i32)))
  (func $main (result i32) (local $x i32) (local $y i32)
    i32.const 5
    local.set $x
    i32.const 3
    local.set $y
    i32.const 0
    local.get $x
    local.get $y
    i32.add
    call $__casm_dbg_i32
    i32.const 0
    local.get $x
    local.get $y
    i32.mul
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
