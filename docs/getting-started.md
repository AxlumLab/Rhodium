# Getting started

This guide creates a minimal CMake project that restores dependencies through Rhodium.

## 1. Create `rhodium.lua`

```lua
package("HelloRHODIUM")
version("0.1.0")
default_config("debug")
default_platform("host")
default_arch("x64")

require("fmt", {
    source = "https://github.com/fmtlib/fmt.git",
    ref = "10.2.1",
    build = "cmake",
    cmake = {
        options = {
            FMT_DOC = false,
            FMT_TEST = false
        }
    },
    artifacts = {
        include_dirs = {"include"},
        libs = {"fmt"}
    }
})
```

## 2. Detect toolchains

```bash
rhodium sdk detect
rhodium sdk list
```

Choose a profile ID shown by `sdk list`, for example `windows-msvc-x64`.

## 3. Restore dependencies

```bash
rhodium restore --config debug --toolchain windows-msvc-x64
```

This reads `rhodium.lua`, fetches dependency sources, builds dependencies if needed, writes or updates `tpkg.lock.toml`, and stores artifacts under `.rhodium/`.

## 4. Generate CMake integration

```bash
rhodium generate --config debug --toolchain windows-msvc-x64
```

The default output is:

```text
.rhodium/generated/cmake/
```

## 5. Consume from CMake

```cmake
cmake_minimum_required(VERSION 3.25)
project(HelloRHODIUM LANGUAGES CXX)

include("${CMAKE_SOURCE_DIR}/.rhodium/generated/cmake/rhodiumConfig.cmake")

add_executable(hello src/main.cpp)
target_link_libraries(hello PRIVATE fmt)
```

## 6. Keep the right files in Git

Commit:

```text
rhodium.lua
tpkg.lock.toml
CMakeLists.txt
```

Do not commit:

```text
.rhodium/
```

## Next steps

- [Basic workflow](guide/basic-workflow.md)
- [DSL overview](dsl/overview.md)
- [Command reference](reference/command-reference.md)
