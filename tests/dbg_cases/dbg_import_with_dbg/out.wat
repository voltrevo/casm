(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $square (param $x i32) (result i32)
    local.get $x
    local.get $x
    i32.mul
    return
  )

  (func $cube (param $x i32) (result i32)
    local.get $x
    local.get $x
    i32.mul
    local.get $x
    i32.mul
    return
  )

  (func $main (result i32) (local $x i32)
    i32.const 3
    local.set $x
    i32.const 0
    local.get $x
    call $square
    call $__casm_dbg_i32
    i32.const 0
    local.get $x
    call $cube
    call $__casm_dbg_i32
    i32.const 0
    i32.const 2
    call $cube
    call $square
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
