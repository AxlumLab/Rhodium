# Rhodium

**English** | [简体中文](README.zh-CN.md)

**A C/C++ dependency manager for CMake projects.**

Rhodium fetches, builds, and caches third-party dependencies, records resolved source revisions in a lockfile, and generates CMake integration targets.

## Features

- **Lua manifests:** Declare Git, archive, local, or prebuilt dependencies in `rhodium.lua`.
- **Dependency builds:** CMake, Make, configure + Make, custom, header-only, and prebuilt modes.
- **Reproducibility:** Track resolved sources in `rhodium.lock.toml` and use `restore --locked`.
- **Toolchain awareness:** Discover compiler/SDK profiles and keep artifacts separated by build configuration.
- **CMake integration:** Export targets such as `rhodium::fmt` for normal `target_link_libraries` use.
- **Developer tools:** Local overrides, transitive dependency graphs, mirrors, package updates, and cache cleanup.

## Build from source

Requirements: CMake **3.25+**, a **C++20** compiler, Git, and [vcpkg](https://github.com/microsoft/vcpkg). Ninja is recommended.

```sh
git clone https://github.com/AxlumLab/Rhodium.git
cd Rhodium
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DRHODIUM_VCPKG_ROOT=/path/to/vcpkg -DRHODIUM_BUILD_TESTS=OFF
cmake --build build --config Release
```

Replace `/path/to/vcpkg` with the actual vcpkg root. Alternatively configure `CMAKE_TOOLCHAIN_FILE` or `VCPKG_ROOT`. On Windows, use an MSVC-enabled developer shell for Ninja builds. The default configuration builds a static `Rhodium::Core` library and the `rhodium` CLI (`rhodium.exe` on Windows). Add the CLI output directory to your `PATH`.

See [installation](docs/installation.md) and [embedding](docs/guide/embedding.md) for further options.

## Quick start

The repository includes a complete [CMake example](examples/basic-cmake/). Run these commands from the repository root after building Rhodium and placing the CLI on `PATH`.

### 1. Discover a toolchain

```sh
rhodium --root examples/basic-cmake sdk detect
rhodium --root examples/basic-cmake sdk list
```

### 2. Restore and generate

Replace `YOUR_TOOLCHAIN_ID` with an ID from `sdk list`.

```sh
rhodium --root examples/basic-cmake restore --config debug --toolchain YOUR_TOOLCHAIN_ID
rhodium --root examples/basic-cmake generate --config debug --toolchain YOUR_TOOLCHAIN_ID
```

### 3. Build the example

```sh
cmake -S examples/basic-cmake -B build/example -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/example
```

Use compatible compiler, architecture, and Debug/Release settings for the restored packages and the consuming project. The example manifest targets `x64` by default.

## Declaring dependencies

Create `rhodium.lua` in your CMake project:

```lua
package("MyApp")
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

After `rhodium restore` and `rhodium generate`, consume the generated CMake configuration:

```cmake
cmake_minimum_required(VERSION 3.25)
project(MyApp LANGUAGES CXX)

set(RHODIUM_DEPS_FILE "${CMAKE_SOURCE_DIR}/.rhodium/generated/cmake/rhodiumConfig.cmake")
if(NOT EXISTS "${RHODIUM_DEPS_FILE}")
    message(FATAL_ERROR "Run rhodium restore and rhodium generate first")
endif()
include("${RHODIUM_DEPS_FILE}")

add_executable(myapp src/main.cpp)
target_link_libraries(myapp PRIVATE rhodium::fmt)
```

## CLI at a glance

| Command | Purpose |
| --- | --- |
| `rhodium sdk detect` / `rhodium sdk list` | Discover toolchains |
| `rhodium restore` | Fetch, build, and export dependency artifacts |
| `rhodium restore --locked` | Restore using the current lockfile |
| `rhodium generate` | Generate imported CMake targets |
| `rhodium deps` / `rhodium tree` | Inspect dependencies |
| `rhodium update [package]` | Update resolved dependency state |
| `rhodium packages` / `rhodium packages gc` | Inspect and clean caches |

See the [full CLI reference](docs/reference/command-reference.md) for options. Some diagnostic commands, including `doctor`, are only registered in Debug builds.

## Embed as a C++20 library

Rhodium can be installed as a static or shared library. The CLI is optional when embedding it.

```cmake
find_package(Rhodium CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE Rhodium::Core)
```

`Rhodium::Core` is the SDK library; generated third-party package targets use the separate `rhodium::` namespace. See [Embedding Rhodium](docs/guide/embedding.md).

## Platform status and migration

| Platform | Status |
| --- | --- |
| Windows Linux and macOS | Primary tested development workflow |
| Android | Toolchain-profile support exists; |
| iOS | Planned, not yet implemented |

## Documentation

- [Documentation index](docs/INDEX.md), [Getting started](docs/getting-started.md), [Installation](docs/installation.md)
- [DSL overview](docs/dsl/overview.md), [Toolchains](docs/guide/toolchains.md), [Command reference](docs/reference/command-reference.md)
- [Architecture](docs/architecture/overview.md), [Examples](examples/basic-cmake/), [Changelog](CHANGELOG.md)

## Contributing and license

Contributions and issues are welcome. Read [CONTRIBUTING.md](CONTRIBUTING.md), or open a [GitHub Issue](https://github.com/AxlumLab/Rhodium/issues).

Licensed under the **MIT License**. See [LICENSE](LICENSE).
