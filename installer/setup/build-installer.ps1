<#
.SYNOPSIS
  Build the Veyra Sounds end-user NSIS installer.

.DESCRIPTION
  1. Assembles a staging/ directory with all required files.
  2. Calls makensis to produce veyra-sounds-setup-{Version}-x64.exe.

  Compatible with Windows PowerShell 5.1 and PowerShell 7+.
  No PowerShell 7-only syntax (no ?., ??, ForEach-Object -Parallel, etc.).

  Prerequisites:
    - NSIS 3.x installed (auto-detected from common install paths, registry,
      or PATH). Pass -NsisDir to specify an exact location.
    - A completed release build under BinDir.
    - Run from the repo root (or pass -RepoRoot).

.PARAMETER BinDir
  Directory containing the compiled binaries. Searched recursively.

.PARAMETER Version
  Product version string (e.g. "1.0.0"). Defaults to the version in CMakeLists.txt.

.PARAMETER RepoRoot
  Repository root. Defaults to the current directory.

.PARAMETER OutDir
  Where to write the installer .exe. Defaults to dist-setup/.

.PARAMETER NsisDir
  Directory containing makensis.exe. If omitted, auto-detected.

.EXAMPLE
  pwsh installer/setup/build-installer.ps1 -BinDir build/windows-release/bin
  pwsh installer/setup/build-installer.ps1 -BinDir build/windows-release/bin -Version 1.0.0
#>
param(
    [string]$BinDir   = "build/windows-release/bin",
    [string]$Version  = "",
    [string]$RepoRoot = ".",
    [string]$OutDir   = "dist-setup",
    [string]$NsisDir  = ""
)

$ErrorActionPreference = "Stop"

# ── Auto-detect version from CMakeLists.txt ───────────────────────────────────
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

# ── Locate makensis.exe ────────────────────────────────────────────────────────
# PS 5.1-compatible auto-detection: no ?. or ?? operators.

function Find-Makensis {
    param([string]$Hint)

    # 1. Caller-supplied hint
    if ($Hint) {
        $p = Join-Path $Hint "makensis.exe"
        if (Test-Path $p) { return $p }
        Write-Warning "makensis.exe not found at specified -NsisDir: $Hint"
    }

    # 2. Common install locations (x64 and x86 Program Files)
    $candidates = @(
        "C:\Program Files\NSIS\makensis.exe",
        "C:\Program Files (x86)\NSIS\makensis.exe",
        "C:\NSIS\makensis.exe"
    )
    foreach ($c in $candidates) {
        if (Test-Path $c) { return $c }
    }

    # 3. Registry (NSIS writes InstallDir to HKLM)
    foreach ($hive in @("HKLM:\SOFTWARE\NSIS", "HKLM:\SOFTWARE\WOW6432Node\NSIS")) {
        if (Test-Path $hive) {
            try {
                $nsisDir = (Get-ItemPropertyValue -Path $hive -Name "(default)" -ErrorAction Stop)
                $p = Join-Path $nsisDir "makensis.exe"
                if (Test-Path $p) { return $p }
            } catch { }
        }
    }

    # 4. PATH (PS 5.1 compatible: avoid ?.Source)
    $cmd = Get-Command "makensis.exe" -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }

    return $null
}

$makensis = Find-Makensis -Hint $NsisDir
if (-not $makensis) {
    Write-Error @"
makensis.exe not found.

Install NSIS 3.x from https://nsis.sourceforge.io/Download
Then re-run this script. To specify the install location manually:
  pwsh installer/setup/build-installer.ps1 -NsisDir "C:\Your\NSIS\path"
"@
}
Write-Host "makensis: $makensis"

# ── Stage directory ────────────────────────────────────────────────────────────
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$stage = Join-Path $scriptDir "staging"
if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
New-Item -ItemType Directory -Force $stage | Out-Null
Write-Host "Staging: $stage"

# Required binaries (searched recursively under BinDir)
foreach ($b in @("veyra.exe", "veyra-service.exe", "veyra-apo.dll",
                 "veyra-overlay.exe", "VeyraSetupHelper.exe")) {
    $hit = Get-ChildItem -Path $BinDir -Recurse -Filter $b -File -ErrorAction SilentlyContinue |
           Select-Object -First 1
    if (-not $hit) {
        Write-Error "Missing binary '$b' under $BinDir. Build the project first (cmake --build --preset windows-release)."
    }
    Copy-Item $hit.FullName $stage
    Write-Host "  $b -> staging/"
}

