# DSL examples

See the repository `examples/` directory for complete example manifests.

## Header-only dependency

```lua
require("nameof", {
    source = "https://github.com/Neargye/nameof.git",
    ref = "v0.10.3",
    build = "header_only",
    artifacts = {
        include_dirs = {"include"}
    }
})
```

## CMake dependency

```lua
require("fmt", {
    source = "https://github.com/fmtlib/fmt.git",
    ref = "10.2.1",
    build = "cmake",
    cmake = {
        options = {
            FMT_DOC = false,
            FMT_TEST = false
        }
    },
    artifacts = {
        include_dirs = {"include"},
        libs = {"fmt"}
    }
})
```

## Dependency with local override

```bash
rhodium override set fmt ../fmt-fork
rhodium restore fmt
```

## Transitive dependency

```lua
require("fmt", {
    source = "https://github.com/fmtlib/fmt.git",
    ref = "10.2.1",
    build = "cmake"
})

require("spdlog", {
    source = "https://github.com/gabime/spdlog.git",
    ref = "v1.13.0",
    build = "cmake",
    dependencies = {"fmt"}
})
```
