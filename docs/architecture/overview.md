# Architecture overview

Rhodium is organized around a narrow responsibility: restore dependencies for CMake projects and generate CMake integration files.

## Main flow

```text
rhodium.lua
  -> script loader
  -> build model
  -> dependency resolver
  -> source fetch/cache
  -> package builder
  -> artifact store
  -> lockfile
  -> generated CMake files
```

## Module responsibilities

| Module | Responsibility |
| --- | --- |
| `cli/` | Command registration and command-level orchestration |
| `script/` | Lua runtime, DSL binding, manifest loading |
| `model/` | Plain dependency/project data structures and validation |
| `resolve/` | Dependency graph resolution and restore orchestration |
| `package/` | Source cache, package builders, artifact store, artifact cleanup |
| `toolchain/` | Toolchain profile detection, serialization, registry |
| `cmake/` | Generated CMake dependency integration |
| `config/` | TOML/JSON config, lockfile, local overrides |
| `core/` | Filesystem, process, path, environment, logging helpers |
| `diagnostics/` | Console/JSON diagnostics |
| `utils/` | Low-level utility code |

## Design boundaries

- `model/` should stay pure data plus validation.
- `cli/` should not contain dependency restore internals.
- `script/` should not fetch, build, or export packages.
- `resolve/` should orchestrate dependency stages, not implement platform toolchain detection.
- `package/` should build and materialize artifacts, not parse CLI options.
