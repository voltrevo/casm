(module
  (import "env" "_casm_dbg_i32" (func $__casm_dbg_i32 (param i32 i32)))
  (func $main (result i32) (local $x i32)
    i32.const 5
    local.set $x
    i32.const 0
    local.get $x
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
