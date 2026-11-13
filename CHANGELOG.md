# Changelog

This project follows a human-maintained changelog. Record user-visible changes only; internal-only refactors may be grouped under "Internal".

## 0.1.3

- Fixed CI vcpkg baseline resolution by checking out the pinned baseline with full history.
- Verify Lua source downloads with official mirror fallback, and retain dependency caches after build failures.
- Build installable Windows MSVC, Linux GCC, and macOS ARM64 Clang packages; removed Linux Clang.
- Publish tagged versions automatically as GitHub Releases with archives and SHA-256 checksums.
- Added static/shared library installs and CMake package exports.
- Default new lockfiles to `rhodium.lock.toml`, retaining legacy lockfile compatibility.
- Updated the organization name and copyright headers to AxlumLab.

## Unreleased

### Documentation

- Added project-level README.
- Added user guide, DSL guide, command reference, architecture notes, contributor documentation, and agent task plans.
- Added example dependency manifests.

### Internal

- Removed stale refactoring plan and incomplete utility headers that did not represent the current architecture direction.
- Reorganized CMake source collection by module groups while preserving the existing target layout.

## 0.1.1

- Existing development snapshot.
