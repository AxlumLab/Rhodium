# Installation

Rhodium is currently distributed as a development executable from this repository.

## Requirements

Required for normal use:

- Git.
- CMake.
- A C/C++ toolchain for the target platform.
- Ninja is recommended for dependency builds.

Required for building Rhodium itself:

- CMake 3.25 or newer.
- A C++20 compiler.
- vcpkg dependencies defined by `vcpkg.json`.

## Platform status

Rhodium is designed to be cross-platform as a dependency manager.

Current validation status:

| Platform | Status |
| --- | --- |
| Windows desktop | Tested development path |
| Linux desktop | Intended, requires project validation |
| macOS desktop | Intended, requires project validation |
| Android | Toolchain support exists, more validation needed |
| iOS | Planned, not implemented |

## Install from a local build

After building the executable, add the output directory to `PATH` or call it with an absolute path.

```bash
rhodium --help
rhodium doctor
```

## Project-local usage

For reproducible project workflows, prefer documenting the exact `rhodium` version or commit used by the project.
