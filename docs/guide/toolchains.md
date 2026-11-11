# Toolchains

A toolchain profile describes the local compiler, linker, SDK, and helper tools used for dependency restore.

## Detect profiles

```bash
rhodium sdk detect
rhodium sdk list
```

## Show a profile

```bash
rhodium sdk show windows-msvc-x64
```

## Save a preferred profile

```bash
rhodium sdk select windows-msvc-x64
```

Clear it with:

```bash
rhodium sdk clear
```

## Register a profile manually

```bash
rhodium sdk add android-arm64 \
  --platform android \
  --compiler-kind clang \
  --target-arch arm64 \
  --android-sdk D:/Sdks/android \
  --android-ndk D:/Sdks/android/ndk/28.2.13676358 \
  --android-api 35 \
  --android-abi arm64-v8a \
  --source project-user
```

Use `--force` to replace an existing profile with the same ID.

## Diagnostics

```bash
rhodium sdk doctor
rhodium doctor --sdk
```

Use diagnostics when a package build cannot find a compiler, CMake, Ninja, Git, Android SDK, NDK, JDK, or system library path.
