# Deploy CommanderMarkers.dll to a local GW2 addons folder.
#
# Usage:
#   .\scripts\deploy-to-gw2.ps1
#   .\scripts\deploy-to-gw2.ps1 -Release
#   .\scripts\deploy-to-gw2.ps1 -LocalTest
#   .\scripts\deploy-to-gw2.ps1 -LocalTest -Release   # invalid, rejected
param(
    [switch]$Release,
    [switch]$LocalTest
)

$ErrorActionPreference = 'Stop'

if ($LocalTest -and $Release) {
    Write-Error '--LocalTest cannot be combined with -Release'
    exit 1
}

$Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$BuildDir = Join-Path $Root 'build'
$DistDir = Join-Path $Root 'dist'
$DllName = 'CommanderMarkers.dll'

function Resolve-Gw2AddonsDir {
    if ($env:GW2_ADDONS_DIR) {
        return $env:GW2_ADDONS_DIR
    }

    $blishLaunch = Join-Path (Split-Path $Root -Parent) 'BlishHud-CommanderMarkers/Properties/launchSettings.json'
    if (Test-Path -LiteralPath $blishLaunch) {
        $launch = Get-Content -LiteralPath $blishLaunch -Raw | ConvertFrom-Json
        foreach ($name in @('gw2', 'powershell', 'mumblelink1')) {
            $wd = $launch.profiles.$name.workingDirectory
            if ($wd -and (Test-Path -LiteralPath $wd)) {
                return (Split-Path -Parent $wd)
            }
        }
    }

    foreach ($candidate in @(
            'E:\Games\Guild Wars 2\addons',
            'C:\Program Files\Guild Wars 2\addons',
            'C:\Program Files (x86)\Guild Wars 2\addons'
        )) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    Write-Error 'GW2 addons folder not found. Set `$env:GW2_ADDONS_DIR` to your GW2/addons path.'
    exit 1
}

function Ensure-Command {
    param([string]$Name)
    if (-not (Get-Command $Name -ErrorAction SilentlyContinue)) {
        Write-Error "Required command not found: $Name"
        exit 1
    }
}

function Resolve-PythonExecutable {
    foreach ($name in @('python', 'python3')) {
        $matches = Get-Command $name -All -ErrorAction SilentlyContinue |
            Where-Object { $_.Source -notmatch '\\msys64\\(usr|home)\\' -and $_.Source -notmatch '/usr/bin/' }
        if ($matches) {
            return ($matches | Select-Object -First 1).Source
        }
    }

    foreach ($name in @('python', 'python3')) {
        $cmd = Get-Command $name -ErrorAction SilentlyContinue
        if ($cmd) {
            return $cmd.Source
        }
    }

    return $null
}

function Initialize-MingwToolchain {
    $msysRoots = @($env:MSYS2_ROOT, 'C:\msys64', 'C:\tools\msys64') |
        Where-Object { $_ -and (Test-Path -LiteralPath $_) } |
        Select-Object -Unique

    foreach ($root in $msysRoots) {
        foreach ($envName in @('mingw64', 'ucrt64', 'clang64')) {
            $bin = Join-Path $root "$envName/bin"
            $gpp = Join-Path $bin 'g++.exe'
            $gcc = Join-Path $bin 'gcc.exe'
            if ((Test-Path -LiteralPath $gpp) -and (Test-Path -LiteralPath $gcc)) {
                $usrBin = Join-Path $root 'usr/bin'
                $env:PATH = "$bin;$usrBin;$env:PATH"
                return [PSCustomObject]@{
                    Gcc  = $gcc
                    Gpp  = $gpp
                    Bin  = $bin
                    Root = $root
                    Env  = $envName
                }
            }
        }
    }

    $gppCmd = Get-Command g++ -ErrorAction SilentlyContinue
    $gccCmd = Get-Command gcc -ErrorAction SilentlyContinue
    if ($gppCmd -and $gccCmd) {
        return [PSCustomObject]@{
            Gcc = $gccCmd.Source
            Gpp = $gppCmd.Source
            Bin = Split-Path -Parent $gppCmd.Source
        }
    }

    return $null
}

