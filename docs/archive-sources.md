# Archive Sources

Rhodium supports fetching dependencies from archive files (`.tar.gz`, `.tar.bz2`, `.zip`, etc.) in addition to Git repositories.

## Basic Usage

```lua
require("mylib", {
    source = "https://example.com/mylib-1.0.tar.gz",
    type = "archive",
    sha256 = "abcdef1234567890...",  -- Required for verification
    strip_components = 1,            -- Optional: remove top-level directory
    build = "cmake",
    -- ... other configuration
})
```

## Parameters

### `source` (required)
The URL to the archive file. Supported formats:
- `.tar.gz`, `.tgz`
- `.tar.bz2`, `.tbz2`
- `.tar.xz`, `.txz`
- `.zip`

### `type` (optional)
Set to `"archive"` to explicitly specify archive source type. Usually auto-detected from URL.

### `sha256` (required)
SHA256 hash of the archive file for verification. This ensures the downloaded file hasn't been tampered with.

To calculate the SHA256 hash:
```bash
# Linux/macOS
sha256sum mylib-1.0.tar.gz

# Windows (PowerShell)
Get-FileHash mylib-1.0.tar.gz -Algorithm SHA256
```

### `strip_components` (optional, default: 0)
Number of leading path components to strip when extracting. Most archives contain a top-level directory (e.g., `mylib-1.0/`), so set this to `1` to remove it.

Example:
```
Without strip_components=1:
  mylib-1.0/
  mylib-1.0/include/
  mylib-1.0/src/

With strip_components=1:
  include/
  src/
```

## Complete Example

```lua
require("stb", {
    source = "https://github.com/nothings/stb/archive/refs/tags/master.zip",
    type = "archive",
    sha256 = "1234567890abcdef...",
    strip_components = 1,
    build = "header_only",
    artifacts = {
        include_dirs = { "." }
    }
})
```

## Mirrors

You can provide fallback mirrors in case the primary source is unavailable:

```lua
require("mylib", {
    source = "https://primary.com/mylib-1.0.tar.gz",
    mirrors = {
        "https://mirror1.com/mylib-1.0.tar.gz",
        "https://mirror2.com/mylib-1.0.tar.gz"
    },
    sha256 = "...",
    -- ... rest of configuration
})
```

Rhodium will try each URL in order until one succeeds.

## Git vs Archive

| Feature | Git | Archive |
|---------|-----|---------|
| Version selection | Tags, branches, commits | Fixed version only |
| Update checking | Can check for updates | Manual |
| Download size | Full history (larger) | Single version (smaller) |
| Offline work | Yes (after clone) | No (unless cached) |
| Verification | Git commit hash | SHA256 hash |

**Recommendation:** Use Git sources when possible for better version management. Use archives for:
- Libraries that only release tarballs
- Reducing download size
- Faster initial download (no git history)
