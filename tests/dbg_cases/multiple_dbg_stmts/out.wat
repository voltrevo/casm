(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $main (result i32) (local $x i32)
    i32.const 10
    local.set $x
    i32.const 0
    local.get $x
    call $__casm_dbg_i32
    local.get $x
    i32.const 5
    i32.add
    local.set $x
    i32.const 0
    local.get $x
    call $__casm_dbg_i32
    local.get $x
    i32.const 2
    i32.mul
    local.set $x
    i32.const 0
    local.get $x
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
