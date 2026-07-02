<#
.SYNOPSIS
  Associate or remove the Veyra APO from an audio endpoint's FxProperties.

.DESCRIPTION
  Writes PKEY_FX_PostMixEffectClsid into the endpoint's FxProperties registry
  key so audiodg.exe loads veyra-apo.dll as the post-mix effect on that device.

  This is STEP 2 in developer APO setup; run register-apo.ps1 (COM registration)
  first, and ensure test-signing is enabled (bcdedit /set testsigning on + reboot).

  Run from an ELEVATED PowerShell prompt.

.PARAMETER EndpointGuid
  The {xxxxxxxx-xxxx-...} MMDevice GUID of the target endpoint.
  Omit to list all endpoints and pick interactively.

.PARAMETER Capture
  Target the capture (microphone) chain instead of the render chain.
  Associates VeyraMicApo ({B2D4E6F8-...}) instead of VeyraApoEfx ({7E9C2B14-...}).

.PARAMETER Unassociate
  Remove the APO association instead of writing it.

.PARAMETER ListEndpoints
  List all audio endpoints (render + capture) and show which ones currently
  have a Veyra CLSID in their FxProperties. Does not modify anything.
  Useful for verifying the state after install or uninstall.

.EXAMPLE
  # List render endpoints and pick one interactively:
  .\associate-apo.ps1

  # Associate directly by GUID (skip the prompt):
  .\associate-apo.ps1 -EndpointGuid "{1234abcd-5678-...}"

  # Remove association:
  .\associate-apo.ps1 -EndpointGuid "{1234abcd-5678-...}" -Unassociate

  # Associate the mic APO with a capture endpoint:
  .\associate-apo.ps1 -Capture

  # Show current Veyra APO state on all endpoints (post-uninstall verify):
  .\associate-apo.ps1 -ListEndpoints
#>
param(
    [string]$EndpointGuid = "",
    [switch]$Capture,
    [switch]$Unassociate,
    [switch]$ListEndpoints
)

$ErrorActionPreference = "Stop"

# ── Constants ─────────────────────────────────────────────────────────────────

# FMTID {D04E05A6-594B-4FB6-A80D-01AF5EED7D1D} — note ED7D1D, NOT the old EC11D9
# (a wrong-GUID key is silently ignored by audiodg). Render endpoints use
# PKEY_FX_ModeEffectClsid (PID 6) — the slot audiodg reads for every endpoint
# type, including Bluetooth A2DP. Capture uses PKEY_FX_PreMixEffectClsid (PID 1).
# This matches what VeyraSetupHelper.exe writes during installation.
$RenderClsid  = "{7E9C2B14-3F6A-4D8E-9B21-5C0A1F2E3D44}"   # VeyraApoEfx
$CaptureClsid = "{B2D4E6F8-1A3C-4E5D-8F09-2B4C6D8E0A12}"   # VeyraMicApo
$FxKeyName    = "{D04E05A6-594B-4FB6-A80D-01AF5EED7D1D},6"                  # render: ModeEffect
if ($Capture) { $FxKeyName = "{D04E05A6-594B-4FB6-A80D-01AF5EED7D1D},1" }   # capture: PreMix

# PKEY_Device_DeviceDesc = {a45c254e-df1c-4efd-8020-67d146a850e0},2 ("Speakers" etc.)
$FriendlyNameKey = "{a45c254e-df1c-4efd-8020-67d146a850e0},2"

$TargetClsid = if ($Capture) { $CaptureClsid } else { $RenderClsid }
$ApoName     = if ($Capture) { "VeyraMicApo (capture)"  } else { "VeyraApoEfx (render)" }
$Flow        = if ($Capture) { "Capture" } else { "Render" }
$MmBase      = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\$Flow"

# ── Helpers ───────────────────────────────────────────────────────────────────

function Get-FriendlyName([string]$guid) {
    try {
        $p = Join-Path $MmBase "$guid\Properties"
        return (Get-ItemPropertyValue -Path $p -Name $FriendlyNameKey -EA Stop)
    } catch {
        return $guid
    }
}

function Get-Endpoints() {
    if (-not (Test-Path $MmBase)) { return @() }
    return @(Get-ChildItem $MmBase | ForEach-Object {
        [PSCustomObject]@{ Guid = $_.PSChildName; Name = Get-FriendlyName $_.PSChildName }
    })
}

# ── Admin check ───────────────────────────────────────────────────────────────

$identity = [Security.Principal.WindowsIdentity]::GetCurrent()
$principal = New-Object Security.Principal.WindowsPrincipal $identity
if (-not $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
    Write-Error "This script must be run as Administrator (the FxProperties key requires HKLM write access)."
    exit 1
}

# ── List-endpoints mode (diagnostic; read-only) ───────────────────────────────

if ($ListEndpoints) {
    $PreMixKey   = "{D04E05A6-594B-4FB6-A80D-01AF5EED7D1D},1"  # PKEY_FX_PreMixEffectClsid (capture)
    $PostMixKey  = "{D04E05A6-594B-4FB6-A80D-01AF5EED7D1D},6"  # PKEY_FX_ModeEffectClsid (render)
    $veyraClsids = @($RenderClsid, $CaptureClsid)
    Write-Host ""
    Write-Host "Audio endpoints — Veyra APO association status:"
    Write-Host ""
    foreach ($f in @("Render", "Capture")) {
        $base = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\$f"
        if (-not (Test-Path $base)) { continue }
        Get-ChildItem $base | ForEach-Object {
            $guid = $_.PSChildName
            $name = try { Get-ItemPropertyValue (Join-Path $base "$guid\Properties") $FriendlyNameKey -EA Stop } catch { $guid }
            $fxp  = Join-Path $_.PSPath "FxProperties"
            $hits = @()
            if (Test-Path $fxp) {
                foreach ($key in @($PostMixKey, $PreMixKey)) {
                    $v = Get-ItemProperty -Path $fxp -Name $key -EA SilentlyContinue
                    if ($v -and ($veyraClsids -contains $v.$key)) { $hits += $v.$key }
                }
            }
            $status = if ($hits.Count -gt 0) { "VEYRA APO PRESENT: $($hits -join ', ')" } else { "no Veyra APO" }
            Write-Host ("  [{0}] {1}" -f $f, $name)
            Write-Host ("         {0}  ->  {1}" -f $guid, $status)
        }
    }
    Write-Host ""
    exit 0
}

