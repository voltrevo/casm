(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $main (result i32) (local $neg i32) (local $pos i32)
    i32.const 100
    i32.const 0
    i32.sub
    local.set $neg
    i32.const 100
    local.set $pos
    i32.const 0
    local.get $neg
    call $__casm_dbg_i32
    i32.const 1
    local.get $pos
    call $__casm_dbg_i32
    i32.const 2
    local.get $neg
    local.get $pos
    i32.add
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
