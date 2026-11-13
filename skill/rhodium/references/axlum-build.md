# AxlumBuild consumption and cross-repository fixes

## Establish what is actually linked

The independent Rhodium worktree, AxlumBuild, and `AxlumBuild/third_party/rhodium` have distinct Git state. Common local roots are `E:/Projects/Rhodium` and `E:/Projects/AxlumBuild`; locate actual paths on other machines.

Read the consumer's `.gitmodules`, root CMake, `src/packages/Rhodium.*`, and `src/toolchain/Toolchain.*`. Compare both Rhodium HEADs with the consumer's recorded gitlink. The consumer defaults to submodule `Rhodium::Core` with CLI/tests/install disabled. Installed mode via `AXLUM_USE_INSTALLED_RHODIUM=ON` is an explicit alternative, not an automatic fallback.

Bootstrap dependencies belong to CMake/vcpkg; user-project packages belong to embedded Rhodium. Do not change AxlumBuild bootstrap to Rhodium package management to fix a user recipe.

## Responsibility boundary

- Rhodium owns recipe parsing, acquisition, overrides, restore/lock/cache/artifacts, SDK/profile detection, and dependency CMake exports.
- AxlumBuild owns workspace/target/module/filter, native/C# build graphs, Ninja, IDE output, run/clean/install, and the adapter into those interfaces.
- Bridge anchors are `PackageManager::resolvePackages`, `packageInputsFingerprint`, `ToolchainDiscovery`, and Rhodium binding calls in LuaManifest.

Trace `LuaDependencyReader`, `DependencyRestoreRequest/DependencyResolver`, `LockFile`, and `PackageArtifactResolver`. Reuse Detector/Registry instead of maintaining another SDK detector in the consumer. A CLI wrapper is not automatically a linkable Core API; do not call CLI-only implementation from Core.

## Compatibility seams

Rhodium's Path API prefers rhodium filenames, while the AxlumBuild adapter has previously hardcoded `tpkg.lock.toml`. If restore succeeds but artifacts/locks are missing, compare actual linked Path API behavior with consumer paths, workspace roots, and coexisting old/new state. Do not mask the mismatch by copying locks or wiping caches.

Standalone recipes use `package/require`. AxlumBuild's `axlum.build.lua` uses `workspace/target`, `rhodium.require/require_local`, and `packages/public_packages`. The consumer DSL does not justify adding general project build declarations to Rhodium.

## Complete a coordinated change

Consumer-only adaptation belongs in AxlumBuild. Fix a generic provider defect in Rhodium and check its API/persistent compatibility. Advance the consumer pin only when an upgrade is authorized. Uncommitted worktree copies are not a released provider version.

Report provider changes/commit, consumer changes/gitlink, profile/config, and minimal checks actually run. Do not commit or push either repository, or move a published tag, without relevant authorization.

AxlumBuild uses `namespace axlum`, PascalCase filenames, m_/s_ naming, and root-only CMake organization. Do not copy Rhodium's directory layout into it. Its managed backend uses the official .NET SDK; do not broaden a fix into Mono/UWP/GDK/console/32-bit support.
