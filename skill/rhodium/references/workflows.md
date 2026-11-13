# Dependency workflows

## Names and persistent state

The inspected `core/Path.cpp` prefers `rhodium.lua`, `rhodium.lock.toml`, and `.rhodium`, with legacy `tpkg.lua`, `tpkg.lock.toml`, and `.tpkg` handling. When both exist, follow actual Path API selection. Do not automatically migrate existing workspaces or delete a lockfile to bypass a failure.

`Rhodium::Core` is the embedding SDK target; `rhodium::fmt` is a generated dependency target. Resolve legacy tpkg names from actual generated configuration, respecting case.

## Minimal recipe and commands

In a consumer CMake workspace, create `rhodium.lua`:

```lua
package("Demo")
version("0.1.0")
default_config("debug")
default_platform("host")
default_arch("x64")
require("fmt", {
    source = "https://github.com/fmtlib/fmt.git",
    ref = "10.2.1",
    build = "cmake",
    cmake = { options = { FMT_DOC = false, FMT_TEST = false } },
    artifacts = { include_dirs = {"include"}, libs = {"fmt"} }
})
```

This pins a Git ref; it does not promise a built-in registry or semantic-version solver. Read current DSL documentation for mirrors, archives, prebuilt artifacts, patches, and transitive recipes.

Current README command shape, assuming an existing CLI and permission to run it:

```text
rhodium --root <consumer> sdk detect
rhodium --root <consumer> sdk list
rhodium --root <consumer> restore --config debug --toolchain <profile-id>
rhodium --root <consumer> generate --config debug --toolchain <profile-id>
```

Consumers include `.rhodium/generated/cmake/rhodiumConfig.cmake` and link `rhodium::<package>`. Verify actual paths for legacy state or custom export settings. Restore can fetch/build third-party libraries; permission to read or edit a recipe is not automatically permission to execute it.

## Evidence and repair scope

1. **Acquisition:** inspect source/ref/hash, mirrors/proxies, and Git/archive errors; an offline environment is not a compiler defect.
2. **Restore/locking:** compare manifest, overrides, resolved revisions, selected config/profile, and lockfile path. Do not bypass `--locked` by deleting locks or silently updating resolutions.
3. **Build:** inspect the chosen builder's argv/cwd/env, CRT/linkage/PIC, and profile completeness. Matching compiler paths alone do not prove ABI compatibility.
4. **Artifacts:** inspect artifact metadata, lock entries, declared include/lib/runtime paths, and materialization roots. Fix recipes/path interpretation rather than exposing incidental build directories as public interfaces.
5. **Export/link:** inspect propagation, library order, system libraries/frameworks, and dynamic runtimes; compare the consumer and restore triplet/config/SDK/CRT.

Prove why an affected package/config fingerprint should invalidate. Delete only the affected scope when authorized, preserving useful logs. On Windows, resolve and verify absolute paths before destructive operations.

## Standalone SDK builds

Read root CMake for current CLI/Core, static/shared, and installation switches. MSVC host and library runtimes must match; shared Rhodium exchanges STL objects and requires a compatible DLL CRT.

Read `docs/guide/embedding.md`, `docs/contributing/development-setup.md`, and current agent instructions before proposing or executing the smallest authorized build. Do not assume historical `install-static` or `static-deps` SDK directories still exist.
