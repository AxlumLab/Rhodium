# Lockfile

`rhodium.lock.toml` records resolved dependency state. New workspaces use this name. Existing `tpkg.lock.toml` files remain supported and are reused when `rhodium.lock.toml` is absent; when both exist, `rhodium.lock.toml` takes precedence. To migrate an existing project, rename its `tpkg.lock.toml` to `rhodium.lock.toml`.

## Commit policy

Commit `rhodium.lock.toml` for applications and projects that need reproducible dependency restore.

## Locked restore

```bash
rhodium restore --locked
```

A locked restore should use existing lock entries and should not update the lockfile.

## Update policy

Use `rhodium update` when intentionally changing lock state.

```bash
rhodium update fmt
rhodium restore fmt
```

## Do not hand-edit by default

Manual edits can make source state inconsistent with cache state. Prefer command workflows unless debugging.
