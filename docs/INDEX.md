<!--
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
-->
# Rhodium documentation

Rhodium is a dependency management system for C/C++ projects that use CMake. It restores dependencies, builds dependency artifacts when needed, records reproducible lock state, and generates CMake integration files.

It is not a general build system. Your project still uses CMake and a native backend such as Ninja, Visual Studio, or Xcode.

## Start here

- [Getting started](getting-started.md)
- [Installation](installation.md)
- [Core concepts](concepts.md)
- [Troubleshooting](troubleshooting.md)

## Guides

- [Basic workflow](guide/basic-workflow.md)
- [CMake integration](guide/cmake-integration.md)
- [Toolchains](guide/toolchains.md)
- [Dependency restore](guide/dependency-restore.md)
- [Dependency update](guide/dependency-update.md)
- [Local overrides](guide/local-overrides.md)
- [Package cache](guide/package-cache.md)
- [Mirrors](guide/mirrors.md)
- [Cleaning](guide/cleaning.md)

## DSL guide

- [DSL overview](dsl/overview.md)
- [Package metadata](dsl/package-metadata.md)
- [Dependency declaration](dsl/dependency-declaration.md)
- [Source types](dsl/source-types.md)
- [Build systems](dsl/build-systems.md)
- [Artifacts](dsl/artifacts.md)
- [Platform overrides](dsl/platform-overrides.md)
- [Custom build](dsl/custom-build.md)
- [DSL examples](dsl/examples.md)

## Reference

- [Command reference](reference/command-reference.md)
- [DSL reference](reference/dsl-reference.md)
- [Lockfile](reference/lockfile.md)
- [Cache layout](reference/cache-layout.md)
- [Generated CMake](reference/generated-cmake.md)
- [Exit codes](reference/exit-codes.md)

## Architecture and contribution

- [Architecture overview](architecture/overview.md)
- [Code organization](architecture/code-organization.md)
- [Dependency resolution](architecture/dependency-resolution.md)
- [Package build flow](architecture/package-build-flow.md)
- [Toolchain detection](architecture/toolchain-detection.md)
- [Development setup](contributing/development-setup.md)
- [Coding style](contributing/coding-style.md)
- [Testing](contributing/testing.md)
- [Release process](contributing/release-process.md)
- [Agent guide](contributing/agent-guide.md)
- [Agent task plan](agent-tasks/README.md)

## Stability levels

Rhodium documentation uses these labels:

- **Stable**: intended for normal users and should not change without migration notes.
- **Experimental**: usable, but behavior or fields may change.
- **Internal**: implementation detail; do not rely on it in user projects.
- **Planned**: accepted direction but not implemented or not fully validated.
