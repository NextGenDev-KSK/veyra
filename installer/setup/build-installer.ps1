<#
.SYNOPSIS
  Build the Veyra Sounds end-user NSIS installer.

.DESCRIPTION
  1. Assembles a staging/ directory with all required files.
  2. Calls makensis to produce veyra-sounds-setup-{Version}-x64.exe.

  Prerequisites:
    - NSIS 3.x installed (makensis on PATH, or specify -NsisDir)
    - A completed release build under BinDir
    - Run from the repo root (or pass -RepoRoot)

.PARAMETER BinDir
  Directory containing the four compiled binaries. Searched recursively.

.PARAMETER Version
  Product version string (e.g. "0.9.0"). Defaults to the version in CMakeLists.txt.

.PARAMETER RepoRoot
  Repository root. Defaults to the current directory.

.PARAMETER OutDir
  Where to write the installer .exe. Defaults to dist-setup/.

.PARAMETER NsisDir
  Directory containing makensis.exe. Defaults to the NSIS default install path.

.EXAMPLE
  pwsh installer/setup/build-installer.ps1 -BinDir build/windows-release/bin -Version 0.9.0
#>
param(
    [string]$BinDir   = "build/windows-release/bin",
    [string]$Version  = "",
    [string]$RepoRoot = ".",
    [string]$OutDir   = "dist-setup",
    [string]$NsisDir  = "C:\Program Files (x86)\NSIS"
)

$ErrorActionPreference = "Stop"

