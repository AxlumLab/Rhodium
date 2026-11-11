# Source types

Rhodium can restore dependencies from Git, archives, and local paths.

## Git source

```lua
require("fmt", {
    source = "https://github.com/fmtlib/fmt.git",
    ref = "10.2.1",
    build = "cmake"
})
```

`ref` may be a tag, branch, or commit. Lockfiles should record the resolved commit.

## Archive source

```lua
require("zlib", {
    source = "https://zlib.net/zlib-1.3.tar.gz",
    source_type = "archive",
    sha256 = "...",
    build = "cmake"
})
```

Use `sha256` for archives whenever possible.

## Local source

```lua
require_local("../libs/internal", {
    name = "internal",
    build = "cmake"
})
```

Local sources are useful for private workspace dependencies.

## Mirrors

```lua
require("fmt", {
    source = "https://github.com/fmtlib/fmt.git",
    mirrors = {
        "https://gitee.com/mirrors/fmt.git"
    },
    ref = "10.2.1"
})
```

Mirrors must represent equivalent source content.
