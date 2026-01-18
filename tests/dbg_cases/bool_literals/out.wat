(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $main (result i32)
    i32.const 0
    i32.const 1
    call $__casm_dbg_i32
    i32.const 1
    i32.const 0
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
