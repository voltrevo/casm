(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $main (result i32) (local $a i32) (local $b i32)
    i32.const 17
    local.set $a
    i32.const 5
    local.set $b
    i32.const 0
    local.get $a
    local.get $b
    i32.rem_s
    call $__casm_dbg_i32
    i32.const 1
    local.get $b
    local.get $a
    i32.rem_s
    call $__casm_dbg_i32
    i32.const 2
    i32.const 0
    local.get $b
    i32.rem_s
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