# LICENSE
$license = Join-Path $RepoRoot "LICENSE"
if (Test-Path $license) { Copy-Item $license $stage }

# INSTALLATION.txt (quick-start guide embedded in the installer)
$installTxt = Join-Path $stage "INSTALLATION.txt"
@"
Veyra Sounds $Version — Quick Start

Veyra is running. The brand LED turns GREEN when the app is connected to the
Veyra service.

1. To HEAR the effects on this release, set up the Audio Bridge:
   https://github.com/NextGenDev-KSK/veyra/blob/main/docs/AUDIO_BRIDGE.md
   (This open-source build is unsigned, and Windows only loads signed audio
   processing objects — so the low-latency APO path stays inactive until a
   signed release. The Bridge runs the identical DSP in the Veyra service.)

2. To change your playback device:
   Open Veyra -> Devices -> Preferred Output.

3. To re-run the audio driver setup (e.g. after buying new headphones):
   Start -> Veyra Sounds -> Setup Audio Driver (Advanced)

Config and logs: %ProgramData%\Veyra
Help: https://github.com/NextGenDev-KSK/veyra
"@ | Set-Content $installTxt -Encoding utf8
Write-Host "  INSTALLATION.txt -> staging/"

# setup-audio-driver.cmd
$cmdSrc = Join-Path $scriptDir "setup-audio-driver.cmd"
if (Test-Path $cmdSrc) {
    Copy-Item $cmdSrc $stage
    Write-Host "  setup-audio-driver.cmd -> staging/"
}

# APO developer scripts (for the Setup Audio Driver shortcut and advanced users)
$drvSrc = Join-Path $RepoRoot "installer/driver"
$drvDst = Join-Path $stage "driver"
New-Item -ItemType Directory -Force $drvDst | Out-Null
foreach ($f in @("register-apo.ps1", "associate-apo.ps1", "uninstall-apo.ps1")) {
    $p = Join-Path $drvSrc $f
    if (Test-Path $p) {
        Copy-Item $p $drvDst
        Write-Host "  driver/$f -> staging/driver/"
    }
}
# Note: apo-helper.ps1 is no longer needed by the installer.
# VeyraSetupHelper.exe replaces it for all runtime operations.

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
    Write-Host "  resources/lang/ -> staging/resources/lang/"
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

# VC++ redistributable (optional — bundle if present)
$vcredist = Join-Path $RepoRoot "installer/redist/vc_redist.x64.exe"
$bundledVcredist = $false
if (Test-Path $vcredist) {
    Copy-Item $vcredist $stage
    Write-Host "  vc_redist.x64.exe -> staging/ (bundled)"
    $bundledVcredist = $true
}

# ── Run makensis ───────────────────────────────────────────────────────────────
New-Item -ItemType Directory -Force $OutDir | Out-Null

$nsi = Join-Path $scriptDir "veyra-setup.nsi"
Write-Host ""
Write-Host "Building installer..."
$nsiDefines = @("/DVERSION=$Version")
if ($bundledVcredist) { $nsiDefines += "/DHAVE_VCREDIST" }
& $makensis @nsiDefines $nsi
if ($LASTEXITCODE -ne 0) {
    Write-Error "makensis failed with exit code $LASTEXITCODE"
}

# Locate the output exe (NSIS writes it next to the .nsi file)
$exeName = "veyra-sounds-setup-$Version-x64.exe"
$exeSrc  = Join-Path $scriptDir $exeName
$exeDst  = Join-Path (Resolve-Path $OutDir) $exeName

if (Test-Path $exeSrc) {
    Move-Item -Force $exeSrc $exeDst
    Write-Host ""
    Write-Host "Installer: $exeDst"
} elseif (Test-Path $exeDst) {
    Write-Host ""
    Write-Host "Installer: $exeDst"
} else {
    Write-Warning "makensis succeeded but '$exeName' not found. Check makensis output above."
}
