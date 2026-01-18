(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $base_add (param $a i32) (param $b i32) (result i32)
    local.get $a
    local.get $b
    i32.add
    return
  )

  (func $level1_add_one (param $x i32) (result i32)
    local.get $x
    i32.const 1
    call $base_add
    return
  )

  (func $level2_add_two (param $x i32) (result i32)
    local.get $x
    call $level1_add_one
    call $level1_add_one
    return
  )

  (func $main (result i32) (local $result i32)
    i32.const 5
    call $level2_add_two
    local.set $result
    i32.const 0
    local.get $result
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
