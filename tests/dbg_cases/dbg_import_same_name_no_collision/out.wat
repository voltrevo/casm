(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $helper (param $x i32) (result i32)
    local.get $x
    i32.const 10
    i32.add
    return
  )

  (func $process_a (param $n i32) (result i32)
    local.get $n
    call $helper
    return
  )

  (func $helper (param $x i32) (result i32)
    local.get $x
    i32.const 5
    i32.mul
    return
  )

  (func $process_b (param $n i32) (result i32)
    local.get $n
    call $helper
    return
  )

  (func $main (result i32) (local $result_a i32) (local $result_b i32)
    i32.const 3
    call $process_a
    local.set $result_a
    i32.const 3
    call $process_b
    local.set $result_b
    i32.const 0
    local.get $result_a
    call $__casm_dbg_i32
    i32.const 0
    local.get $result_b
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
