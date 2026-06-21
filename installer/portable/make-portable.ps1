<#
.SYNOPSIS
  Assemble a portable (no-installer) Veyra Sounds build and zip it.

.DESCRIPTION
  Collects the four binaries, the runtime resources the app reads from disk
  (localization catalogs), the optional APO dev-registration helpers, the
  license, and a portable README into a staging folder, then compresses it to
  veyra-portable-<version>-x64.zip. Fonts/icons are embedded in the executables,
  so they aren't copied. Run from the repo root after a release build.

.EXAMPLE
  pwsh installer/portable/make-portable.ps1 -BinDir build/windows-release/bin -Version 0.3.0
#>
param(
    [string]$BinDir   = "build/windows-release/bin",
    [string]$RepoRoot = ".",
    [string]$OutDir   = "dist-portable",
    [string]$Version  = "0.0.0"
)

$ErrorActionPreference = "Stop"

$stage = Join-Path $OutDir "Veyra-Sounds"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force $stage | Out-Null

# --- Binaries (search recursively; JUCE may nest veyra.exe under *_artefacts) ---
foreach ($b in @("veyra.exe", "veyra-service.exe", "veyra-apo.dll", "veyra-overlay.exe")) {
    $hit = Get-ChildItem -Path $BinDir -Recurse -Filter $b -File -ErrorAction SilentlyContinue |
           Select-Object -First 1
    if (-not $hit) { Write-Error "Portable: missing binary '$b' under $BinDir"; exit 1 }
    Copy-Item $hit.FullName $stage
}

# --- Runtime resources read from disk: localization catalogs ---
$lang = Join-Path $RepoRoot "resources/lang"
if (Test-Path $lang) {
    $dst = Join-Path $stage "resources/lang"
    New-Item -ItemType Directory -Force $dst | Out-Null
    Copy-Item (Join-Path $lang "*.json") $dst -ErrorAction SilentlyContinue
}

# --- Measured MIT KEMAR HRTF set (default spatial); ~0.9 MB ---
$hrtf = Join-Path $RepoRoot "third_party/hrtf/mit_kemar/diffuse"
if (Test-Path $hrtf) {
    $dst = Join-Path $stage "hrtf/diffuse"
    New-Item -ItemType Directory -Force $dst | Out-Null
    Copy-Item (Join-Path $hrtf "*") $dst -Recurse -Force
}

# --- Optional APO developer-registration helpers ---
$drv = Join-Path $RepoRoot "installer/driver"
if (Test-Path $drv) {
    $dst = Join-Path $stage "driver"
    New-Item -ItemType Directory -Force $dst | Out-Null
    foreach ($f in @("register-apo.ps1", "veyra_apo.inf")) {
        $p = Join-Path $drv $f
        if (Test-Path $p) { Copy-Item $p $dst }
    }
}

# --- License + portable README ---
$license = Join-Path $RepoRoot "LICENSE"
if (Test-Path $license) { Copy-Item $license $stage }

@"
Veyra Sounds — Portable $Version (x64)

This is a portable build: no installer, no admin needed for the basics.

QUICK START
  1. Run veyra-service.exe       (the background audio service)
  2. Run veyra.exe               (the app) - the brand LED turns green when connected
  3. To HEAR effects without a driver: install a virtual audio cable
     (e.g. VB-CABLE), set it as the Windows default output, then in the app go
     to Devices -> Audio Bridge: Source = the cable, Output = your headphones,
     Enable. Play audio and adjust EQ / Spatial / Loudness live.

CONTENTS
  veyra.exe            the app (UI)
  veyra-service.exe    the audio service (run this first)
  veyra-apo.dll        the system audio effect (optional, advanced - see driver/)
  veyra-overlay.exe    the Gamer Mode radar HUD
  resources/lang/      localization catalogs
  driver/              optional APO dev-registration (test-signing required)

Config + logs live under %APPDATA%\Veyra. Veyra Sounds is free software (GPLv3);
see LICENSE.
"@ | Set-Content (Join-Path $stage "README-PORTABLE.txt") -Encoding utf8

# --- Zip it ---
New-Item -ItemType Directory -Force $OutDir | Out-Null
$zip = Join-Path $OutDir "veyra-portable-$Version-x64.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip -Force
Write-Host "Portable package: $zip"
