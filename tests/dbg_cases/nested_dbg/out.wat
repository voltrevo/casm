(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $main (result i32) (local $x i32) (local $y i32)
    i32.const 10
    local.set $x
    i32.const 20
    local.set $y
    i32.const 0
    local.get $x
    call $__casm_dbg_i32
    i32.const 1
    local.get $y
    call $__casm_dbg_i32
    i32.const 0
    local.get $x
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
