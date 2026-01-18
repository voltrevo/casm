(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $add (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.add
    return
  )

  (func $multiply (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.mul
    return
  )

  (func $increment (param $x i32) (result i32)
    local.get $x
    i32.const 1
    call $add
    return
  )

  (func $double_value (param $x i32) (result i32)
    local.get $x
    local.get $x
    call $add
    return
  )

  (func $main (result i32) (local $x i32) (local $inc_x i32) (local $double_x i32)
    i32.const 5
    local.set $x
    local.get $x
    call $increment
    local.set $inc_x
    local.get $x
    call $double_value
    local.set $double_x
    i32.const 0
    local.get $inc_x
    call $__casm_dbg_i32
    i32.const 0
    local.get $double_x
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
