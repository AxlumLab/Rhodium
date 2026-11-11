# Security Policy

Rhodium downloads, unpacks, patches, builds, and exposes third-party dependency artifacts. Treat dependency manifests and lock files as security-sensitive project inputs.

## Supported versions

Only the current development branch is maintained unless release branches are explicitly created later.

## Reporting a vulnerability

Report security issues privately to the project maintainer before opening a public issue.

Include:

- Affected command or workflow.
- Minimal `rhodium.lua` reproduction.
- Platform and toolchain profile.
- Whether the issue involves source fetching, archive extraction, patch application, command execution, generated CMake files, or lockfile behavior.

## Security-sensitive areas

- Archive extraction and path traversal prevention.
- Patch application paths.
- Custom commands in dependency manifests.
- Environment variable expansion.
- Generated CMake files.
- Git source URLs, mirrors, refs, and lockfile commits.
- Local overrides.

## Manifest trust model

`rhodium.lua` is executable Lua configuration. Do not run manifests from untrusted repositories without review.
