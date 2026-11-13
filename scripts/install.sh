#!/usr/bin/env bash
# @Author: AxlumLab
# @LastEditors: AxlumLab
# Copyright (c) 2026 by AxlumLab, All Rights Reserved.

set -euo pipefail

build_type="${BUILD_TYPE:-Release}"
install_prefix="${INSTALL_PREFIX:-out/install/rhodium}"
vcpkg_triplet="${VCPKG_TARGET_TRIPLET:-x64-windows}"
build_dir="${BUILD_DIR:-out/build/ninja-vcpkg-$vcpkg_triplet-$build_type}"

repo_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_dir"

"$repo_dir/scripts/build.sh"
cmake --install "$build_dir" --config "$build_type" --prefix "$install_prefix"
