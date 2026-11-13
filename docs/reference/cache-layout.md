<!--
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
-->
# Cache layout

Rhodium uses `.rhodium/` for new local generated state. If `.rhodium/` is absent and a legacy `.tpkg/` directory exists, it reuses `.tpkg/` without moving or copying it. If both exist, `.rhodium/` takes precedence. All cache, artifact, toolchain, override, generation, and clean operations use the selected directory. `clean --all` removes only that selected directory.

The same directory selection applies to user-level toolchains under the home directory. Internal cache structure, marker filenames, and artifact ID generation remain unchanged.

```text
.rhodium/
  packages/          dependency source cache
  build-packages/    dependency build workspaces
  artifacts/         exported artifacts
  generated/cmake/   generated CMake integration
  toolchains/        detected toolchain cache
  local.toml         local workspace preferences, when present
```

## Stability

The existence of these top-level concepts is stable. Exact internal artifact directory names are internal implementation details.
