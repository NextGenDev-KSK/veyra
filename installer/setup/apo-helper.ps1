<#
.SYNOPSIS
  Veyra Sounds — Installer APO Helper
  Called by the NSIS installer to enumerate endpoints, associate the APO,
  verify COM registration, and handle uninstall cleanup.

.PARAMETER Action
  list            Enumerate active render endpoints; write INI to OutFile.
  associate       Write the Veyra APO CLSID into an endpoint's FxProperties.
  verify-com      Exit 0 if the render CLSID is registered, 1 if not.
  check-testsign  Exit 0 if test-signing is ON, 1 if OFF.
  unassociate-all Remove Veyra CLSIDs from every render + capture endpoint.

.PARAMETER EndpointGuid
  The {xxxxxxxx-...} MMDevice GUID. Required for 'associate'.

.PARAMETER OutFile
  Output path for the 'list' action. Default: $env:TEMP\veyra-devices.ini

.PARAMETER LogFile
  Append all actions to this log. Default: $env:ProgramData\Veyra\logs\install.log
#>
param(
    [string]$Action       = "list",
    [string]$EndpointGuid = "",
    [string]$OutFile      = "$env:TEMP\veyra-devices.ini",
    [string]$LogFile      = "$env:ProgramData\Veyra\logs\install.log"
)

$ErrorActionPreference = "Stop"

# ── Constants ─────────────────────────────────────────────────────────────────
$RenderClsid   = "{7E9C2B14-3F6A-4D8E-9B21-5C0A1F2E3D44}"   # VeyraApoEfx
$CaptureClsid  = "{B2D4E6F8-1A3C-4E5D-8F09-2B4C6D8E0A12}"   # VeyraMicApo
$FxPostMixKey  = "{D04E05A6-594B-4FB6-A80D-01AF5EEC11D9},6"  # PKEY_FX_PostMixEffectClsid
$FxPreMixKey   = "{D04E05A6-594B-4FB6-A80D-01AF5EEC11D9},4"  # PKEY_FX_PreMixEffectClsid
$FriendlyKey   = "{a45c254e-df1c-4efd-8020-67d146a850e0},2"  # PKEY_Device_FriendlyName
$RenderBase    = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render"
$CaptureBase   = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Capture"

# ── Logging ───────────────────────────────────────────────────────────────────
function Write-Log {
    param([string]$Msg, [string]$Level = "INFO")
    $ts   = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    $line = "[$ts] [$Level] [apo-helper] $Msg"
    try {
        $dir = Split-Path $LogFile -Parent
        if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Force $dir | Out-Null }
        Add-Content -Path $LogFile -Value $line -Encoding UTF8
    } catch { }
    Write-Host $line
}

function Write-Err { param([string]$Msg) Write-Log $Msg "ERROR" }
function Write-Warn { param([string]$Msg) Write-Log $Msg "WARN" }

# ── Endpoint helpers ──────────────────────────────────────────────────────────
function Get-FriendlyName([string]$Base, [string]$Guid) {
    try {
        return (Get-ItemPropertyValue -Path "$Base\$Guid\Properties" -Name $FriendlyKey -ErrorAction Stop)
    } catch {
        return $Guid
    }
}

function Get-ActiveRenderEndpoints {
    $eps = @()
    if (-not (Test-Path $RenderBase)) { return $eps }
    Get-ChildItem $RenderBase -ErrorAction SilentlyContinue | ForEach-Object {
        $guid = $_.PSChildName
        # DeviceState: 1=active, 2=disabled, 4=not present, 8=unplugged
        try {
            $state = (Get-ItemPropertyValue -Path $_.PSPath -Name "DeviceState" -ErrorAction Stop)
            if ($state -ne 1) { return }
        } catch { return }
        $name = Get-FriendlyName $RenderBase $guid
        $eps += [PSCustomObject]@{ Guid = $guid; Name = $name }
    }
    return $eps
}

# ── Audio service restart ─────────────────────────────────────────────────────
function Restart-AudioService {
    Write-Log "Restarting Windows Audio (AudioSrv)..."
    try {
        Stop-Service  -Name AudioSrv           -Force -ErrorAction Stop
        Start-Sleep   -Milliseconds 800
        Start-Service -Name AudioEndpointBuilder       -ErrorAction Stop
        Start-Service -Name AudioSrv                   -ErrorAction Stop
        Write-Log "AudioSrv restarted."
    } catch {
        Write-Warn "Could not restart AudioSrv: $_"
    }
}

