param(
    [string]$BuildDir = "",
    [string]$BuildType = "Release",
    [string]$Generator = "Ninja",
    [string]$VcpkgRoot = $(if ($env:RHODIUM_VCPKG_ROOT) { $env:RHODIUM_VCPKG_ROOT } else { $env:VCPKG_ROOT }),
    [string]$VcpkgTriplet = $(if ($env:VCPKG_TARGET_TRIPLET) { $env:VCPKG_TARGET_TRIPLET } else { "x64-windows" })
)

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

Push-Location $repo
try {
    $configureArgs = @("-S", ".", "-B", $BuildDir, "-G", $Generator, "-DCMAKE_BUILD_TYPE=$BuildType")
    if (-not [string]::IsNullOrWhiteSpace($VcpkgRoot)) {
        $configureArgs += "-DRHODIUM_VCPKG_ROOT=$VcpkgRoot"
    }
    if (-not [string]::IsNullOrWhiteSpace($VcpkgTriplet)) {
        $configureArgs += "-DVCPKG_TARGET_TRIPLET=$VcpkgTriplet"
    }

    Invoke-Checked cmake @configureArgs
    Invoke-Checked cmake --build $BuildDir --config $BuildType
}
finally {
    Pop-Location
}
