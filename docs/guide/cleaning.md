# Cleaning

Use targeted cleanup first.

## Clean generated integration files

```bash
rhodium clean
```

## Clean one dependency

```bash
rhodium clean fmt --artifacts --sources
```

## Remove the lockfile

```bash
rhodium clean --lock
```

Only do this when you intentionally want to resolve dependencies again.

## Remove all local state

```bash
rhodium clean --all
```

This removes `.rhodium/` and should be treated as a full local reset.
