<!--
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
-->
# Maintenance refactor notes

This snapshot performs the low-risk maintenance preparation work and packages the remaining large code refactor as staged agent tasks.

## Completed in this snapshot

- Added root `README.md` with corrected project positioning: Rhodium is a dependency manager for CMake projects, not a general build system.
- Added standard project files:
  - `CONTRIBUTING.md`
  - `CODE_OF_CONDUCT.md`
  - `SECURITY.md`
  - `CHANGELOG.md`
  - `LICENSE`
  - `AGENTS.md`
- Removed stale/incomplete refactor artifacts from the implementation tree.
- Fixed `.gitignore` formatting and expanded local/generated ignores.
- Reworked `cmake/RhodiumTargets.cmake` to collect source files by module groups instead of one repository-wide source glob.
- Added documentation structure under `docs/`:
  - user guides
  - DSL guides
  - command reference
  - architecture notes
  - contributor notes
  - agent task plan
- Added example manifests under `examples/`.

## Intentionally not done in this snapshot

The large `.cpp` splits were not performed here because the project owner explicitly wants to run compilation and testing locally, and large mechanical refactors without local validation can introduce avoidable breakage.

The following staged tasks are ready for an agent:

- `docs/agent-tasks/02-cli-split.md`
- `docs/agent-tasks/03-toolchain-detector-split.md`
- `docs/agent-tasks/04-dependency-resolver-split.md`
- `docs/agent-tasks/05-lua-dsl-split.md`
- `docs/agent-tasks/06-package-builder-util-split.md`

Agents must follow `AGENTS.md` and must not compile or run tests unless explicitly instructed.

## Owner validation

Recommended first checks:

```bash
grep -R "REFACTORING_PLAN\|ToolchainUtil\|PackageArtifactUtil" -n docs src cmake README.md
```

Expected: references only in agent task history/checklists, not in implementation, CMake, or user docs.

Then run your normal configure/build/test process locally.
