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
  pwsh installer/portable/make-portable.ps1 -BinDir build/windows-release/bin -Version 1.0.0
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
    foreach ($f in @("register-apo.ps1", "associate-apo.ps1", "uninstall-apo.ps1", "veyra_apo.inf")) {
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

HOW EFFECTS WORK
  Veyra processes audio via an APO (Audio Processing Object) loaded into the Windows
  audio engine — the same mechanism used by Dolby Atmos and DTS. No virtual cable or
  rerouting required.

  For an official signed release, the APO loads automatically after association.
  For developer builds from source, test-signing must be enabled first (see driver/).

  Bluetooth headphones that reject the APO can use the Audio Bridge fallback
  (Devices -> Audio Bridge) with VB-CABLE as the virtual source.

QUICK START
  1. Run veyra-service.exe       (the background audio service - run as Administrator
                                   for APO association; not required for basic use)
  2. Run veyra.exe               (the app) - the brand LED turns green when connected
  3. Play audio. Move an EQ band on the Home screen to verify effects are audible.

  DEVELOPER: To activate the APO on a dev build (test-signing required):
    a. Enable test-signing: bcdedit /set testsigning on  (admin, reboot once)
    b. From an elevated prompt: powershell driver\associate-apo.ps1
    c. Restart Windows Audio and relaunch Veyra.

CONTENTS
  veyra.exe            the app (UI)
  veyra-service.exe    the audio service (run this first)
  veyra-apo.dll        the system audio effect (APO for audiodg.exe)
  veyra-overlay.exe    the Gamer Mode radar HUD
  resources/lang/      localization catalogs
  driver/              APO developer registration scripts (test-signing required)

Config + logs live under %ProgramData%\Veyra. Veyra Sounds is free software (GPLv3);
see LICENSE. For help: https://github.com/NextGenDev-KSK/veyra
"@ | Set-Content (Join-Path $stage "README-PORTABLE.txt") -Encoding utf8

# --- Zip it ---
New-Item -ItemType Directory -Force $OutDir | Out-Null
$zip = Join-Path $OutDir "veyra-portable-$Version-x64.zip"
if (Test-Path $zip) { Remove-Item -Force $zip }
Compress-Archive -Path (Join-Path $stage "*") -DestinationPath $zip -Force
Write-Host "Portable package: $zip"
