# Transitive Dependencies

Rhodium automatically resolves and builds transitive dependencies, eliminating the need to manually declare every dependency in your project.

## Overview

When a package declares its own dependencies, Rhodium will automatically fetch, build, and make them available to your project. This creates a clean dependency tree where each package is responsible for declaring what it needs.

## Usage

In your dependency configuration, use the `dependencies` field to declare transitive dependencies:

```lua
require("opencv", {
    source = "https://github.com/opencv/opencv.git",
    ref = "4.8.0",
    dependencies = {"zlib", "jpeg", "png"}
})

require("zlib", {
    source = "https://github.com/madler/zlib.git",
    ref = "v1.3"
})

require("jpeg", {
    source = "https://github.com/libjpeg-turbo/libjpeg-turbo.git",
    ref = "3.0.0"
})

require("png", {
    source = "https://github.com/glennrp/libpng.git",
    ref = "v1.6.40"
})
```

In this example, when you depend on `opencv`, Rhodium will automatically:
1. Resolve all transitive dependencies (`zlib`, `jpeg`, `png`)
2. Build them in the correct order (dependencies first)
3. Make them available to `opencv` during its build

## Features

### Automatic Resolution
Rhodium recursively walks the dependency tree and collects all required packages.

### Circular Dependency Detection
If a circular dependency is detected, Rhodium will report an error:

```
error: circular dependency detected: packageA
```

### Topological Ordering
Dependencies are built in topological order, ensuring that each package's dependencies are available before it is built.

### Deduplication
If multiple packages depend on the same library, it is only built once and shared among all dependents.

## Dependency Tree Visualization

Use the `tree` command to visualize your dependency tree:

```bash
rhodium tree
```

Output:
```
my-project
├── opencv@4.8.0
│   ├── zlib@1.3.0
│   ├── jpeg@3.0.0
│   └── png@1.6.40
└── fmt@10.2.1
```

## Best Practices

1. **Declare direct dependencies only**: In your root project, only declare packages you directly use. Their dependencies will be automatically resolved.

2. **Be specific about versions**: Use specific tags or commits to ensure reproducible builds.

3. **Document transitive dependencies**: While Rhodium handles them automatically, document why a package needs its dependencies for future maintainers.

## Error Handling

### Missing Transitive Dependency

If a transitive dependency is declared but not found:

```
error: transitive dependency not found: zlib (required by opencv)
```

**Solution**: Add the missing dependency to your `rhodium.lua`.

### Circular Dependencies

```
error: circular dependency detected: packageA
```

**Solution**: Refactor your dependencies to break the cycle. Consider:
- Extracting common code into a separate package
- Using dependency injection
- Restructuring package boundaries

## Comparison with Other Tools

| Feature | Rhodium | vcpkg | Conan |
|---------|--------------|-------|-------|
| Auto-resolve transitive deps | ✅ | ✅ | ✅ |
| Circular dep detection | ✅ | ✅ | ✅ |
| Build order guarantee | ✅ | ✅ | ✅ |
| Dependency tree visualization | ✅ | ❌ | ✅ |

## Technical Details

### Resolution Algorithm

1. Start with root dependencies
2. For each dependency:
   - Check if already visited (deduplication)
   - Check if in progress (circular detection)
   - Recursively process its dependencies
   - Mark as visited
3. Build in reverse order (topological sort)

### Lock File

Transitive dependencies are recorded in `tpkg.lock.toml` along with their exact versions (commits), ensuring reproducible builds across machines.
