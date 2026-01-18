(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $process (param $s i32) (result i32)
    local.get $s
    i32.const 2
    i32.mul
    i32.const 1
    i32.add
    return
  )

  (func $main (result i32) (local $result i32)
    i32.const 5
    call $process
    local.set $result
    i32.const 0
    local.get $result
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
