---
name: rhodium
description: Develop, use, and troubleshoot AxlumLab Rhodium recipes, dependency restore, lockfiles, caches, artifacts, CMake exports, SDK profiles, and C++ embedding. For AxlumBuild consumers, cross-check the adapter and pinned submodule revision.
---

# Rhodium

Rhodium manages dependencies for CMake/C++ consumers and provides an embeddable `Rhodium::Core`. It does not replace AxlumBuild's project compilation, Ninja graph, or IDE backends.

## Locate the worktrees and constraints

The repository root is two directories above this skill. AxlumBuild is usually a sibling; it actually links `AxlumBuild/third_party/rhodium`, not automatically this independent worktree's HEAD.

Read current `AGENTS.md` if present, `CONTRIBUTING.md`, `docs/contributing/agent-guide.md`, and relevant task files. The inspected agent guide forbids compiling/testing by default. Compare actual task and existing user authorization with these constraints: this skill grants no additional permission, but do not repeat questions when authorization already covers the action. Preserve CLI, DSL, lockfile, artifact-ID, and cache contracts during ordinary fixes; implement changes to those contracts when explicitly requested and scoped.

Use CodeGraph first when `.codegraph/` exists; otherwise use ordinary search without automatically indexing. Inspect status and revisions separately across worktrees and preserve existing changes.

## Route by failure stage

| Stage | Search anchors |
| --- | --- |
| CLI parsing/output/orchestration | `src/rhodium/cli/`, `Commands` |
| Lua manifest/include/recipe parsing | `script/LuaBindings`, `LuaDependencyReader`, `LuaRuntime` |
| Configuration and source models | `model/`, `config/BuildConfig`; keep models focused on data/validation |
| Overrides, graph, restore | `resolve/DependencyResolver`, `DependencyOverrides`, `DepGraph` |
| Fetch/build/materialize/validate/cache | Fetcher, PackageBuilder, and Artifact classes under `package/` |
| Lockfiles and compatibility paths | `config/LockFile`, `core/Path` |
| SDK profiles/environment | `toolchain/ToolchainDetector`, `ToolchainRegistry`, `ToolchainSnapshot` |
| Consumer export | CMake export implementation, generated configs, `PackageArtifactResolver` |

Capture package, source, configuration, profile, failing stage, and output paths. Do not collapse acquisition failures, compilation failures, missing artifacts, and consumer link errors into "clear the cache". Treat paths as search anchors and verify current source locations.

- For recipes, locks, and failure handling, read [dependency workflows](references/workflows.md).
- For embedding, AxlumBuild integration, or gitlink updates, read [cross-repository consumption](references/axlum-build.md).
- Use current CLI help when execution is permitted, `docs/reference/`, and `examples/basic-cmake/` for exact command/DSL syntax. Do not invent options from old tpkg documentation.

## Implementation and delivery

Follow current Rhodium conventions rather than imposing AxlumBuild's `namespace axlum` or static facade organization on `namespace rhodium`. Prefer responsibility/stage-specific classes and DiagnosticSink, not a large generic Util class. Embedded Core behavior must not depend on implementation compiled only into the CLI target.

Keep validation proportional: static checks for static changes; only run an affected test or offline fixture when authorized. Do not routinely rebuild every dependency, clear caches, or run AOT. Report checks actually performed and anything not executed, with reasons; historical reports are not current test evidence.

Commits, pushes, consumer gitlink updates, tags, and releases require task authorization. A dependency repair does not implicitly authorize those external actions.
