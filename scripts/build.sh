#!/usr/bin/env bash
set -euo pipefail

build_type="${BUILD_TYPE:-Release}"
generator="${GENERATOR:-Ninja}"
vcpkg_root="${RHODIUM_VCPKG_ROOT:-${VCPKG_ROOT:-}}"
vcpkg_triplet="${VCPKG_TARGET_TRIPLET:-x64-windows}"
build_dir="${BUILD_DIR:-out/build/ninja-vcpkg-$vcpkg_triplet-$build_type}"

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_dir"

configure_args=(
  -S .
  -B "$build_dir"
  -G "$generator"
  "-DCMAKE_BUILD_TYPE=$build_type"
)

if [[ -n "$vcpkg_root" ]]; then
  configure_args+=("-DRHODIUM_VCPKG_ROOT=$vcpkg_root")
fi

if [[ -n "$vcpkg_triplet" ]]; then
  configure_args+=("-DVCPKG_TARGET_TRIPLET=$vcpkg_triplet")
fi

cmake "${configure_args[@]}"
cmake --build "$build_dir" --config "$build_type"
