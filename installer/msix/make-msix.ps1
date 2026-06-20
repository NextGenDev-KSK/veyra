<#
.SYNOPSIS
  Stage + pack (and optionally sign) a Veyra Sounds MSIX.

.DESCRIPTION
  Assembles the binaries + runtime resources + manifest + logo assets into a
  layout, then runs MakeAppx (Windows SDK) to produce veyra-<version>.msix.
  With -CertPath it also signs via SignTool. MakeAppx/SignTool ship in the
  Windows 10/11 SDK (…\bin\<ver>\x64\); run from a Developer prompt or pass
  -SdkBin. See installer/SIGNING.md.

.EXAMPLE
  pwsh installer/msix/make-msix.ps1 -BinDir build/windows-release/bin -Version 0.3.0
#>
param(
    [string]$BinDir   = "build/windows-release/bin",
    [string]$RepoRoot = ".",
    [string]$OutDir   = "dist-msix",
    [string]$Version  = "0.3.0",
    [string]$SdkBin   = "",            # optional explicit path to the SDK bin\x64
    [string]$CertPath = "",            # optional .pfx for signing
    [string]$CertPassword = ""
)

$ErrorActionPreference = "Stop"

$layout = Join-Path $OutDir "layout"
if (Test-Path $layout) { Remove-Item -Recurse -Force $layout }
New-Item -ItemType Directory -Force (Join-Path $layout "Assets") | Out-Null

# Binaries
foreach ($b in @("veyra.exe", "veyra-service.exe", "veyra-apo.dll", "veyra-overlay.exe")) {
    $hit = Get-ChildItem -Path $BinDir -Recurse -Filter $b -File -ErrorAction SilentlyContinue | Select-Object -First 1
    if (-not $hit) { Write-Error "MSIX: missing binary '$b'"; exit 1 }
    Copy-Item $hit.FullName $layout
}

# Runtime resources
$lang = Join-Path $RepoRoot "resources/lang"
if (Test-Path $lang) {
    New-Item -ItemType Directory -Force (Join-Path $layout "resources/lang") | Out-Null
    Copy-Item (Join-Path $lang "*.json") (Join-Path $layout "resources/lang") -ErrorAction SilentlyContinue
}

# Manifest (stamp version) + logo assets (Windows scales the single square mark).
$manifest = Get-Content (Join-Path $RepoRoot "installer/msix/AppxManifest.xml") -Raw
$manifest = $manifest -replace 'Version="[0-9.]+"', "Version=`"$Version.0`""
$manifest | Set-Content (Join-Path $layout "AppxManifest.xml") -Encoding utf8

$icon = Join-Path $RepoRoot "resources/icons/Veyra_Icon_square.png"
foreach ($a in @("StoreLogo.png", "Square150x150Logo.png", "Square44x44Logo.png")) {
    if (Test-Path $icon) { Copy-Item $icon (Join-Path $layout "Assets/$a") }
}

# Locate MakeAppx / SignTool.
function Find-SdkTool($name) {
    if ($SdkBin -and (Test-Path (Join-Path $SdkBin $name))) { return (Join-Path $SdkBin $name) }
    $cmd = Get-Command $name -ErrorAction SilentlyContinue
    if ($cmd) { return $cmd.Source }
    $root = "C:\Program Files (x86)\Windows Kits\10\bin"
    $hit = Get-ChildItem -Path $root -Recurse -Filter $name -File -ErrorAction SilentlyContinue |
           Where-Object { $_.FullName -match "x64" } | Sort-Object FullName -Descending | Select-Object -First 1
    if ($hit) { return $hit.FullName }
    return $null
}

$makeappx = Find-SdkTool "MakeAppx.exe"
if (-not $makeappx) { Write-Error "MakeAppx.exe not found - install the Windows SDK or pass -SdkBin."; exit 1 }

New-Item -ItemType Directory -Force $OutDir | Out-Null
$msix = Join-Path $OutDir "veyra-$Version-x64.msix"
& $makeappx pack /d $layout /p $msix /o
if ($LASTEXITCODE -ne 0) { Write-Error "MakeAppx failed ($LASTEXITCODE)."; exit 1 }
Write-Host "MSIX package: $msix"

if ($CertPath) {
    $signtool = Find-SdkTool "signtool.exe"
    if (-not $signtool) { Write-Error "signtool.exe not found."; exit 1 }
    & $signtool sign /fd SHA256 /a /f $CertPath /p $CertPassword $msix
    if ($LASTEXITCODE -ne 0) { Write-Error "signtool failed ($LASTEXITCODE)."; exit 1 }
    Write-Host "Signed: $msix"
} else {
    Write-Host "Unsigned. Sign with -CertPath, or test-install after trusting a self-signed cert (see installer/SIGNING.md)."
}