# ── Auto-detect version from CMakeLists.txt if not specified ─────────────────
if (-not $Version) {
    $m = Select-String -Path (Join-Path $RepoRoot "CMakeLists.txt") `
             -Pattern 'set\(VEYRA_VERSION\s+([\d.]+)' | Select-Object -First 1
    if ($m) {
        $Version = $m.Matches[0].Groups[1].Value
        Write-Host "Version from CMakeLists.txt: $Version"
    } else {
        Write-Error "Could not detect version from CMakeLists.txt. Pass -Version explicitly."
    }
}

# ── Locate makensis ───────────────────────────────────────────────────────────
$makensis = Join-Path $NsisDir "makensis.exe"
if (-not (Test-Path $makensis)) {
    # Try PATH
    $makensis = (Get-Command "makensis.exe" -ErrorAction SilentlyContinue)?.Source
    if (-not $makensis) {
        Write-Error "makensis.exe not found. Install NSIS 3.x from https://nsis.sourceforge.io or pass -NsisDir."
    }
}
Write-Host "makensis: $makensis"

# ── Stage directory ───────────────────────────────────────────────────────────
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$stage = Join-Path $scriptDir "staging"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force $stage | Out-Null

# Required binaries
foreach ($b in @("veyra.exe", "veyra-service.exe", "veyra-apo.dll", "veyra-overlay.exe")) {
    $hit = Get-ChildItem -Path $BinDir -Recurse -Filter $b -File -ErrorAction SilentlyContinue |
           Select-Object -First 1
    if (-not $hit) { Write-Error "Missing binary '$b' under $BinDir" }
    Copy-Item $hit.FullName $stage
    Write-Host "  $b -> staging/"
}

# LICENSE
$license = Join-Path $RepoRoot "LICENSE"
if (Test-Path $license) { Copy-Item $license $stage }

# INSTALLATION.txt (Quick Start guide bundled in the installer)
$tmpReadme = Join-Path $stage "INSTALLATION.txt"
if (-not (Test-Path $tmpReadme)) {
    @"
Veyra Sounds $Version — Quick Start

Veyra is running. Here's how to get started:

1. The brand LED in the top-left turns GREEN when the service is connected.
   If it stays AMBER, open the Devices screen and check the service status.

2. Audio is being processed on the output device you selected during setup.
   To change it: Devices screen -> Preferred Output.

3. The effect is live immediately. Open any app and play audio —
   the EQ, compressor, and spatial audio are already active.

4. To set up the APO on an additional device (e.g. after buying new headphones):
   Start -> Veyra Sounds -> Setup Audio Driver (Advanced)
   This runs once and takes about 30 seconds.

5. If you use Bluetooth headphones that do not work with the APO:
   Devices -> Audio Bridge -> enable it for Bluetooth compatibility mode.

Config and logs: %ProgramData%\Veyra
Need help? https://github.com/NextGenDev-KSK/veyra
"@ | Set-Content $tmpReadme -Encoding utf8
}

# Setup audio driver helper
Copy-Item (Join-Path $scriptDir "setup-audio-driver.cmd") $stage

# APO driver scripts
$drvSrc = Join-Path $RepoRoot "installer/driver"
$drvDst = Join-Path $stage "driver"
New-Item -ItemType Directory -Force $drvDst | Out-Null
foreach ($f in @("register-apo.ps1", "associate-apo.ps1", "uninstall-apo.ps1", "apo-helper.ps1")) {
    $p = Join-Path $drvSrc $f
    # apo-helper.ps1 lives in installer/setup/ (installer-specific helper)
    if ($f -eq "apo-helper.ps1") { $p = Join-Path $scriptDir "apo-helper.ps1" }
    if (Test-Path $p) { Copy-Item $p $drvDst; Write-Host "  driver/$f -> staging/driver/" }
}

# HRTF data
$hrtf = Join-Path $RepoRoot "third_party/hrtf/mit_kemar/diffuse"
if (Test-Path $hrtf) {
    $dst = Join-Path $stage "hrtf/diffuse"
    New-Item -ItemType Directory -Force $dst | Out-Null
    Copy-Item (Join-Path $hrtf "*") $dst -Recurse -Force
    Write-Host "  hrtf/diffuse/ -> staging/hrtf/diffuse/"
}

# Localisation catalogs
$lang = Join-Path $RepoRoot "resources/lang"
if (Test-Path $lang) {
    $dst = Join-Path $stage "resources/lang"
    New-Item -ItemType Directory -Force $dst | Out-Null
    Copy-Item (Join-Path $lang "*.json") $dst -ErrorAction SilentlyContinue
}

# Themes
$themes = Join-Path $RepoRoot "resources/themes"
if (Test-Path $themes) {
    $dst = Join-Path $stage "resources/themes"
    New-Item -ItemType Directory -Force $dst | Out-Null
    Copy-Item (Join-Path $themes "*.json") $dst -Recurse -ErrorAction SilentlyContinue
    Write-Host "  resources/themes/ -> staging/resources/themes/"
}

# AutoEQ profiles
$autoeq = Join-Path $RepoRoot "resources/autoeq"
if (Test-Path $autoeq) {
    $dst = Join-Path $stage "resources/autoeq"
    New-Item -ItemType Directory -Force $dst | Out-Null
    Copy-Item (Join-Path $autoeq "*") $dst -Recurse -Force -ErrorAction SilentlyContinue
    Write-Host "  resources/autoeq/ -> staging/resources/autoeq/"
}

# ── Run makensis ──────────────────────────────────────────────────────────────
New-Item -ItemType Directory -Force $OutDir | Out-Null

$nsi = Join-Path $scriptDir "veyra-setup.nsi"
Write-Host ""
Write-Host "Running makensis..."
& $makensis /DVERSION=$Version /DOUTDIR=$OutDir $nsi
if ($LASTEXITCODE -ne 0) {
    Write-Error "makensis failed with exit code $LASTEXITCODE"
}

$exe = Get-ChildItem $OutDir -Filter "veyra-sounds-setup-$Version-x64.exe" | Select-Object -First 1
if ($exe) {
    Write-Host ""
    Write-Host "Installer: $($exe.FullName)"
} else {
    Write-Warning "makensis succeeded but installer file not found in $OutDir."
}
