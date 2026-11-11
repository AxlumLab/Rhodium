<!--
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
-->
# Agent guide

This repository may be modified by coding agents. Agents must follow these rules.

## Hard constraints

- Do not compile.
- Do not run tests.
- Do not change external behavior unless a task explicitly says so.
- Do not change CLI commands, option names, or default values.
- Do not change DSL field names or accepted value meanings.
- Do not change lockfile format.
- Do not change artifact ID generation.
- Do not change cache layout.
- Do not turn Rhodium into a general build system.

## Preferred workflow

1. Read the relevant task in `docs/agent-tasks/`.
2. Read the architecture documents for the touched module.
3. Make mechanical changes first.
4. Keep commits or patches small by phase.
5. Update docs only when the task asks for it.
6. Leave validation commands for the owner.

## Refactor style

- Extract existing logic without changing it.
- Move helper functions into named modules only when there is a clear owner.
- Prefer stage classes for dependency restore.
- Prefer platform files for toolchain detection.
- Prefer reader classes for Lua table parsing.
- Keep facade functions temporarily when it reduces risk.

## Deliverable expectations

Each task should end with a short report containing:

- Files changed.
- Behavior intentionally preserved.
- Any places that need owner validation.
- Any unresolved ambiguity.
