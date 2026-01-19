(module
  (func $func_void (result i32)
    i32.const 0
    return
  )

  (func $main (result i32)
    call $func_void
    i32.const 0
    return
  )
  (export "main" (func $main))
)
