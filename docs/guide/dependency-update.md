# Dependency update

`update` changes lockfile entries for one package or all packages.

## Update one package

```bash
rhodium update fmt
rhodium restore fmt
```

## Update all packages

```bash
rhodium update --all
rhodium restore
```

## With toolchain and config

```bash
rhodium update fmt --config release --toolchain windows-msvc-x64
```

## Policy

Do not use update when you only want a clean rebuild. Use restore cleanup options instead.

Use update when you intentionally want a new resolved source state.
