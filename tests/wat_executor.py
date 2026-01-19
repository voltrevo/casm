#!/usr/bin/env python3
"""
WAT executor with debug output support using Python wasmtime bindings.

This executor loads a WAT module and implements host functions for debug output.
The debug_begin/debug_value_*/debug_end pattern is used to capture debug information.
"""

import sys
import os

try:
    from wasmtime import Engine, Module, Store, Linker, FuncType, ValType
except ImportError:
    print("Error: wasmtime module not found. Install with: pip install wasmtime", file=sys.stderr)
    sys.exit(1)


class DebugState:
    """Tracks debug state during WAT execution."""
    
    def __init__(self, memory_obj, store):
        self.memory = memory_obj
        self.store = store
        self.pattern = None
        self.pattern_len = None
        self.values = []  # List of values captured
        self.output = None
    
    def begin(self, pattern_ptr, pattern_len):
        """Initialize a debug sequence with a format pattern."""
        # Read the format string from memory
        if self.memory is None:
            raise RuntimeError("Memory not available for debug_begin")
        
        mem_data = self.memory.read(self.store, pattern_ptr, pattern_ptr + pattern_len)
        self.pattern = bytes(mem_data).decode('utf-8')
        self.pattern_len = pattern_len
        self.values = []
        self.output = None
    
    def add_value_i32(self, value):
        """Add an i32 value."""
        self.values.append((value, 'i32'))
    
    def add_value_i64(self, value):
        """Add an i64 value."""
        self.values.append((value, 'i64'))
    
    def add_value_u32(self, value):
        """Add an u32 value."""
        self.values.append((value, 'u32'))
    
    def add_value_u64(self, value):
        """Add an u64 value."""
        self.values.append((value, 'u64'))
    
    def add_value_bool(self, value):
        """Add a bool value."""
        self.values.append((value, 'bool'))
    
    def end(self):
        """Format and output the debug information."""
        if not self.pattern:
            return ""
        
        # Count the number of % placeholders in the pattern
        # %% is an escaped %, so we need to count only single % chars
        placeholder_count = 0
        i = 0
        while i < len(self.pattern):
            if self.pattern[i] == '%':
                if i + 1 < len(self.pattern) and self.pattern[i + 1] == '%':
                    # %% is an escaped %, skip both characters
                    i += 2
                else:
                    # Single % is a placeholder
                    placeholder_count += 1
                    i += 1
            else:
                i += 1
        
        # Validate that we have the right number of values
        if len(self.values) != placeholder_count:
            print(f"Error: format string has {placeholder_count} placeholders but got {len(self.values)} values",
                  file=sys.stderr)
            sys.exit(1)
        
        # Format the output by replacing each % (that's not %%) with the corresponding value
        output = ""
        value_idx = 0
        i = 0
        while i < len(self.pattern):
            if self.pattern[i] == '%':
                if i + 1 < len(self.pattern) and self.pattern[i + 1] == '%':
                    # %% becomes a single %
                    output += '%'
                    i += 2
                else:
                    # Single % is a placeholder to be replaced
                    if value_idx < len(self.values):
                        value, type_hint = self.values[value_idx]
                        if type_hint == 'bool':
                            formatted = "true" if value else "false"
                        else:
                            formatted = str(value)
                        output += formatted
                        value_idx += 1
                    i += 1
            else:
                output += self.pattern[i]
                i += 1
        
        # Add newline and return
        self.output = output + "\n"
        return self.output


