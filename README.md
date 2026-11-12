# Rhodium

Repository: https://github.com/AxlumLab/Rhodium.git

Rhodium is a cross-platform C/C++ dependency management system designed to serve CMake projects.

It is **not** a replacement for CMake, Ninja, Visual Studio, Xcode, or any native build backend. Its job is to restore source, archive, local, or prebuilt dependencies; build dependency artifacts when needed; write a lock file for reproducibility; and generate CMake integration files that your normal CMake project can consume.

The executable is `rhodium` (`rhodium.exe` on Windows). Rhodium reads `rhodium.lua` first and falls back to `tpkg.lua`. State uses an existing `.rhodium/` first, otherwise an existing `.tpkg/`, otherwise a new `.rhodium/`. Lockfiles remain `tpkg.lock.toml`. Generated CMake packages and imported targets use `rhodium` and `rhodium::`.

## Project status

Rhodium is under active development.

Current validation scope:

- Desktop platforms are the current tested target.
- Windows desktop workflows are the most exercised path.
- Linux and macOS are intended cross-platform targets but should be validated per project.
- Android toolchain profiles are supported and will continue to be tested.
- iOS support is planned but not implemented yet.

## Core workflow

```bash
# Check the local environment
rhodium doctor

# Detect local C/C++ and platform toolchains
rhodium sdk detect
rhodium sdk list

# Restore dependency sources and artifacts
rhodium restore --config debug --toolchain windows-msvc-x64

# Generate CMake integration files
rhodium generate --config debug --toolchain windows-msvc-x64
```

A normal project keeps using CMake:

```cmake
include("${CMAKE_SOURCE_DIR}/.rhodium/generated/cmake/rhodiumConfig.cmake")
```

## Minimal `rhodium.lua`

```lua
package("ExampleApp")
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

## Library integration

Rhodium can also be installed as a static or shared C++20 library. Link the
`Rhodium::Core` target from `find_package(Rhodium CONFIG REQUIRED)`; the CLI
is optional. See [Embedding Rhodium](docs/guide/embedding.md) for build,
installation and host-program examples.

## Documentation

Start here:

- [Documentation index](docs/INDEX.md)
- [Getting started](docs/getting-started.md)
- [Basic workflow](docs/guide/basic-workflow.md)
- [DSL overview](docs/dsl/overview.md)
- [Command reference](docs/reference/command-reference.md)
- [Architecture overview](docs/architecture/overview.md)
- [Contributor guide](CONTRIBUTING.md)

## Repository layout

```text
src/rhodium/          Rhodium implementation
cmake/             CMake project glue
docs/              User, reference, architecture, and contributor docs
examples/          Example dependency manifests and project layouts
tests/             Unit and integration-style tests
scripts/           Local helper scripts
```

## License

This snapshot preserves the existing project copyright notice. See [LICENSE](LICENSE).
