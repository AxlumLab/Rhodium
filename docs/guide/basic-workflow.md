<!--
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
-->
# Basic workflow

A normal Rhodium workflow has four steps.

## 1. Write the manifest

Create `rhodium.lua` in the CMake project root.

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

## 2. Detect or select a toolchain

```bash
rhodium sdk detect
rhodium sdk list
rhodium sdk select windows-msvc-x64
```

The selected toolchain is stored in local `.rhodium` state and should not be committed.

## 3. Restore dependencies

```bash
rhodium restore --config debug
```

This fetches sources, builds dependency packages if needed, exports artifacts, and updates `rhodium.lock.toml`.

## 4. Generate CMake files

```bash
rhodium generate --config debug
```

Then include the generated file from your CMake project.

## Recommended Git policy

Commit:

```text
rhodium.lua
rhodium.lock.toml
docs explaining required toolchains
```

Ignore:

```text
.rhodium/
```
