# Package cache

Rhodium stores dependency state under `.rhodium/`.

## Main directories

```text
.rhodium/packages/          fetched dependency sources
.rhodium/build-packages/    dependency build workspaces
.rhodium/artifacts/         exported dependency artifacts
.rhodium/generated/cmake/   generated CMake integration files
.rhodium/toolchains/        detected toolchain cache
```

## Artifact identity

Artifact identity is derived from dependency source state and build-relevant configuration. Do not depend on cache directory names manually; consume generated CMake output instead.

## Inspect packages

```bash
rhodium packages
rhodium packages --verbose
```

## Garbage collect unused artifacts

Dry run:

```bash
rhodium packages gc
```

Apply:

```bash
rhodium packages gc --apply
```

Options:

```bash
rhodium packages gc --package fmt --builds --sources
```
