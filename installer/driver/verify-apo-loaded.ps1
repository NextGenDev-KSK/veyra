<#
.SYNOPSIS
  Reliably verify whether veyra-apo.dll is loaded and processing audio.

.DESCRIPTION
  tasklist /m is USELESS for this: audiodg.exe is a protected process (PPL),
  so its module list is never visible ("N/A" / 0 modules). This script instead
  uses a file-lock probe -- a DLL that audiodg has mapped as an image cannot be
  opened for exclusive read -- which works without elevation and never lies.

  Run AFTER a reboot (DisableProtectedAudioDG only takes effect when audiodg is
  recreated at boot). Play audio through the target device before/while running.
#>
param(
    [string]$Dll = "C:\Program Files\Veyra\veyra-apo.dll"
)

function Test-DllLoaded {
    param([string]$Path)
    try {
        $fs = [System.IO.File]::Open($Path, [System.IO.FileMode]::Open,
              [System.IO.FileAccess]::Read, [System.IO.FileShare]::None)
        $fs.Close()
        return $false   # got exclusive handle => nobody has it loaded
    } catch [System.IO.IOException] {
        return $true    # sharing violation => a process has it mapped as image
    } catch {
        return $null    # access denied / other => inconclusive
    }
}

Write-Host "=== 1. DisableProtectedAudioDG ==="
$flag = (Get-ItemProperty "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Audio" `
         -Name DisableProtectedAudioDG -ErrorAction SilentlyContinue).DisableProtectedAudioDG
if ($flag -eq 1) { "  OK: flag = 1" } else { "  MISSING: flag = $flag (set it and reboot)" }

Write-Host "`n=== 2. Is audiodg unprotected now? ==="
$adg = Get-Process audiodg -ErrorAction SilentlyContinue
if (-not $adg) {
    "  audiodg not running (no active stream). Start playing audio, then re-run."
} else {
    "  audiodg PID $($adg.Id), started $($adg.StartTime), modules visible: $($adg.Modules.Count)"
    if ($adg.Modules.Count -gt 0) {
        "  OK: audiodg is UNPROTECTED -- flag took effect."
        $veyra = $adg.Modules | Where-Object { $_.ModuleName -ieq "veyra-apo.dll" }
        if ($veyra) { "  >>> veyra-apo.dll IS LOADED in audiodg (path: $($veyra.FileName)) <<<" }
        else        { "  veyra-apo.dll NOT in audiodg module list yet." }
    } else {
        "  STILL PROTECTED: flag not in effect. A reboot is required."
    }
}

Write-Host "`n=== 3. Lock-probe (reliable, works even when audiodg is protected) ==="
$loaded = Test-DllLoaded -Path $Dll
switch ($loaded) {
    $true  { "  >>> LOCKED: a process has veyra-apo.dll mapped -> APO IS LOADED <<<" }
    $false { "  UNLOCKED: no process has the DLL loaded -> APO is NOT loaded." }
    default { "  INCONCLUSIVE (access denied). Run this script as your normal user, not elevated." }
}

Write-Host "`n=== 4. Default render endpoint FxProperties (PID 6 = ModeEffect) ==="
Add-Type -TypeDefinition @'
using System; using System.Runtime.InteropServices;
[ComImport, Guid("BCDE0395-E52F-467C-8E3D-C4579291692E")] class E {}
[Guid("A95664D2-9614-4F35-A746-DE8DB63617E6"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
interface IE { int f(int a,int b,out IntPtr c); int GetDefaultAudioEndpoint(int a,int b,out ID d); }
[Guid("D666063F-1587-4E43-81F1-B948E807363F"), InterfaceType(ComInterfaceType.InterfaceIsIUnknown)]
interface ID { int a(ref Guid i,int c,IntPtr p,out IntPtr f); int b(int a,out IntPtr p);
  int GetId([MarshalAs(UnmanagedType.LPWStr)] out string id); }
public class D2 { public static string Id() { var e=(IE)(object)new E(); ID d; if(e.GetDefaultAudioEndpoint(0,0,out d)!=0) return null; string s; d.GetId(out s); return s; } }
'@ -ErrorAction SilentlyContinue
$defId = [D2]::Id()
if ($defId -match '\{([0-9a-fA-F-]+)\}$') {
    $g = $Matches[1]
    "  Default render endpoint GUID: {$g}"
    $fx = "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render\{$g}\FxProperties"
    $v = (Get-ItemProperty $fx -Name "{D04E05A6-594B-4FB6-A80D-01AF5EED7D1D},6" -ErrorAction SilentlyContinue)."{D04E05A6-594B-4FB6-A80D-01AF5EED7D1D},6"
    if ($v) { "  PID6 ModeEffect = $v" } else { "  PID6 ModeEffect: NOT SET on the default device!" }
    if ($v -ne "{7E9C2B14-3F6A-4D8E-9B21-5C0A1F2E3D44}") {
        "  NOTE: default device is not associated with Veyra. Run VeyraSetupHelper --associate {$g}"
    }
}

Write-Host "`nDone. The lock-probe in step 3 is the authoritative answer."
