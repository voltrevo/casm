(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $main (result i32) (local $z i32) (local $x i32)
    i32.const 0
    local.set $z
    i32.const 10
    local.set $x
    i32.const 0
    local.get $z
    call $__casm_dbg_i32
    i32.const 1
    local.get $x
    local.get $z
    i32.mul
    call $__casm_dbg_i32
    i32.const 2
    local.get $z
    local.get $x
    i32.add
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
