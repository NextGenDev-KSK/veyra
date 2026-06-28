<#
.SYNOPSIS
  Fully uninstall the Veyra APO from an endpoint and remove its COM registration.

.DESCRIPTION
  Performs a complete APO removal:
    1. Removes PKEY_FX_PostMixEffectClsid / PKEY_FX_PreMixEffectClsid from every
       endpoint that has the Veyra CLSID associated (render and/or capture).
    2. Restarts Windows Audio so audiodg.exe unloads the DLL.
    3. Unregisters the COM server (HKCR\CLSID entries) via regsvr32 /u.
    4. Optionally deletes the DLL file.

  This is the inverse of the developer setup described in BUILD_GUIDE.md §2.
  Run from an ELEVATED PowerShell prompt.

.PARAMETER DllPath
  Path to veyra-apo.dll. Required for COM unregistration (step 3).
  Defaults to the standard build output location.

.PARAMETER DeleteDll
  Delete the DLL after unregistration. Use with care.

.EXAMPLE
  .\uninstall-apo.ps1 -DllPath ..\..\build\windows-release\bin\veyra-apo.dll
#>
param(
    [string]$DllPath = "..\..\build\windows-release\bin\veyra-apo.dll",
    [switch]$DeleteDll
)

$ErrorActionPreference = "Stop"

# ── Constants ─────────────────────────────────────────────────────────────────

$FxKeyName    = "{D04E05A6-594B-4FB6-A80D-01AF5EEC11D9},6"   # PKEY_FX_PostMixEffectClsid
$FxKeyNamePre = "{D04E05A6-594B-4FB6-A80D-01AF5EEC11D9},4"   # PKEY_FX_PreMixEffectClsid
$RenderClsid  = "{7E9C2B14-3F6A-4D8E-9B21-5C0A1F2E3D44}"     # VeyraApoEfx
$CaptureClsid = "{B2D4E6F8-1A3C-4E5D-8F09-2B4C6D8E0A12}"     # VeyraMicApo
$VeyraClsids  = @($RenderClsid, $CaptureClsid)

# ── Admin check ───────────────────────────────────────────────────────────────

$identity  = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal $identity
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "This script must be run as Administrator."
    exit 1
}

# ── Step 1: Remove FxProperties from all endpoints ───────────────────────────

Write-Host ""
Write-Host "Step 1: Removing Veyra CLSID from all audio endpoint FxProperties..."

$removed = 0
foreach ($flow in @("Render", "Capture")) {
    $mmBase = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\$flow"
    if (-not (Test-Path $mmBase)) { continue }

    Get-ChildItem $mmBase | ForEach-Object {
        $fxPath = Join-Path $_.PSPath "FxProperties"
        if (-not (Test-Path $fxPath)) { return }

        foreach ($key in @($FxKeyName, $FxKeyNamePre)) {
            $val = Get-ItemProperty -Path $fxPath -Name $key -EA SilentlyContinue
            if ($val -and ($VeyraClsids -contains $val.$key)) {
                Remove-ItemProperty -Path $fxPath -Name $key -EA SilentlyContinue
                Write-Host ("  Removed $key from $flow endpoint {0}" -f $_.PSChildName)
                $removed++
            }
        }
    }
}

if ($removed -eq 0) {
    Write-Host "  No Veyra FxProperties entries found."
} else {
    Write-Host "  $removed entr$(if ($removed -eq 1) {'y'} else {'ies'}) removed."
}

# ── Step 2: Restart Windows Audio ────────────────────────────────────────────

Write-Host ""
Write-Host "Step 2: Restarting Windows Audio to unload the APO DLL..."
try {
    Stop-Service  -Name AudioSrv -Force -EA Stop
    Start-Service -Name AudioEndpointBuilder -EA Stop
    Start-Service -Name AudioSrv -EA Stop
    Write-Host "  AudioSrv restarted."
} catch {
    Write-Warning "Could not restart AudioSrv automatically: $_"
    Write-Host "  Manual fallback: Device Manager → audio device → Disable → Enable."
}

# ── Step 3: Unregister COM server ─────────────────────────────────────────────

Write-Host ""
Write-Host "Step 3: Unregistering COM server..."

if (-not (Test-Path $DllPath)) {
    Write-Warning "DLL not found at '$DllPath' — skipping COM unregistration."
    Write-Warning "Run manually: regsvr32 /s /u <path-to-veyra-apo.dll>"
} else {
    $full = (Resolve-Path $DllPath).Path
    Write-Host "  regsvr32 /s /u $full"
    regsvr32.exe /s /u $full
    if ($LASTEXITCODE -eq 0) {
        Write-Host "  COM server unregistered."
    } else {
        Write-Warning "regsvr32 exited $LASTEXITCODE. Check HKCR\CLSID manually."
    }
}

# ── Step 4: (Optional) Delete DLL ─────────────────────────────────────────────

if ($DeleteDll -and (Test-Path $DllPath)) {
    Write-Host ""
    Write-Host "Step 4: Deleting DLL..."
    $full = (Resolve-Path $DllPath).Path
    Remove-Item $full -Force
    Write-Host "  Deleted $full"
}

# ── Done ──────────────────────────────────────────────────────────────────────

Write-Host ""
Write-Host "Veyra APO uninstalled."
Write-Host "  Verify: run associate-apo.ps1 -ListEndpoints to confirm no Veyra CLSIDs remain."
Write-Host "  Re-install: follow BUILD_GUIDE.md §2 (register + associate)."
Write-Host ""
