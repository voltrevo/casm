(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $main (result i32) (local $max_val i32) (local $min_val i32) (local $zero i32)
    i32.const 2147483647
    local.set $max_val
    i32.const 2147483648
    i32.const 0
    i32.sub
    local.set $min_val
    i32.const 0
    local.set $zero
    i32.const 0
    local.get $max_val
    call $__casm_dbg_i32
    i32.const 1
    local.get $min_val
    call $__casm_dbg_i32
    i32.const 2
    local.get $zero
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
