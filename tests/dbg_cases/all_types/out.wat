(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $main (result i32) (local $a i32) (local $b i32) (local $c i32)
    i32.const 5
    local.set $a
    i32.const 1
    local.set $b
    i32.const 0
    local.set $c
    i32.const 0
    local.get $a
    call $__casm_dbg_i32
    i32.const 0
    local.get $b
    call $__casm_dbg_i32
    i32.const 0
    local.get $c
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
