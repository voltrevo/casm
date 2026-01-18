(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $get_five (result i32)
    i32.const 5
    return
  )

  (func $main (result i32)
    i32.const 0
    call $get_five
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
