# Local overrides

Local overrides redirect dependency sources to local working directories.

## Command-line override

```bash
rhodium restore --override fmt=../fmt-fork
```

## Persistent local override

```bash
rhodium override set fmt ../fmt-fork
rhodium override list
rhodium override remove fmt
rhodium override clear
```

Overrides are local development state and should not be committed.

## Manifest override table

A manifest may also describe overrides when a project needs a controlled development layout.

```lua
dependency_overrides({
    fmt = "../fmt-fork"
})
```

Prefer command or local override files for day-to-day development so the main manifest remains reproducible.
