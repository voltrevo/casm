(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $main (result i32) (local $t i32) (local $f i32)
    i32.const 1
    local.set $t
    i32.const 0
    local.set $f
    i32.const 0
    local.get $t
    call $__casm_dbg_i32
    i32.const 0
    local.get $f
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
