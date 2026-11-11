# Code organization

This document defines the intended source layout after the maintenance refactor.

## CLI

Current target structure:

```text
src/rhodium/cli/
  CommandLine.hpp
  CommandLine.cpp
  CommandContext.hpp
  CommandContext.cpp
  CommandCommon.hpp
  CommandCommon.cpp
  DoctorCommand.cpp
  LoadCommand.cpp
  DumpModelCommand.cpp
  DepsCommand.cpp
  GenerateCommand.cpp
  RestoreCommand.cpp
  UpdateCommand.cpp
  PackagesCommand.cpp
  TreeCommand.cpp
  CleanCommand.cpp
  OverrideCommand.cpp
  SdkCommand.cpp
```

`CommandLine.cpp` should register the CLI application and subcommands only. Command implementation belongs in command files.

## Toolchain detection

Target structure:

```text
src/rhodium/toolchain/
  ToolchainDetector.hpp
  ToolchainDetector.cpp
  ToolchainProbe.hpp
  ToolchainProbe.cpp
  WindowsToolchainDetector.cpp
  AndroidToolchainDetector.cpp
  UnixToolchainDetector.cpp
```

Split by platform because environment variables, SDK paths, compiler families, and validation rules are platform-specific.

## Dependency resolver

Target structure:

```text
src/rhodium/resolve/
  DependencyResolver.hpp
  DependencyResolver.cpp
  DependencyGraph.hpp
  DependencyGraph.cpp
  DependencyFetchStep.hpp
  DependencyFetchStep.cpp
  DependencyBuildStep.hpp
  DependencyBuildStep.cpp
  DependencyLockStep.hpp
  DependencyLockStep.cpp
  DependencyExportStep.hpp
  DependencyExportStep.cpp
```

`DependencyResolver` should become the high-level coordinator.

## Lua DSL

Target structure:

```text
src/rhodium/script/
  LuaBindings.hpp
  LuaBindings.cpp
  LuaValueReader.hpp
  LuaValueReader.cpp
  LuaDependencyReader.hpp
  LuaDependencyReader.cpp
  LuaOverlayReader.hpp
  LuaOverlayReader.cpp
  LuaCondition.hpp
  LuaCondition.cpp
  LuaDslDiagnostics.hpp
  LuaDslDiagnostics.cpp
```

`LuaBindings.cpp` should register functions; readers should parse tables into model data.

## Package builder utilities

Target structure:

```text
src/rhodium/package/
  PackageBuilderUtil.hpp
  PackageBuilderUtil.cpp
  BuildVariableExpander.hpp
  BuildVariableExpander.cpp
  ArtifactValidator.hpp
  ArtifactValidator.cpp
  ArtifactMaterializer.hpp
  ArtifactMaterializer.cpp
  CMakeArgumentBuilder.hpp
  CMakeArgumentBuilder.cpp
  ToolchainContextResolver.hpp
  ToolchainContextResolver.cpp
```

Keep `PackageBuilderUtil` as a thin compatibility facade while moving large responsibilities out.
