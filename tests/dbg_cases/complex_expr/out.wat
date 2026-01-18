(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $main (result i32) (local $x i32) (local $y i32) (local $z i32)
    i32.const 10
    local.set $x
    i32.const 5
    local.set $y
    i32.const 2
    local.set $z
    i32.const 0
    local.get $x
    local.get $y
    local.get $z
    i32.mul
    i32.add
    call $__casm_dbg_i32
    i32.const 1
    local.get $x
    local.get $y
    i32.sub
    local.get $y
    local.get $z
    i32.add
    i32.mul
    call $__casm_dbg_i32
    i32.const 2
    local.get $x
    local.get $y
    i32.mul
    local.get $z
    i32.div_s
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
