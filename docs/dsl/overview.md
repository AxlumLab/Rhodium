<!--
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
-->
# DSL overview

`rhodium.lua` is the dependency manifest for a Rhodium workspace. If it is absent, Rhodium also accepts the legacy `tpkg.lua`. If both exist, only `rhodium.lua` is loaded; an invalid `rhodium.lua` reports an error rather than falling back. DSL fields and meanings are unchanged.

The DSL is Lua-based. Treat the manifest as trusted project code.

## Minimal structure

```lua
package("MyProject")
version("0.1.0")

default_config("debug")
default_platform("host")
default_arch("x64")

require("fmt", {
    source = "https://github.com/fmtlib/fmt.git",
    ref = "10.2.1",
    build = "cmake",
    artifacts = {
        include_dirs = {"include"},
        libs = {"fmt"}
    }
})
```

## Stability policy

- `package`, `version`, `require`, `require_local`, `include`, and `artifacts` are stable user-facing concepts.
- Platform overlays and custom commands are useful but should be treated as advanced features.
- Internal cache layout and generated artifact IDs are implementation details.

## Evaluation model

Rhodium loads the manifest, builds an in-memory dependency model, validates it, resolves sources, restores artifacts, and then generates CMake integration files.
