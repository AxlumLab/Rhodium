# Mirror Support

Rhodium supports fallback mirrors for both Git and Archive sources, improving reliability when the primary source is unavailable.

## Overview

When a source fails to download, Rhodium automatically tries mirrors in the order specified. This is useful for:
- Handling temporary network issues
- Working around geo-restrictions
- Providing backup sources for critical dependencies

## Usage

### Git Mirrors

```lua
require("fmt", {
    source = "https://github.com/fmtlib/fmt.git",
    mirrors = {
        "https://gitee.com/mirrors/fmt.git",
        "https://gitlab.com/fmt/fmt.git"
    },
    ref = "10.2.1"
})
```

### Archive Mirrors

```lua
require("zlib", {
    source = "https://zlib.net/zlib-1.3.tar.gz",
    mirrors = {
        "https://mirror1.example.com/zlib-1.3.tar.gz",
        "https://mirror2.example.com/zlib-1.3.tar.gz"
    },
    source_type = "archive",
    sha256 = "ff0ba4c292013dbc27530b3a81e1f9a813cd39de01ca5e0f8bf355702efa593e"
})
```

## Behavior

### Retry Logic

1. **Primary source**: Rhodium first attempts the main `source`
2. **Mirror fallback**: If the primary fails, it tries each mirror in order
3. **Cleanup**: Failed partial downloads are removed before trying the next source
4. **Verification**: For archives, SHA256 is verified regardless of which source succeeded

### Console Output

```
info: cloning package fmt from https://github.com/fmtlib/fmt.git
error: failed to connect
warn: failed to clone from https://github.com/fmtlib/fmt.git, trying next mirror
info: trying mirror 1: https://gitee.com/mirrors/fmt.git
info: cloned package fmt from https://gitee.com/mirrors/fmt.git
```

## Best Practices

### 1. Use Trusted Mirrors

Only add mirrors you trust, as they will provide the source code for your dependencies:

```lua
mirrors = {
    "https://official-mirror.example.com/package.tar.gz",  -- ✅ Official mirror
    "https://random-site.com/package.tar.gz"               -- ❌ Untrusted source
}
```

### 2. Verify with SHA256

For archive sources, always specify `sha256` to ensure the downloaded file matches, regardless of which mirror was used:

```lua
source = "https://primary.com/lib.tar.gz",
mirrors = {"https://mirror.com/lib.tar.gz"},
sha256 = "abc123...",  -- Protects against tampering
```

### 3. Order Mirrors by Priority

List mirrors in order of preference. Rhodium tries them sequentially:

```lua
mirrors = {
    "https://fast-mirror.com/...",      -- Try fastest first
    "https://reliable-mirror.com/...",  -- Then most reliable
    "https://fallback-mirror.com/..."   -- Last resort
}
```

### 4. Keep Mirrors Updated

Ensure mirrors host the same version as your primary source. Mismatched versions will fail SHA256 verification for archives or checkout for Git.

## Use Cases

### Geo-Distributed Teams

```lua
require("boost", {
    source = "https://github.com/boostorg/boost.git",
    mirrors = {
        "https://gitee.com/mirrors/boost.git",        -- China
        "https://gitlab.com/boostorg/boost.git"       -- Europe
    },
    ref = "boost-1.83.0"
})
```

### Self-Hosted Mirrors

For corporate environments with internal mirrors:

```lua
require("openssl", {
    source = "https://github.com/openssl/openssl.git",
    mirrors = {
        "https://git.internal.company.com/openssl.git",  -- Internal mirror
        "https://gitlab.com/openssl/openssl.git"         -- Public fallback
    },
    ref = "OpenSSL_1_1_1w"
})
```

### Archived Projects

For projects that may be deleted:

```lua
require("old-lib", {
    source = "https://github.com/original/repo.git",
    mirrors = {
        "https://archive.softwareheritage.org/...",  -- Software Heritage
        "https://git.company.com/archived/old-lib.git"  -- Company archive
    },
    ref = "v1.0.0"
})
```

## Limitations

### Git Mirrors

- All mirrors must be Git repositories
- The specified `ref` (tag/branch/commit) must exist in all mirrors
- Authentication credentials are not shared across mirrors

### Archive Mirrors

- All mirrors must provide the same file (same SHA256)
- File format must be identical across mirrors
- Different compression levels are acceptable (as long as content matches)

## Technical Details

### Failure Conditions

A mirror is tried when the previous source fails due to:
- Network timeout
- Connection refused
- DNS resolution failure
- HTTP 4xx/5xx errors
- Git protocol errors

### Success Criteria

A source is considered successful when:
- **Git**: Repository is cloned and `ref` can be resolved
- **Archive**: File is downloaded and (if specified) SHA256 matches

### Cleanup

When a source fails:
1. Partial downloads are removed
2. For Git, the `.git` directory is deleted
3. For Archives, the incomplete file is deleted
4. Next mirror is attempted with a clean slate

## Comparison with Other Tools

| Feature | Rhodium | vcpkg | Conan |
|---------|--------------|-------|-------|
| Git mirrors | ✅ | ❌ | ✅ |
| Archive mirrors | ✅ | ❌ | ✅ |
| Automatic fallback | ✅ | ❌ | ✅ |
| SHA256 verification | ✅ | ✅ | ✅ |
| Custom mirror order | ✅ | ❌ | ✅ |

## Related Features

- [Archive sources](../archive-sources.md) - Using tarball/zip dependencies
- [Source types](../dsl/source-types.md) - Using Git and archive sources
- [Lockfile](../reference/lockfile.md) - Reproducible restore state