# =============================================================================
# Action: list
# Write an INI file with all active render endpoints (name + GUID, indexed).
# =============================================================================
if ($Action -eq "list") {
    Write-Log "Enumerating active render endpoints..."
    $eps = @(Get-ActiveRenderEndpoints)
    Write-Log "Found $($eps.Count) active render endpoint(s)."

    # Try to identify the current default endpoint via WinRT
    $defaultGuid = ""
    try {
        Add-Type -AssemblyName System.Runtime.WindowsRuntime -ErrorAction Stop
        $deviceClass = [Windows.Media.Devices.MediaDevice,Windows.Media.Devices,ContentType=WindowsRuntime]
        $defaultId = [Windows.Media.Devices.MediaDevice]::GetDefaultAudioRenderId(
            [Windows.Media.Devices.AudioDeviceRole]::Default)
        # ID format: \\?\SWD#MMDEVAPI#{0.0.0.00000000}.{GUID}#...
        if ($defaultId -match '\{([0-9a-fA-F\-]{36})\}#') {
            $defaultGuid = "{$($Matches[1])}"
        }
    } catch {
        Write-Log "WinRT default device detection unavailable; defaulting to first endpoint."
    }

    $defaultIdx = 0
    for ($i = 0; $i -lt $eps.Count; $i++) {
        if ($eps[$i].Guid -ieq $defaultGuid) { $defaultIdx = $i; break }
    }

    # Build INI content
    $ini  = "[Devices]`r`nCount=$($eps.Count)`r`nDefault=$defaultIdx`r`n"
    for ($i = 0; $i -lt $eps.Count; $i++) {
        $ini += "Name$i=$($eps[$i].Name)`r`n"
        $ini += "Guid$i=$($eps[$i].Guid)`r`n"
    }

    $outDir = Split-Path $OutFile -Parent
    if (-not (Test-Path $outDir)) { New-Item -ItemType Directory -Force $outDir | Out-Null }
    [System.IO.File]::WriteAllText($OutFile, $ini, [System.Text.Encoding]::UTF8)
    Write-Log "Device list written to $OutFile"
    exit 0
}

# =============================================================================
# Action: check-testsign
# Exit 0 if test-signing is ON, 1 if OFF.
# =============================================================================
if ($Action -eq "check-testsign") {
    try {
        $out = & bcdedit /enum 2>&1 | Out-String
        if ($out -match "testsigning\s+Yes") {
            Write-Log "Test-signing: ENABLED"
            exit 0
        } else {
            Write-Log "Test-signing: DISABLED"
            exit 1
        }
    } catch {
        Write-Warn "bcdedit check failed: $_"
        exit 1
    }
}

# =============================================================================
# Action: verify-com
# Exit 0 if render CLSID InprocServer32 exists and the DLL file is present.
# =============================================================================
if ($Action -eq "verify-com") {
    $clsidPath = "HKLM:\SOFTWARE\Classes\CLSID\$RenderClsid\InprocServer32"
    if (Test-Path $clsidPath) {
        try {
            $dll = (Get-ItemPropertyValue -Path $clsidPath -Name "(default)" -ErrorAction Stop)
            if ($dll -and (Test-Path $dll)) {
                Write-Log "COM verified: $dll"
                exit 0
            }
        } catch { }
    }
    Write-Log "COM NOT registered or DLL not found"
    exit 1
}

# =============================================================================
# Action: associate
# Write the render APO CLSID into the specified endpoint's FxProperties.
# =============================================================================
if ($Action -eq "associate") {
    if (-not $EndpointGuid) {
        Write-Err "associate: -EndpointGuid is required"
        exit 1
    }

    $epPath = "$RenderBase\$EndpointGuid"
    if (-not (Test-Path $epPath)) {
        Write-Err "Endpoint GUID not found: $EndpointGuid"
        exit 1
    }

    $name  = Get-FriendlyName $RenderBase $EndpointGuid
    $fxPath = "$epPath\FxProperties"
    Write-Log "Associating VeyraApoEfx with: $name ($EndpointGuid)"

    try {
        if (-not (Test-Path $fxPath)) {
            New-Item -Path $fxPath -Force | Out-Null
            Write-Log "Created FxProperties subkey."
        }
        Set-ItemProperty -Path $fxPath -Name $FxPostMixKey -Value $RenderClsid -Type String
        Write-Log "Written: $FxPostMixKey = $RenderClsid"
    } catch {
        Write-Err "Registry write failed: $_"
        exit 1
    }

    Restart-AudioService
    Write-Log "APO association complete for: $name"
    exit 0
}

# =============================================================================
# Action: unassociate-all
# Remove Veyra CLSIDs from every render + capture endpoint. Used by uninstaller.
# =============================================================================
if ($Action -eq "unassociate-all") {
    $allClsids = @($RenderClsid, $CaptureClsid)
    $removed   = 0

    foreach ($flow in @("Render", "Capture")) {
        $base = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\$flow"
        if (-not (Test-Path $base)) { continue }

        Get-ChildItem $base -ErrorAction SilentlyContinue | ForEach-Object {
            $fxPath = Join-Path $_.PSPath "FxProperties"
            if (-not (Test-Path $fxPath)) { return }

            foreach ($key in @($FxPostMixKey, $FxPreMixKey)) {
                $val = (Get-ItemProperty -Path $fxPath -Name $key -ErrorAction SilentlyContinue).$key
                if ($val -and ($allClsids -contains $val)) {
                    Remove-ItemProperty -Path $fxPath -Name $key -ErrorAction SilentlyContinue
                    Write-Log "Removed $key from $flow endpoint $($_.PSChildName)"
                    $removed++
                }
            }
        }
    }

    Write-Log "Removed APO from $removed endpoint(s)."
    if ($removed -gt 0) { Restart-AudioService }
    exit 0
}

Write-Err "Unknown action: $Action"
exit 1
