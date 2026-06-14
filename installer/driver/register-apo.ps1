<#
.SYNOPSIS
  Developer helper to (un)register the Veyra APO COM server for local testing.

.DESCRIPTION
  Registers veyra-apo.dll as an in-proc COM server via regsvr32 (calls the
  DLL's DllRegisterServer). This is only the COM registration step; associating
  the APO with an audio endpoint additionally requires either the driver INF or
  setting the endpoint's FX PropertyStore keys, and test-signing must be enabled
  for an unsigned/test-signed DLL to load into audiodg.exe. See BUILD_GUIDE.md.

  Run from an ELEVATED PowerShell prompt.

.PARAMETER Unregister
  Unregister instead of register.

.EXAMPLE
  .\register-apo.ps1 -DllPath ..\..\build\windows-release\bin\veyra-apo.dll
#>
param(
    [string]$DllPath = "veyra-apo.dll",
    [switch]$Unregister
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $DllPath)) {
    Write-Error "DLL not found: $DllPath"
    exit 1
}
$full = (Resolve-Path $DllPath).Path

if ($Unregister) {
    Write-Host "Unregistering $full"
    regsvr32.exe /s /u $full
} else {
    Write-Host "Registering $full"
    regsvr32.exe /s $full
}

if ($LASTEXITCODE -eq 0) {
    Write-Host "regsvr32 succeeded."
} else {
    Write-Error "regsvr32 failed with exit code $LASTEXITCODE (run elevated?)."
}