function Reset-StaleCmakeBuild {
    param(
        [string]$BuildDir,
        [string]$Root
    )

    $cacheFile = Join-Path $BuildDir 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cacheFile)) {
        return
    }

    $cache = Get-Content -LiteralPath $cacheFile -Raw
    $currentRoot = ($Root -replace '\\', '/').TrimEnd('/')
    $cachedRoot = ''
    $stale = $false
    if ($cache -match 'CMAKE_HOME_DIRECTORY:INTERNAL=([^\r\n]+)') {
        $cachedRoot = $Matches[1].Trim().TrimEnd('/')
    }

    if ($cachedRoot -and ($cachedRoot -ne $currentRoot)) {
        $stale = $true
    }
    if ($cache -match 'CMAKE_CXX_COMPILER:.*?=([^\r\n]+)') {
        $cachedCompiler = $Matches[1].Trim()
        if ($cachedCompiler -and -not (Test-Path -LiteralPath $cachedCompiler)) {
            $stale = $true
        }
    }

    if ($stale) {
        if ($cachedRoot) {
            Write-Host "-> Removing stale CMake cache (configured for $cachedRoot)"
        } else {
            Write-Host '-> Removing stale CMake cache (missing compiler from prior configure)'
        }
        Remove-Item -LiteralPath $BuildDir -Recurse -Force
    }
}

$PythonExe = Resolve-PythonExecutable
if (-not $PythonExe) {
    Write-Error 'Python not found. Install Python 3 for Windows from https://www.python.org/ (avoid relying on MSYS python3 for builds).'
    exit 1
}

$Toolchain = Initialize-MingwToolchain
if (-not $Toolchain) {
    Write-Error @"
MinGW g++ not found on PATH.

Install MSYS2 from https://www.msys2.org/, open the MSYS2 MinGW x64 shell, then run:
  pacman -S --needed mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja python

Re-run this script from PowerShell.
"@
    exit 1
}

Ensure-Command cmake
Ensure-Command ninja

Write-Host "-> Using MinGW from $($Toolchain.Gpp)"
Write-Host "-> Using Python from $PythonExe"

if ($LocalTest) {
    Reset-StaleCmakeBuild -BuildDir $BuildDir -Root $Root

    Write-Host '-> Configuring local-test build (CM server http://localhost:3000)...'
    $cmakeArgs = @(
        '-B', $BuildDir,
        '-G', 'Ninja',
        "-DCMAKE_C_COMPILER=$($Toolchain.Gcc)",
        "-DCMAKE_CXX_COMPILER=$($Toolchain.Gpp)",
        "-DPython3_EXECUTABLE=$PythonExe",
        '-DCMAKE_BUILD_TYPE=Debug',
        '-DCM_LOCAL_TEST=ON',
        '-DNEXUSCOMMANDERMARKERS_CROSS_COMPILE=OFF',
        $Root
    )
    $ninja = Join-Path $Toolchain.Bin 'ninja.exe'
    if (Test-Path -LiteralPath $ninja) {
        $cmakeArgs += "-DCMAKE_MAKE_PROGRAM=$ninja"
    }
    & cmake @cmakeArgs
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

    Write-Host "-> Building $DllName..."
    & cmake --build $BuildDir
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if ($Release) {
    $dllSrc = Join-Path $DistDir $DllName
    if (-not (Test-Path -LiteralPath $dllSrc)) {
        Write-Error "$dllSrc not found. Run ./scripts/build-release.sh first (or build a release export on Linux)."
        exit 1
    }
} else {
    $dllSrc = Join-Path $BuildDir $DllName
    if (-not (Test-Path -LiteralPath $dllSrc)) {
        Write-Error "$dllSrc not found. Build first, or pass -LocalTest to configure and build."
        exit 1
    }
}

$addonsDir = Resolve-Gw2AddonsDir
New-Item -ItemType Directory -Force -Path $addonsDir | Out-Null
Copy-Item -LiteralPath $dllSrc -Destination (Join-Path $addonsDir $DllName) -Force

Write-Host "-> Deployed $DllName to $addonsDir"
if ($LocalTest) {
    Write-Host '-> Local test: ensure ggg server is running at http://localhost:3000 (.\scripts\dev.ps1 in ggg/server)'
}
