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

  (func $main (result i32) (local $sum i32) (local $product i32)
    i32.const 10
    i32.const 5
    call $add
    local.set $sum
    i32.const 3
    i32.const 4
    call $multiply
    local.set $product
    i32.const 0
    local.get $sum
    call $__casm_dbg_i32
    i32.const 0
    local.get $product
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
