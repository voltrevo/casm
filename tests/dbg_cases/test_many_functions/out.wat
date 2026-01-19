(module
  (import "host" "debug_begin" (func $debug_begin (param i32 i32)))
  (import "host" "debug_value_i32" (func $debug_value_i32 (param i32)))
  (import "host" "debug_value_i64" (func $debug_value_i64 (param i64)))
  (import "host" "debug_value_u32" (func $debug_value_u32 (param i32)))
  (import "host" "debug_value_u64" (func $debug_value_u64 (param i64)))
  (import "host" "debug_value_bool" (func $debug_value_bool (param i32)))
  (import "host" "debug_end" (func $debug_end))
  (memory 1)
  (func $func1 (result i32)
    i32.const 1
    return
  )

  (func $func2 (result i32)
    i32.const 2
    return
  )

  (func $func3 (result i32)
    i32.const 3
    return
  )

  (func $func4 (result i32)
    i32.const 4
    return
  )

  (func $func5 (result i32)
    i32.const 5
    return
  )

  (func $func6 (result i32)
    i32.const 6
    return
  )

  (func $func7 (result i32)
    i32.const 7
    return
  )

  (func $func8 (result i32)
    i32.const 8
    return
  )

  (func $func9 (result i32)
    i32.const 9
    return
  )

  (func $func10 (result i32)
    i32.const 10
    return
  )

  (func $func11 (result i32)
    i32.const 11
    return
  )

  (func $func12 (result i32)
    i32.const 12
    return
  )

  (func $func13 (result i32)
    i32.const 13
    return
  )

  (func $func14 (result i32)
    i32.const 14
    return
  )

  (func $func15 (result i32)
    i32.const 15
    return
  )

  (func $main (result i32)
    i32.const 0
    i32.const 26
    call $debug_begin
    call $func1
    call $debug_value_i32
    call $debug_end
    i32.const 26
    i32.const 27
    call $debug_begin
    call $func15
    call $debug_value_i32
    call $debug_end
    i32.const 0
    return
  )
  (data (i32.const 0) "test.csm:18:4: func1() = %" "test.csm:19:4: func15() = %")
  (export "memory" (memory 0))
  (export "main" (func $main))
)
