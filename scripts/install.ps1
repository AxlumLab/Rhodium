# @Author: AxlumLab
# @LastEditors: AxlumLab
# Copyright (c) 2026 by AxlumLab, All Rights Reserved.

param(
    [string]$BuildDir = "",
    [string]$BuildType = "Release",
    [string]$InstallPrefix = "out/install/rhodium",
    [string]$Generator = "Ninja",
    [string]$VcpkgRoot = $(if ($env:RHODIUM_VCPKG_ROOT) { $env:RHODIUM_VCPKG_ROOT } else { $env:VCPKG_ROOT }),
    [string]$VcpkgTriplet = $(if ($env:VCPKG_TARGET_TRIPLET) { $env:VCPKG_TARGET_TRIPLET } else { "x64-windows" })
)

$ErrorActionPreference = "Stop"
$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = "out/build/ninja-vcpkg-$VcpkgTriplet-$BuildType"
}

& (Join-Path $PSScriptRoot "build.ps1") `
    -BuildDir $BuildDir `
    -BuildType $BuildType `
    -Generator $Generator `
    -VcpkgRoot $VcpkgRoot `
    -VcpkgTriplet $VcpkgTriplet

if ($LASTEXITCODE -ne 0) {
    throw "build.ps1 failed with exit code $LASTEXITCODE"
}

Push-Location $repo
try {
    & cmake --install $BuildDir --config $BuildType --prefix $InstallPrefix
    if ($LASTEXITCODE -ne 0) {
        throw "cmake --install failed with exit code $LASTEXITCODE"
    }
}
finally {
    Pop-Location
}
