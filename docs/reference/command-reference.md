# Command reference

## Global options

```bash
rhodium [--root <path>] <command> [options]
```

| Option | Meaning |
| --- | --- |
| `--root <path>` | Workspace root containing `rhodium.lua` |
| `--help` | Show help |

## `doctor`

Check the local Rhodium runtime.

```bash
rhodium doctor
rhodium doctor --sdk
```

`--sdk` prints detailed SDK and toolchain diagnostics.

## `load`

Load and validate `rhodium.lua`.

```bash
rhodium load
```

Use this before restore when checking DSL syntax and model validation.

## `dump-model`

Print the loaded dependency model as JSON.

```bash
rhodium dump-model
```

## `deps`

Print dependencies declared in the manifest.

```bash
rhodium deps
```

## `generate`

Generate CMake dependency integration files.

```bash
rhodium generate [options]
```

| Option | Meaning |
| --- | --- |
| `--toolchain <id>` | Preferred local toolchain profile ID |
| `--config <debug|release>` | Build config |
| `--output <dir>` | Output directory; default `<active-state>/generated/cmake` (`.rhodium/`, or existing legacy `.tpkg/`) |
| `--override <name=path>` | Override dependency source |

## `restore`

Prepare package cache, build/export dependency artifacts, and update lock state.

```bash
rhodium restore [package] [options]
```

| Option | Meaning |
| --- | --- |
| `package` | Optional dependency name to restore only one dependency |
| `--toolchain <id>` | Preferred local toolchain profile ID |
| `--config <debug|release>` | Build config |
| `--override <name=path>` | Override dependency source |
| `--locked` | Use lockfile entries without updating the lockfile |
| `--build-only` | Build package workspace without exporting artifacts |
| `--export-only` | Export artifacts from an existing package build workspace |
| `--rebuild` | Remove the package build workspace before building |

## `update`

Update package lock entries.

```bash
rhodium update [package] [options]
```

| Option | Meaning |
| --- | --- |
| `package` | Optional package to update |
| `--toolchain <id>` | Preferred local toolchain profile ID |
| `--config <debug|release>` | Build config |
| `--override <name=path>` | Override dependency source |
| `--all` | Update all non-overridden packages |

## `packages`

Print restored package cache and artifact status.

```bash
rhodium packages
rhodium packages --verbose
```

| Option | Meaning |
| --- | --- |
| `--override <name=path>` | Override dependency source |
| `--verbose` | Print build hash and validation details |

### `packages gc`

Garbage collect unreferenced package caches.

```bash
rhodium packages gc [options]
```

| Option | Meaning |
| --- | --- |
| `--apply` | Delete unreferenced artifacts instead of dry run |
| `--package <name>` | Only consider artifacts for one package |
| `--builds` | Also delete stale package build caches |
| `--sources` | Also delete stale package source caches |
| `--toolchain <id>` | Toolchain profile ID for build cache retention |
| `--config <debug|release>` | Config for build cache retention |

## `tree`

Display dependency tree.

```bash
rhodium tree
```

## `clean`

Clean dependency cache and generated integration files.

```bash
rhodium clean [package] [options]
```

| Option | Meaning |
| --- | --- |
| `package` | Optional dependency to clean |
| `--all` | Remove the active state directory (`.rhodium` or legacy `.tpkg`) |
| `--artifacts` | Remove `.rhodium/artifacts` |
| `--sources` | Remove `.rhodium/packages` source cache |
| `--lock` | Remove `rhodium.lock.toml` |

## `override`

Manage local dependency overrides.

```bash
rhodium override list
rhodium override set <name> <path>
rhodium override remove <name>
rhodium override clear
```

## `sdk`

Detect and inspect local SDK/toolchains.

### `sdk detect`

```bash
rhodium sdk detect
```

Detect SDK/toolchains and write `.rhodium/toolchains/host.toml`.

### `sdk list`

```bash
rhodium sdk list [--refresh] [--source auto|project-user|workspace|global-user]
```

### `sdk show`

```bash
rhodium sdk show <id>
```

### `sdk add`

```bash
rhodium sdk add <id> [options]
```

Common options:

```text
--source project-user|global-user
--force
--platform <name>
--compiler-kind <name>
--sdk-kind <name>
--host-arch <arch>
--target-arch <arch>
--target-triple <triple>
--cc <path>
--cxx <path>
--linker <path>
--archiver <path>
--rc <path>
--mt <path>
--strip <path>
--ranlib <path>
--ninja <path>
--git <path>
--include <path>
--lib <path>
--bin <path>
--sysroot <path>
--android-sdk <path>
--android-ndk <path>
--android-api <level>
--android-abi <abi>
--jdk <path>
--adb <path>
--java <path>
--javac <path>
--gradle <path>
--env KEY=VALUE
```

### `sdk remove`

```bash
rhodium sdk remove <id> [--source project-user|global-user]
```

### Other SDK commands

```bash
rhodium sdk doctor
rhodium sdk dump
rhodium sdk select <id>
rhodium sdk clear
```
