# Embedding Rhodium

Rhodium provides a C++20 dependency-management library and an optional CLI.
The library keeps the same manifest, lockfile, artifact-ID and cache semantics as the CLI.

## Build and install

```sh
# Static library and CLI (the default).
cmake -S . -B build/static -DBUILD_SHARED_LIBS=OFF -DBUILD_TESTING=OFF
cmake --build build/static --config Release
cmake --install build/static --config Release --prefix /path/to/rhodium-static

# Shared library and CLI. Use a separate build directory and install prefix.
cmake -S . -B build/shared -DBUILD_SHARED_LIBS=ON -DBUILD_TESTING=OFF
cmake --build build/shared --config Release
cmake --install build/shared --config Release --prefix /path/to/rhodium-shared
```

On Windows, the default vcpkg triplet is `x64-windows-static` for static builds
and `x64-windows` for shared builds (`arm64` on ARM64 hosts). An explicit
`VCPKG_TARGET_TRIPLET` overrides this selection. Select matching dependencies
and compiler runtime settings when overriding the triplet. Windows shared
builds require the DLL CRT; `x64-windows-static-md` is also supported when
using static dependencies with that CRT.

Build and install the configuration used by the host program (`Release` or
`Debug`). Keep Debug and Release shared installations in separate prefixes:
some bundled dependency DLLs have the same file names in both configurations.

| Option | Purpose | Standalone default |
| --- | --- | --- |
| `BUILD_SHARED_LIBS` | Build a shared library instead of a static library | `OFF` |
| `RHODIUM_BUILD_CLI` | Build and install the `rhodium` executable | `ON` |
| `RHODIUM_BUILD_TESTS` | Build the existing tests | Follows `BUILD_TESTING` |
| `RHODIUM_INSTALL` | Generate installation rules | `ON` |

For a library-only installation, set `RHODIUM_BUILD_CLI=OFF` and
`BUILD_TESTING=OFF`. When Rhodium is included with `add_subdirectory`, the
CLI, tests and installation rules default to off.

Installation includes the library, public headers under `include/rhodium`,
`rhodium/export.hpp`, and `lib/cmake/Rhodium/RhodiumConfig.cmake` (using the
configured GNU install directories). Windows shared installations also copy
the dependent DLLs into `bin`. CLI implementation headers are excluded.

## Use the installed library

```cmake
cmake_minimum_required(VERSION 3.25)
project(MyApplication LANGUAGES CXX)

find_package(Rhodium CONFIG REQUIRED)
add_executable(myapp main.cpp)
target_link_libraries(myapp PRIVATE Rhodium::Core)

# The public C++ interface exchanges STL objects across the library boundary.
# Use matching compiler, standard-library and dependency versions, and MSVC runtime.
if(MSVC)
    set_property(TARGET myapp PROPERTY MSVC_RUNTIME_LIBRARY
        "${Rhodium_MSVC_RUNTIME_LIBRARY}")
endif()
```

Configure the application with the SDK prefix in `CMAKE_PREFIX_PATH` and the
same dependency toolchain/triplet used to build Rhodium. The installed config
finds public dependencies, plus the private link dependencies needed by a
static library. The application does not need Rhodium's source or build tree.

`Rhodium_VERSION` and `Rhodium_SHARED_LIBS` describe the installed package.
For a Windows shared build, put the installed `bin` directory on `PATH` when
running the application, or deploy those DLLs beside its executable. MSVC's
runtime redistributable must also be available when using the DLL runtime.
On Linux/macOS, configure the application's install RPATH or library search
path to include the SDK's library directory.

This SDK package uses `find_package(Rhodium)` and `Rhodium::Core`. Packages
generated for a dependency consumer continue to use `find_package(rhodium)`
and targets such as `rhodium::fmt`.

## Load a manifest in a host program

```cpp
#include <rhodium/rhodium.hpp>
#include <iostream>

class Diagnostics final : public rhodium::DiagnosticSink
{
public:
    void report(const rhodium::Diagnostic &diagnostic) override
    {
        std::cerr << rhodium::Diagnostic::name(diagnostic.level)
                  << ": " << diagnostic.message << '\n';
    }
};

int main(int argc, char **argv)
{
    if (argc != 2)
        return 2;

    Diagnostics diagnostics;
    rhodium::ManifestLoader loader;
    auto loaded = loader.load(argv[1], diagnostics);
    if (!loaded.ok)
        return 1;

    std::cout << loaded.model.rootPackage.name << '\n';
    return 0;
}
```

To restore dependencies, load overrides with `DependencyOverrides::make`,
select a toolchain using `ToolchainDetector`/`ToolchainRegistry`, and call
`DependencyResolver::restore` with a `DependencyRestoreRequest`. Its options
control locked restores, selected packages, rebuilds and build/export stages.
Diagnostics flow through the host-supplied `DiagnosticSink`; the CLI is not
required. `PackageArtifactResolver` and `CMakeDependency::writeFiles` expose
restored artifacts and CMake generation to the host.

## Use the source tree

```cmake
# Configure the parent project's dependencies/toolchain before adding Rhodium.
add_subdirectory(path/to/Rhodium)
target_link_libraries(myapp PRIVATE Rhodium::Core)
```

`Rhodium::Core` propagates C++20, include directories, static/shared export
macros and public dependency targets in both source-tree and installed usage.
