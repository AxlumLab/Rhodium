param(
    [string]$BuildDir = "",
    [string]$BuildType = "Release",
    [string]$Toolchain = "windows-clangcl-x64",
    [string]$Config = "debug",
    [string]$VcpkgRoot = $env:VCPKG_ROOT,
    [string]$VcpkgTriplet = "x64-windows",
    [switch]$SkipBuild
)

# Source-level and dependency-export smoke script.
# By default this configures and builds the rhodium executable before using it.
# Pass -SkipBuild only when an existing executable is already present under
# -BuildDir; restore/generate still run and may build dependency packages.

$ErrorActionPreference = "Stop"

$repo = Resolve-Path (Join-Path $PSScriptRoot "..")
if ([string]::IsNullOrWhiteSpace($BuildDir)) {
    $BuildDir = "out/build/ninja-vcpkg-$VcpkgTriplet-$BuildType"
}

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,
        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]]$Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath $($Arguments -join ' ') failed with exit code $LASTEXITCODE"
    }
}

function Assert-CMakePackageExport {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Root
    )

    $stateRoot = Join-Path $Root ".rhodium"
    if (-not (Test-Path $stateRoot) -and (Test-Path (Join-Path $Root ".tpkg"))) {
        $stateRoot = Join-Path $Root ".tpkg"
    }
    $generated = Join-Path $stateRoot "generated/cmake"
    $config = Join-Path $generated "rhodiumConfig.cmake"
    $targets = Join-Path $generated "rhodiumTargets.cmake"
    if (-not (Test-Path $config)) {
        throw "missing generated CMake package config: $config"
    }
    if (-not (Test-Path $targets)) {
        throw "missing generated CMake targets file: $targets"
    }

    $configText = Get-Content $config -Raw
    $targetsText = Get-Content $targets -Raw
    if ($configText -notmatch "RHODIUM_TOOLCHAIN_ID" -or $configText -notmatch "RHODIUM_COMPILER_KIND") {
        throw "generated config is missing rhodium ABI metadata"
    }
    if ($targetsText -notmatch "add_library\(rhodium::") {
        throw "generated targets do not define rhodium:: imported targets"
    }
    if ($targetsText -match "INTERFACE_LINK_DIRECTORIES") {
        throw "generated targets must not rely on INTERFACE_LINK_DIRECTORIES"
    }
}

Push-Location $repo
try {
    if (-not $SkipBuild) {
        $configureArgs = @("-S", ".", "-B", $BuildDir, "-G", "Ninja", "-DCMAKE_BUILD_TYPE=$BuildType")
        if (-not [string]::IsNullOrWhiteSpace($VcpkgRoot)) {
            $configureArgs += "-DRHODIUM_VCPKG_ROOT=$VcpkgRoot"
        }
        if (-not [string]::IsNullOrWhiteSpace($VcpkgTriplet)) {
            $configureArgs += "-DVCPKG_TARGET_TRIPLET=$VcpkgTriplet"
        }
        Invoke-Checked cmake @configureArgs
        Invoke-Checked cmake --build $BuildDir
    }

    $rhodium = Join-Path $repo "$BuildDir/rhodium.exe"
    if (-not (Test-Path $rhodium)) {
        $rhodium = Join-Path $repo "$BuildDir/$BuildType/rhodium.exe"
    }
    if (-not (Test-Path $rhodium)) {
        throw "rhodium executable was not found under '$BuildDir'"
    }

    Invoke-Checked $rhodium --version
    Invoke-Checked $rhodium --root examples/cmake-consumer sdk select $Toolchain
    Invoke-Checked $rhodium --root examples/cmake-consumer deps
    Invoke-Checked $rhodium --root examples/cmake-consumer restore --config $Config
    Invoke-Checked $rhodium --root examples/cmake-consumer generate --config $Config
    Invoke-Checked $rhodium --root examples/cmake-consumer packages --verbose
    Assert-CMakePackageExport -Root (Join-Path $repo "examples/cmake-consumer")
}
finally {
    Pop-Location
}
