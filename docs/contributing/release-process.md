# Release process

Rhodium releases use semantic versions and annotated Git tags, for example `0.1.3`.
Tags have **no `v` prefix** and must exactly match both `project(Rhodium VERSION ...)`
in `CMakeLists.txt` and `version-string` in `vcpkg.json`.

## Agent responsibilities

When asked to release a version, the agent must complete the release work:

1. Stay on `main`. Update the CMake and manifest versions together and add a
   matching entry to `CHANGELOG.md`. Preserve the pinned vcpkg baseline unless
   dependency updates are explicitly part of the release.
2. Validate the workflow, build, and install as appropriate to the change. Use
   existing tests when needed; do not add test cases solely for a release.
3. Commit the finished version on `main`, then create its annotated version tag
   on that exact commit. The agent sets the tag; the workflow never guesses or
   increments versions and does not create Git tags.
4. Push `main` before pushing the version tag so CI can confirm that the release
   commit belongs to `main`. Never push release work to `dev`.
5. Check the tag-triggered Actions run and report the actual build/release result
   and URL. A pushed tag alone does not mean the Release has been published.

For version `0.1.3`, after committing and validating:

```sh
git tag -a 0.1.3 -m "Rhodium 0.1.3"
git push origin main
git push origin refs/tags/0.1.3
```

Do not move or overwrite an existing release tag. Fix a failed workflow and
release a new patch version if the tagged source must change. Re-running a
successful tag's workflow may replace its generated assets without moving the tag.

## Automated build and publication

`.github/workflows/main.yml` runs on main/master pushes, pull requests, manual
runs, and numeric version-tag pushes. Only a version-tag push publishes a Release.
The prepare job checks version equality and requires the tagged commit to be on
`main`. Invalid tags fail before dependency builds.

Every build checks out `microsoft/vcpkg` at the manifest's `builtin-baseline` with
`fetch-depth: 0`, then verifies that `versions/baseline.json` exists at that commit.
The complete history is required for pinned port versions; do not replace it
with a shallow checkout of the default branch. Cache downloads and binary packages,
not dependency build trees shared between compiler configurations. Save completed
binary packages even if a later port or project build fails, so retries can reuse
the work. CI prefetches Lua from its official endpoints or the Lua.org-listed
PUC-Rio mirror and verifies the SHA-512 pinned in the checked-out vcpkg port;
TLS verification and source hashes must never be disabled to bypass a download failure.

The required Release build matrix is:

| Runner | Compiler | vcpkg triplet | Archive |
| --- | --- | --- | --- |
| Windows | MSVC x64 | `x64-windows` | `.zip` |
| Linux | GCC x64 | `x64-linux` | `.tar.gz` |
| macOS | Apple Clang ARM64 | `arm64-osx` | `.tar.gz` |

Windows initializes the MSVC environment before the Ninja build. Each job builds,
installs, runs the installed CLI's `--version`, and uploads a versioned archive
containing the install prefix (CLI, library, headers, CMake package, and license).
Use `cmake --install`, not a hand-picked executable copy, to prepare the archive.

All three platform jobs must succeed. The old "at least one package succeeded"
rule is removed. The release job downloads the three archives, checks that they
exist, adds `SHA256SUMS`, and publishes the GitHub Release using the existing tag.
Only that job receives `contents: write`; pull requests cannot publish releases.
A new Release remains a draft until its assets have been uploaded.

Package names include the version, compiler/platform, and triplet, for example:

- `rhodium-0.1.3-windows-msvc-x64-windows.zip`
- `rhodium-0.1.3-linux-gcc-x64-linux.tar.gz`
- `rhodium-0.1.3-macos-clang-arm64-osx.tar.gz`

## Before announcing a release

Confirm that the Release contains all three packages and `SHA256SUMS`, review
`CHANGELOG.md`, and validate the workflows or examples claimed in the release notes.
