(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $add (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.add
    return
  )

  (func $main (result i32)
    i32.const 0
    i32.const 1
    i32.const 2
    call $add
    i32.const 3
    call $add
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
