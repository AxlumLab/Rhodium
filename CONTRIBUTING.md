# Contributing to Rhodium

Rhodium is a C/C++ dependency manager for CMake projects. Contributions should preserve that scope.

## Contribution rules

- Do not turn Rhodium into a general-purpose build-system replacement.
- Do not change CLI behavior, DSL fields, lockfile format, artifact IDs, or cache layout unless the change is explicitly planned.
- Prefer small, staged changes over broad rewrites.
- Keep `model/` as plain data and validation logic. Do not move I/O, resolver, builder, or CLI behavior into model structures.
- Avoid large `Util` classes. Split by responsibility and workflow stage instead.
- Update user-facing documentation when command behavior or DSL behavior changes.

## Local development

See [development setup](docs/contributing/development-setup.md).

## Coding style

See [coding style](docs/contributing/coding-style.md).

## Testing

See [testing](docs/contributing/testing.md).

## Agent-assisted changes

When using an automated coding agent, use [agent guide](docs/contributing/agent-guide.md) and the staged tasks in [docs/agent-tasks](docs/agent-tasks/README.md).

Agents should not compile or run tests unless explicitly instructed by the project owner. The owner will run validation locally.