# ── Resolve endpoint GUID ─────────────────────────────────────────────────────

if (-not $EndpointGuid) {
    $endpoints = Get-Endpoints
    if ($endpoints.Count -eq 0) {
        Write-Error "No $Flow endpoints found under $MmBase"
        exit 1
    }
    Write-Host ""
    Write-Host "${Flow} endpoints on this machine:"
    for ($i = 0; $i -lt $endpoints.Count; $i++) {
        Write-Host ("  [{0}] {1}" -f ($i + 1), $endpoints[$i].Name)
        Write-Host ("       {0}" -f $endpoints[$i].Guid)
    }
    Write-Host ""
    $choice   = Read-Host "Enter number (1-$($endpoints.Count))"
    $idx      = [int]$choice - 1
    if ($idx -lt 0 -or $idx -ge $endpoints.Count) {
        Write-Error "Invalid selection."
        exit 1
    }
    $EndpointGuid = $endpoints[$idx].Guid
}

$endpointName = Get-FriendlyName $EndpointGuid
$fxPath       = Join-Path $MmBase "$EndpointGuid\FxProperties"

# Sanity: the endpoint GUID must exist in the registry.
$endpointPath = Join-Path $MmBase $EndpointGuid
if (-not (Test-Path $endpointPath)) {
    Write-Error "Endpoint GUID not found: $EndpointGuid`nLook up a valid GUID with: Get-ChildItem '$MmBase'"
    exit 1
}

# ── Write / remove FxProperties entry ────────────────────────────────────────

Write-Host ""
if ($Unassociate) {
    Write-Host "Removing Veyra APO from: $endpointName"
    Write-Host "  Path: $fxPath"
    if (Test-Path $fxPath) {
        $existing = Get-ItemProperty -Path $fxPath -Name $FxKeyName -EA SilentlyContinue
        if ($existing) {
            Remove-ItemProperty -Path $fxPath -Name $FxKeyName
            Write-Host "  Removed: $FxKeyName"
        } else {
            Write-Host "  Key not present — nothing to remove."
        }
    } else {
        Write-Host "  FxProperties subkey does not exist — nothing to remove."
    }
} else {
    Write-Host "Associating $ApoName"
    Write-Host "  Endpoint : $endpointName ($EndpointGuid)"
    Write-Host "  Registry : $fxPath"
    Write-Host "  Key      : $FxKeyName"
    Write-Host "  Value    : $TargetClsid"

    if (-not (Test-Path $fxPath)) {
        New-Item -Path $fxPath -Force | Out-Null
        Write-Host "  Created FxProperties subkey."
    }
    Set-ItemProperty -Path $fxPath -Name $FxKeyName -Value $TargetClsid -Type String
    Write-Host "  Written."
}

# ── Reload audiodg by restarting Windows Audio ────────────────────────────────

Write-Host ""
Write-Host "Restarting Windows Audio (AudioSrv) to reload the APO chain..."
try {
    # AudioEndpointBuilder is a dependency; stopping AudioSrv takes it down too.
    Stop-Service  -Name AudioSrv -Force -EA Stop
    Start-Service -Name AudioEndpointBuilder -EA Stop
    Start-Service -Name AudioSrv -EA Stop
    Write-Host "  Done."
} catch {
    Write-Warning "Could not restart AudioSrv automatically: $_"
    Write-Host "  Manual fallback: Device Manager → right-click the audio device → Disable → Enable."
}

# ── Post-association checklist ────────────────────────────────────────────────

Write-Host ""
if ($Unassociate) {
    Write-Host "Veyra APO removed. Audio will route without it until re-associated."
} else {
    Write-Host "APO associated. Validation checklist:"
    Write-Host "  [1] Start the Veyra service:"
    Write-Host "        .\..\..\build\windows-release\bin\veyra-service.exe --console"
    Write-Host "        (Service must be running so the APO finds the shared parameter block.)"
    Write-Host "  [2] Play audio through: $endpointName"
    Write-Host "  [3] Confirm veyra-apo.dll loaded into audiodg.exe:"
    Write-Host "        Process Explorer → audiodg.exe → lower pane (DLLs) → search for veyra-apo"
    Write-Host "  [4] If the DLL is absent:"
    Write-Host "        - Check test-signing:  bcdedit /enum | findstr testsigning"
    Write-Host "        - Check COM reg:       reg query HKCR\CLSID\{7E9C2B14-3F6A-4D8E-9B21-5C0A1F2E3D44}"
    Write-Host "        - Check FxProperties:  reg query `"$fxPath`""
    Write-Host "        - Check Event Viewer:  Windows Logs\System, Source=AudioDg"
    Write-Host "  [5] To remove: .\associate-apo.ps1 -EndpointGuid `"$EndpointGuid`" -Unassociate"
}
Write-Host ""
