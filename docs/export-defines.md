# export_defines Option

Control whether dependency macros are exported to your project.

## Overview

The `export_defines` option controls whether a dependency's preprocessor definitions (from `artifacts.defines`) are propagated to consuming projects via CMake's `INTERFACE_COMPILE_DEFINITIONS`.

**Default**: `false` (macros are NOT exported)

## Syntax

```lua
require("lib", {
    source = "...",
    export_defines = false  -- or true
})
```

## Behavior

### `export_defines = false` (Default)

The dependency's `INTERFACE_COMPILE_DEFINITIONS` will be **empty** in the generated CMake target. Only include directories and libraries are exported.

**Example**:
```lua
require("spdlog", {
    source = "https://github.com/gabime/spdlog.git",
    ref = "v1.12.0",
    export_defines = false  -- Default
})
```

Generated CMake target `rhodium::spdlog` will have:
- ✅ Include directories
- ✅ Libraries to link
- ❌ NO defines exported

### `export_defines = true`

The dependency's defines are exported and will be visible to all code that links against it.

**Example**:
```lua
require("my-internal-lib", {
    source = "../libs/my-lib",
    export_defines = true,
    artifacts = {
        defines = { "MY_LIB_VERSION=1", "MY_LIB_FEATURE=1" }
    }
})
```

Generated CMake target will include defines.

## Use Cases

### Prevent Macro Pollution

Third-party libraries often define macros that can conflict with your project:

```lua
require("spdlog", {
    source = "...",
    export_defines = false  -- Prevent SPDLOG_* macros from leaking
})

require("rapidjson", {
    source = "...",
    export_defines = false  -- Prevent RAPIDJSON_* macros from leaking
})
```

### Export Internal Library Macros

For your own libraries that require macros to be visible:

```lua
require("my-config-lib", {
    source = "../libs/config",
    export_defines = true,  -- Consumers need these macros
    artifacts = {
        defines = {
            "CONFIG_VERSION=2",
            "ENABLE_LOGGING"
        }
    }
})
```

## Example: ToolkitProject Migration

Before (with macro conflicts):
```lua
require("spdlog", {
    source = "https://github.com/gabime/spdlog.git"
})
-- Result: SPDLOG_* macros pollute your project
```

After (clean):
```lua
require("spdlog", {
    source = "https://github.com/gabime/spdlog.git",
    export_defines = false  -- Macros stay in spdlog
})
```

## Technical Details

### CMake Generation

**With `export_defines = false`**:
```cmake
add_library(rhodium::spdlog INTERFACE IMPORTED)
set_target_properties(rhodium::spdlog PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "/path/to/include"
    INTERFACE_COMPILE_DEFINITIONS ""  # Empty!
    INTERFACE_LINK_LIBRARIES "..."
)
```

**With `export_defines = true`**:
```cmake
add_library(rhodium::my-lib INTERFACE IMPORTED)
set_target_properties(rhodium::my-lib PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "/path/to/include"
    INTERFACE_COMPILE_DEFINITIONS "MY_MACRO=1;ANOTHER_MACRO"
    INTERFACE_LINK_LIBRARIES "..."
)
```

### Lock File

The `export_defines` value is stored in `rhodium.lock.toml`:

```toml
[[package]]
name = "spdlog"
export_defines = false
```

## Best Practices

### Default to `false`

For most third-party libraries, disable define export:

```lua
require("fmt", { export_defines = false })
require("spdlog", { export_defines = false })
require("rapidjson", { export_defines = false })
```

### Use `true` selectively

Only export defines from:
1. Your own internal libraries
2. Libraries specifically designed to export configuration macros
3. Header-only libraries that require defines for consumers

## Related

- [DSL Reference](dsl-reference.md) - Complete configuration syntax
- [Artifacts](dsl-reference.md#artifacts) - Artifact configuration

---

**Previous:** [← Features](FEATURES.md)
