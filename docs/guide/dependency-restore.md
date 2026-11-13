# Dependency restore

`restore` is the main dependency preparation command.

```bash
rhodium restore [package] [options]
```

## Common usage

```bash
rhodium restore
rhodium restore --config release
rhodium restore --toolchain windows-msvc-x64
rhodium restore fmt
```

## Reproducible restore

Use the lockfile exactly:

```bash
rhodium restore --locked
```

`--locked` should not update `rhodium.lock.toml`.

## Build-only and export-only

Build a dependency workspace without exporting artifacts:

```bash
rhodium restore dxc --build-only
```

Export artifacts from an already-built workspace:

```bash
rhodium restore dxc --export-only
```

Force a clean dependency build workspace:

```bash
rhodium restore dxc --rebuild
```

## Restore outputs

Restore may update:

```text
.rhodium/packages/
.rhodium/build-packages/
.rhodium/artifacts/
rhodium.lock.toml
```
