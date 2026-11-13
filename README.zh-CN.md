# Rhodium

[English](README.md) | **简体中文**

**为 CMake 项目设计的 C/C++ 依赖管理器。**

Rhodium 负责获取、构建和缓存第三方依赖，将实际解析的源码版本记录到锁文件，并生成可供 CMake 使用的依赖目标。

## 核心功能

- **Lua 清单：** 在 `rhodium.lua` 中声明 Git、压缩包、本地源码或预编译依赖。
- **依赖构建：** 支持 CMake、Make、configure + Make、自定义、纯头文件和预编译模式。
- **可重复恢复：** 通过 `rhodium.lock.toml` 保存解析状态，使用 `restore --locked` 按锁文件恢复。
- **工具链感知：** 检测编译器与 SDK 配置，按不同构建配置管理产物。
- **CMake 集成：** 导出 `rhodium::fmt` 等目标，直接供 `target_link_libraries` 使用。
- **开发工具：** 本地依赖覆盖、传递依赖树、镜像源、依赖更新与缓存清理。

## 从源码构建

需要 CMake **3.25+**、支持 **C++20** 的编译器、Git 和 [vcpkg](https://github.com/microsoft/vcpkg)。推荐 Ninja。

```sh
git clone https://github.com/AxlumLab/Rhodium.git
cd Rhodium
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DRHODIUM_VCPKG_ROOT=/path/to/vcpkg -DRHODIUM_BUILD_TESTS=OFF
cmake --build build --config Release
```

将 `/path/to/vcpkg` 替换为本机 vcpkg 目录。也可以指定 `CMAKE_TOOLCHAIN_FILE` 或设置 `VCPKG_ROOT`。Windows 下使用 MSVC 和 Ninja 时，请在已经加载 MSVC 环境的开发者终端执行。

默认构建会生成静态 `Rhodium::Core` 库和 `rhodium` 命令行程序（Windows 上为 `rhodium.exe`）。请将命令行程序所在目录加入 `PATH`。更多选项参见[安装指南](docs/installation.md)和[嵌入指南](docs/guide/embedding.md)。

## 快速开始

仓库包含完整的 [CMake 示例](examples/basic-cmake/)。以下命令在仓库根目录执行，且假设已构建 Rhodium 并把 CLI 加入 `PATH`。

### 1. 检测工具链

```sh
rhodium --root examples/basic-cmake sdk detect
rhodium --root examples/basic-cmake sdk list
```

### 2. 恢复依赖并生成集成文件

将 `YOUR_TOOLCHAIN_ID` 替换为 `sdk list` 列出的工具链 ID。

```sh
rhodium --root examples/basic-cmake restore --config debug --toolchain YOUR_TOOLCHAIN_ID
rhodium --root examples/basic-cmake generate --config debug --toolchain YOUR_TOOLCHAIN_ID
```

### 3. 使用 CMake 构建示例

```sh
cmake -S examples/basic-cmake -B build/example -G Ninja -DCMAKE_BUILD_TYPE=Debug
cmake --build build/example
```

恢复依赖和构建应用时，需要使用兼容的编译器、架构与 Debug/Release 配置。示例清单默认使用 `x64`。

## 声明依赖

在 CMake 项目根目录创建 `rhodium.lua`：

```lua
package("MyApp")
version("0.1.0")
default_config("debug")
default_platform("host")
default_arch("x64")

require("fmt", {
    source = "https://github.com/fmtlib/fmt.git",
    ref = "10.2.1",
    build = "cmake",
    cmake = {
        options = {
            FMT_DOC = false,
            FMT_TEST = false
        }
    },
    artifacts = {
        include_dirs = {"include"},
        libs = {"fmt"}
    }
})
```

执行 `rhodium restore` 和 `rhodium generate` 后，在 `CMakeLists.txt` 中加载生成的配置：

```cmake
cmake_minimum_required(VERSION 3.25)
project(MyApp LANGUAGES CXX)

set(RHODIUM_DEPS_FILE "${CMAKE_SOURCE_DIR}/.rhodium/generated/cmake/rhodiumConfig.cmake")
if(NOT EXISTS "${RHODIUM_DEPS_FILE}")
    message(FATAL_ERROR "Run rhodium restore and rhodium generate first")
endif()
include("${RHODIUM_DEPS_FILE}")

add_executable(myapp src/main.cpp)
target_link_libraries(myapp PRIVATE rhodium::fmt)
```

## 常用命令

| 命令 | 功能 |
| --- | --- |
| `rhodium sdk detect` / `rhodium sdk list` | 检测并查看工具链 |
| `rhodium restore` | 获取、构建并导出依赖 |
| `rhodium restore --locked` | 根据现有锁文件恢复依赖 |
| `rhodium generate` | 生成 CMake 导入目标 |
| `rhodium deps` / `rhodium tree` | 查看依赖声明和依赖树 |
| `rhodium update [package]` | 更新依赖解析状态 |
| `rhodium packages` / `rhodium packages gc` | 查看和清理缓存 |

完整参数见[命令行参考](docs/reference/command-reference.md)。注意：部分诊断命令（例如 `doctor`）只在 Debug 构建中注册。

## 作为 C++20 库嵌入

Rhodium 可以作为静态库或动态库安装，嵌入其他应用时可以关闭 CLI 构建。

```cmake
find_package(Rhodium CONFIG REQUIRED)
target_link_libraries(myapp PRIVATE Rhodium::Core)
```

`Rhodium::Core` 代表 Rhodium SDK 本身，而导出的第三方依赖使用独立的 `rhodium::` 命名空间。详见[嵌入指南](docs/guide/embedding.md)。

## 平台状态与旧版兼容

| 平台 | 状态 |
| --- | --- |
| Windows Linux 与 macOS | 当前主要验证的开发环境 |
| Android | 已支持工具链配置 |
| iOS | 计划支持，尚未实现 |

## 文档

- [文档索引](docs/INDEX.md)、[入门指南](docs/getting-started.md)、[安装指南](docs/installation.md)
- [DSL 说明](docs/dsl/overview.md)、[工具链指南](docs/guide/toolchains.md)、[命令行参考](docs/reference/command-reference.md)
- [架构说明](docs/architecture/overview.md)、[示例工程](examples/basic-cmake/)、[更新日志](CHANGELOG.md)

## 参与贡献与许可证

欢迎报告问题和贡献代码。请阅读 [CONTRIBUTING.md](CONTRIBUTING.md)，也可以通过 [GitHub Issues](https://github.com/AxlumLab/Rhodium/issues) 反馈。

本项目使用 **MIT License**，详见 [LICENSE](LICENSE)。
