# Generated CMake

`rhodium generate` writes CMake integration files for restored dependency artifacts.

## Default output

```text
.rhodium/generated/cmake/
```

## Include policy

Include generated files from your root `CMakeLists.txt` after restore and generate.

```cmake
include("${CMAKE_SOURCE_DIR}/.rhodium/generated/cmake/rhodiumConfig.cmake")
```

## Regeneration

Regenerate after changing:

- Manifest dependency list.
- Artifact declarations.
- Toolchain or config.
- Lockfile state.