def create_host_functions(store, memory_obj, debug_state):
    """Create host functions for debug output."""
    
    def debug_begin(*args):
        """void debug_begin(i32 pattern_ptr, i32 pattern_len)"""
        pattern_ptr, pattern_len = args[0], args[1]
        debug_state.begin(pattern_ptr, pattern_len)
        return None
    
    def debug_value_i32(*args):
        """void debug_value_i32(i32 value)"""
        value = args[0]
        debug_state.add_value_i32(value)
        return None
    
    def debug_value_i64(*args):
        """void debug_value_i64(i64 value)"""
        value = args[0]
        debug_state.add_value_i64(value)
        return None
    
    def debug_value_u32(*args):
        """void debug_value_u32(i32 value)"""
        value = args[0]
        debug_state.add_value_u32(value)
        return None
    
    def debug_value_u64(*args):
        """void debug_value_u64(i64 value)"""
        value = args[0]
        debug_state.add_value_u64(value)
        return None
    
    def debug_value_bool(*args):
        """void debug_value_bool(i32 value)"""
        value = args[0]
        debug_state.add_value_bool(value)
        return None
    
    def debug_end(*args):
        """void debug_end()"""
        output = debug_state.end()
        if output:
            sys.stdout.write(output)
            sys.stdout.flush()
        return None
    
    # Create function types
    debug_begin_type = FuncType([ValType.i32(), ValType.i32()], [])
    debug_value_i32_type = FuncType([ValType.i32()], [])
    debug_value_i64_type = FuncType([ValType.i64()], [])
    debug_value_u32_type = FuncType([ValType.i32()], [])
    debug_value_u64_type = FuncType([ValType.i64()], [])
    debug_value_bool_type = FuncType([ValType.i32()], [])
    debug_end_type = FuncType([], [])
    
    return {
        "debug_begin": (debug_begin_type, debug_begin),
        "debug_value_i32": (debug_value_i32_type, debug_value_i32),
        "debug_value_i64": (debug_value_i64_type, debug_value_i64),
        "debug_value_u32": (debug_value_u32_type, debug_value_u32),
        "debug_value_u64": (debug_value_u64_type, debug_value_u64),
        "debug_value_bool": (debug_value_bool_type, debug_value_bool),
        "debug_end": (debug_end_type, debug_end),
    }


def execute_wat(wat_file):
    """
    Execute a WAT file and capture debug output.
    
    Args:
        wat_file: Path to the WAT file to execute
    
    Returns:
        Exit code (0 on success)
    """
    
    if not os.path.exists(wat_file):
        print(f"Error: WAT file not found: {wat_file}", file=sys.stderr)
        return 1
    
    # Read the WAT file
    with open(wat_file, 'r') as f:
        wat_code = f.read()
    
    # Create engine
    engine = Engine()
    
    # Load module
    try:
        module = Module(engine, wat_code)
    except Exception as e:
        print(f"Error loading WAT module: {e}", file=sys.stderr)
        return 1
    
    # Create store for execution
    store = Store(engine)
    linker = Linker(engine)
    
    # Set up debug state (will be initialized when debug_begin is called)
    # We'll set memory later after instantiation
    debug_state = DebugState(None, store)
    
    # Add host functions
    host_funcs = create_host_functions(store, None, debug_state)
    
    for func_name, (func_type, func_impl) in host_funcs.items():
        linker.define_func("host", func_name, func_type, func_impl)
    
    # Instantiate the module
    try:
        instance = linker.instantiate(store, module)
    except Exception as e:
        print(f"Error instantiating module: {e}", file=sys.stderr)
        return 1
    
    # Now get memory after instantiation
    exports = instance.exports(store)
    memory_obj = exports.get("memory")
    if memory_obj is not None:
        debug_state.memory = memory_obj
    
    # Call the main function if it exists
    try:
        exports = instance.exports(store)
        main_func = exports.get("main")
        
        if main_func is None:
            print("Error: 'main' function not found in WAT module", file=sys.stderr)
            return 1
        
        result = main_func(store)
        return 0
    except Exception as e:
        print(f"Error calling main function: {e}", file=sys.stderr)
        import traceback
        traceback.print_exc()
        return 1


def main():
    if len(sys.argv) < 2:
        print("Usage: wat_executor.py <wat_file>", file=sys.stderr)
        sys.exit(1)
    
    wat_file = sys.argv[1]
    exit_code = execute_wat(wat_file)
    sys.exit(exit_code)


if __name__ == "__main__":
    main()
