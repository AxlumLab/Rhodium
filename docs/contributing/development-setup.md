# Development setup

## Requirements

- CMake 3.25 or newer.
- A C++20 compiler.
- vcpkg dependencies from `vcpkg.json`.
- Git.
- Ninja is recommended.

## Configure

Use the existing project CMake presets or your local CMake invocation.

```bash
cmake -S . -B build -G Ninja
```

## Build

```bash
cmake --build build
```

## Local validation commands

```bash
rhodium doctor
rhodium load --root examples/basic-cmake
rhodium dump-model --root examples/basic-cmake
```

Automated agents should not run these commands unless explicitly instructed by the project owner.
