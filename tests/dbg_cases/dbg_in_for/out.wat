(module
  (func $main (result i32) (local $i i32) (local $squared i32)
    i32.const 0
    local.set $i
    i32.const 0
    local.set $i
    block $break
    loop $continue
      local.get $i
      i32.const 3
      i32.lt_s
      i32.eqz
      br_if $break
      local.get $i
      local.get $i
      i32.mul
      local.set $squared
      i32.const 0
      local.get $i
      call $__casm_dbg_i32
      i32.const 1
      local.get $squared
      call $__casm_dbg_i32
      local.get $i
      i32.const 1
      i32.add
      local.set $i
      br $continue
    end
    end
    i32.const 0
    return
  )
)
