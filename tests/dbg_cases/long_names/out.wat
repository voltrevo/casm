(module
  (func $__casm_dbg_i32 (param i32 i32))
  (func $main (result i32) (local $very_long_variable_name_that_goes_on_and_on i32)
    i32.const 42
    local.set $very_long_variable_name_that_goes_on_and_on
    i32.const 0
    local.get $very_long_variable_name_that_goes_on_and_on
    call $__casm_dbg_i32
    i32.const 0
    return
  )
)
