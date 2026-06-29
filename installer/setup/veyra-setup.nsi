; =============================================================================
;  Veyra Sounds — Production Installer
;  Requires NSIS 3.x  (https://nsis.sourceforge.io)
;
;  Build:
;    pwsh installer/setup/build-installer.ps1 -BinDir build/windows-release/bin
;  Or directly:
;    makensis /DVERSION=1.0.0 installer/setup/veyra-setup.nsi
;
;  Install flow:
;    Welcome → License → Directory → Install Files → Device Setup → Finish
;
;  What this installer does automatically:
;    1.  Windows 10 build 19041+ check
;    2.  Upgrade detection — stops old service, preserves user data
;    3.  VC++ 2015-2022 x64 runtime check
;    4.  Extracts all binaries and resources to %ProgramFiles%\Veyra
;    5.  Registers the APO COM server (regsvr32 /s)
;    6.  Verifies COM registration
;    7.  Installs and starts the Windows service
;    8.  Creates %ProgramData%\Veyra\{logs,crashes,presets,themes} directories
;    9.  Shows device picker — enumerates playback devices (friendly names)
;   10.  Attempts APO association with selected device
;   11.  On failure (test-signing off / Bluetooth endpoint) — flags Bridge mode
;   12.  Configures preferred output in Veyra config
;   13.  Sets Start with Windows registry key
;   14.  Creates Start Menu shortcuts + optional Desktop shortcut
;   15.  Writes Programs & Features / Settings → Apps entry
;   16.  Runs post-install verification
;   17.  Writes install.log throughout
; =============================================================================

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"
!include "WinVer.nsh"
!include "FileFunc.nsh"
!include "nsDialogs.nsh"

; ── Version (overridden by /DVERSION=x.y.z on command line) ──────────────────
!ifndef VERSION
  !define VERSION "1.0.0"
!endif

; ── Product identity ──────────────────────────────────────────────────────────
!define PRODUCT_NAME      "Veyra Sounds"
!define PRODUCT_PUBLISHER "Veyra Sounds"
!define PRODUCT_URL       "https://github.com/NextGenDev-KSK/veyra"
!define SERVICE_NAME      "VeyraAudioService"
!define UNINST_KEY        "Software\Microsoft\Windows\CurrentVersion\Uninstall\VeyraSounds"
!define STARTUP_KEY       "Software\Microsoft\Windows\CurrentVersion\Run"
!define STARTUP_VALUE     "VeyraSounds"
!define RENDER_CLSID      "{7E9C2B14-3F6A-4D8E-9B21-5C0A1F2E3D44}"
!define PS_EXE            "$WINDIR\System32\WindowsPowerShell\v1.0\powershell.exe"
!define PS_FLAGS          "-NonInteractive -NoProfile -ExecutionPolicy Bypass"
!define APO_HELPER        "$INSTDIR\driver\apo-helper.ps1"
!define LOG_FILE          "$PROGRAMDATA\Veyra\logs\install.log"

; ── Output ────────────────────────────────────────────────────────────────────
Name "${PRODUCT_NAME} ${VERSION}"
OutFile "veyra-sounds-setup-${VERSION}-x64.exe"
InstallDir "$PROGRAMFILES64\Veyra"
InstallDirRegKey HKLM "${UNINST_KEY}" "InstallLocation"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
Unicode true
BrandingText "${PRODUCT_NAME} ${VERSION}"

; ── Variables ─────────────────────────────────────────────────────────────────
Var IsUpgrade
Var OldVersion
Var DeviceCount
Var SelectedIndex
Var SelectedGuid
Var SelectedName
Var TestSignEnabled
Var BridgeFallback
Var ApoAssociated
Var hDevicePage
Var hDeviceList
Var hDeviceLabel
Var hDeviceNote

; Windows combobox messages (not defined in NSIS headers)
!define CB_ADDSTRING  0x0143
!define CB_GETCURSEL  0x0147
!define CB_SETCURSEL  0x014E

; ── Version info ──────────────────────────────────────────────────────────────
VIProductVersion "${VERSION}.0"
VIAddVersionKey /LANG=1033 "ProductName"     "${PRODUCT_NAME}"
VIAddVersionKey /LANG=1033 "ProductVersion"  "${VERSION}"
VIAddVersionKey /LANG=1033 "CompanyName"     "${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=1033 "LegalCopyright"  "GPLv3 — see LICENSE"
VIAddVersionKey /LANG=1033 "FileDescription" "${PRODUCT_NAME} Setup"
VIAddVersionKey /LANG=1033 "FileVersion"     "${VERSION}.0"

; ── MUI2 appearance ───────────────────────────────────────────────────────────
!define MUI_ABORTWARNING
!define MUI_ABORTWARNING_TEXT "Are you sure you want to cancel the ${PRODUCT_NAME} installation?"

!define MUI_WELCOMEPAGE_TITLE   "Welcome to ${PRODUCT_NAME} ${VERSION}"
!define MUI_WELCOMEPAGE_TEXT    "${PRODUCT_NAME} is a free, open-source system-wide audio enhancer for Windows 10/11.$\r$\n$\r$\nIt installs in about 30 seconds and works immediately with your current output device — no virtual cable, no manual configuration.$\r$\n$\r$\nClick Next to begin."

!define MUI_LICENSEPAGE_CHECKBOX
!define MUI_LICENSEPAGE_CHECKBOX_TEXT "I accept the terms of the GPLv3 license"

!define MUI_DIRECTORYPAGE_TEXT_TOP "Setup will install ${PRODUCT_NAME} in the following folder.$\r$\n$\r$\nTo install in a different folder, click Browse and select another folder. Click Next to continue."

!define MUI_FINISHPAGE_TITLE    "${PRODUCT_NAME} is installed"
!define MUI_FINISHPAGE_RUN      "$INSTDIR\veyra.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${PRODUCT_NAME} now"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\INSTALLATION.txt"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Open Quick Start guide"

; ── Pages ─────────────────────────────────────────────────────────────────────
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE   "staging\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
Page custom DevicePageCreate DevicePageLeave
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; =============================================================================
;  Logging helper macro
; =============================================================================
!macro _WriteLog msg
    Push $9
    FileOpen $9 "${LOG_FILE}" a
    ${If} $9 != ""
        FileSeek $9 0 END
        FileWrite $9 "${msg}$\r$\n"
        FileClose $9
    ${EndIf}
    Pop $9
!macroend
!define WriteLog "!insertmacro _WriteLog"

; =============================================================================
;  .onInit — upgrade detection, 64-bit registry, initial state
; =============================================================================
Function .onInit
    ; Require 64-bit Windows
    ${If} ${RunningX64}
        ${EnableX64FSRedirection}
        SetRegView 64
    ${Else}
        MessageBox MB_ICONSTOP "${PRODUCT_NAME} requires a 64-bit version of Windows."
        Abort
    ${EndIf}

    ; Check for existing installation
    ReadRegStr $OldVersion HKLM "${UNINST_KEY}" "DisplayVersion"
    ${If} $OldVersion != ""
        StrCpy $IsUpgrade "1"
    ${Else}
        StrCpy $IsUpgrade "0"
    ${EndIf}

    ; Initialise state variables
    StrCpy $SelectedGuid      ""
    StrCpy $SelectedName      ""
    StrCpy $TestSignEnabled   "0"
    StrCpy $BridgeFallback    "0"
    StrCpy $ApoAssociated     "0"
    StrCpy $DeviceCount       "0"
    StrCpy $SelectedIndex     "0"
FunctionEnd

; =============================================================================
;  Device Setup page — shown AFTER InstFiles
;  Enumerates playback endpoints, lets user pick one, attempts APO association.
; =============================================================================
Function DevicePageCreate
    ; Skip in silent mode
    IfSilent 0 +2
    Abort

    ; Skip device page for upgrades (keep existing APO association)
    ${If} $IsUpgrade == "1"
        Abort
    ${EndIf}

    ; Check test-signing
    nsExec::ExecToLog '"${PS_EXE}" ${PS_FLAGS} -File "${APO_HELPER}" -Action check-testsign -LogFile "${LOG_FILE}"'
    Pop $TestSignEnabled   ; 0 = enabled, 1 = disabled (or error)

    ; Enumerate devices
    nsExec::ExecToLog '"${PS_EXE}" ${PS_FLAGS} -File "${APO_HELPER}" -Action list -OutFile "$TEMP\veyra-devices.ini" -LogFile "${LOG_FILE}"'
    Pop $0

    ReadINIStr $DeviceCount "$TEMP\veyra-devices.ini" "Devices" "Count"
    ${If} $DeviceCount == ""
        StrCpy $DeviceCount "0"
    ${EndIf}

    ; Build the custom page
    !insertmacro MUI_HEADER_TEXT "Audio Device Setup" "Choose where ${PRODUCT_NAME} should process your audio."

    nsDialogs::Create 1018
    Pop $hDevicePage
    ${If} $hDevicePage == error
        Abort
    ${EndIf}

    ; ── Section: APO status note ─────────────────────────────────────────────
    ${If} $TestSignEnabled != "0"
        ; Test-signing off — explain Bridge fallback
        ${NSD_CreateLabel} 0 0u 100% 50u "Audio Enhancement: Compatibility Mode$\r$\n$\r$\nTest-signing is not enabled on this system, so the low-latency APO driver cannot be activated automatically.$\r$\n$\r$\nVeyra will use Audio Bridge compatibility mode instead. You can enable the APO later via the Devices screen."
        Pop $hDeviceNote
        StrCpy $BridgeFallback "1"
    ${Else}
        ${If} $DeviceCount == "0"
            ; No devices found
            ${NSD_CreateLabel} 0 0u 100% 40u "No active playback devices were detected.$\r$\n$\r$\nVeyra will use Audio Bridge compatibility mode. You can configure your output device in the Devices screen after launch."
            Pop $hDeviceNote
            StrCpy $BridgeFallback "1"
        ${Else}
            ; Normal APO path — show device picker
            ${NSD_CreateLabel} 0 0u 100% 28u "Veyra will process all audio on the selected device.$\r$\nYou can change this at any time in the Devices screen."
            Pop $hDeviceLabel

            ${NSD_CreateLabel} 0 32u 70u 14u "Playback device:"
            Pop $0

            ${NSD_CreateDropList} 72u 30u 228u 120u ""
            Pop $hDeviceList

            ; Populate dropdown from INI file
            StrCpy $0 0
            devloop:
                IntCmp $0 $DeviceCount devdone 0 devdone
                ReadINIStr $R1 "$TEMP\veyra-devices.ini" "Devices" "Name$0"
                ${If} $R1 != ""
                    SendMessage $hDeviceList ${CB_ADDSTRING} 0 "STR:$R1"
                ${EndIf}
                IntOp $0 $0 + 1
                Goto devloop
            devdone:

            ; Add skip option
            SendMessage $hDeviceList ${CB_ADDSTRING} 0 "STR:Configure later in Devices screen"

            ; Pre-select the default device
            ReadINIStr $R0 "$TEMP\veyra-devices.ini" "Devices" "Default"
            ${If} $R0 == ""
                StrCpy $R0 0
            ${EndIf}
            SendMessage $hDeviceList ${CB_SETCURSEL} $R0 0

            ${NSD_CreateLabel} 0 52u 100% 28u "Veyra will activate the audio enhancement on this device. A restart of Windows Audio occurs automatically — you may hear a brief audio interruption."
            Pop $0
        ${EndIf}
    ${EndIf}

    nsDialogs::Show
FunctionEnd

Function DevicePageLeave
    ; Nothing to do when Bridge fallback is already decided
    ${If} $BridgeFallback == "1"
        ${WriteLog} "Device page: Bridge fallback mode (test-signing off or no devices)."
        Return
    ${EndIf}

    ; Get selected index from dropdown
    SendMessage $hDeviceList ${CB_GETCURSEL} 0 0 $SelectedIndex

    ; "Configure later" is the last item (index == DeviceCount)
    ${If} $SelectedIndex >= $DeviceCount
        StrCpy $SelectedGuid ""
        StrCpy $SelectedName "Not configured"
        ${WriteLog} "Device page: user chose 'Configure later'."
        Return
    ${EndIf}

    ; Look up GUID and name for the selected index
    ReadINIStr $SelectedGuid "$TEMP\veyra-devices.ini" "Devices" "Guid$SelectedIndex"
    ReadINIStr $SelectedName "$TEMP\veyra-devices.ini" "Devices" "Name$SelectedIndex"
    ${WriteLog} "Device page: selected '$SelectedName' ($SelectedGuid)."

    ; Attempt APO association
    DetailPrint "Activating audio enhancement on $SelectedName..."
    nsExec::ExecToLog '"${PS_EXE}" ${PS_FLAGS} -File "${APO_HELPER}" -Action associate -EndpointGuid "$SelectedGuid" -LogFile "${LOG_FILE}"'
    Pop $R0

    ${If} $R0 == "0"
        StrCpy $ApoAssociated "1"
        ${WriteLog} "APO association succeeded for '$SelectedName'."
    ${Else}
        StrCpy $BridgeFallback "1"
        ${WriteLog} "APO association failed (exit $R0) for '$SelectedName'. Bridge fallback."
        MessageBox MB_ICONINFORMATION \
            "The audio enhancement could not be activated on $SelectedName.$\r$\n$\r$\nThis device may not support the APO (common with Bluetooth endpoints). ${PRODUCT_NAME} will use Audio Bridge compatibility mode.$\r$\n$\r$\nYou can configure this later in the Devices screen." \
            /SD IDOK
    ${EndIf}
FunctionEnd

; =============================================================================
;  Main install section
; =============================================================================
Section "${PRODUCT_NAME}" SecCore

    SectionIn RO   ; required — cannot be deselected

    ; ── OS check ──────────────────────────────────────────────────────────────
    ${IfNot} ${AtLeastBuild} 19041
        MessageBox MB_ICONSTOP \
            "${PRODUCT_NAME} requires Windows 10 version 2004 (build 19041) or later.$\r$\nPlease update Windows and run the installer again."
        Abort
    ${EndIf}

    ; ── Create ProgramData directories (need them for logging immediately) ────
    CreateDirectory "$PROGRAMDATA\Veyra"
    CreateDirectory "$PROGRAMDATA\Veyra\logs"
    CreateDirectory "$PROGRAMDATA\Veyra\crashes"
    CreateDirectory "$PROGRAMDATA\Veyra\presets"
    CreateDirectory "$PROGRAMDATA\Veyra\themes"

    ; Set ACL: LocalService needs full access to write config, logs, crash dumps
    nsExec::ExecToLog '"$SYSDIR\icacls.exe" "$PROGRAMDATA\Veyra" /grant "NT AUTHORITY\LocalService:(OI)(CI)M" /T /Q'

    ; ── Start logging ─────────────────────────────────────────────────────────
    ${WriteLog} "============================================================"
    ${WriteLog} "${PRODUCT_NAME} ${VERSION} — Installation started"
    ${WriteLog} "============================================================"
    ${If} $IsUpgrade == "1"
        ${WriteLog} "Mode: Upgrade from $OldVersion"
        DetailPrint "Upgrading ${PRODUCT_NAME} from $OldVersion to ${VERSION}..."
    ${Else}
        ${WriteLog} "Mode: Fresh install"
        DetailPrint "Installing ${PRODUCT_NAME} ${VERSION}..."
    ${EndIf}

    ; ── Upgrade: stop running processes ───────────────────────────────────────
    ${If} $IsUpgrade == "1"
        DetailPrint "Stopping existing installation..."
        ${WriteLog} "Upgrade: stopping existing processes and service..."
        nsExec::ExecToLog '"$SYSDIR\taskkill.exe" /F /IM veyra.exe /IM veyra-overlay.exe'
        Pop $0
        ; Stop service gracefully (allow 3 s)
        nsExec::ExecToLog '"$SYSDIR\sc.exe" stop ${SERVICE_NAME}'
        Pop $0
        Sleep 3000
        ${WriteLog} "Upgrade: old processes stopped."
    ${EndIf}

    ; ── VC++ 2015-2022 x64 runtime check ─────────────────────────────────────
    DetailPrint "Checking Visual C++ runtime..."
    ${WriteLog} "Checking VC++ 2015-2022 x64 runtime..."
    ReadRegDWORD $R0 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Installed"
    ${If} $R0 != "1"
        ${WriteLog} "VC++ runtime not found — installing bundled redistributable..."
        DetailPrint "Installing Visual C++ runtime..."
        ; Bundle vc_redist.x64.exe in staging/ if present
        IfFileExists "staging\vc_redist.x64.exe" vcredist_found vcredist_notfound
        vcredist_found:
            File "staging\vc_redist.x64.exe"
            ExecWait '"$INSTDIR\vc_redist.x64.exe" /install /quiet /norestart' $R1
            Delete "$INSTDIR\vc_redist.x64.exe"
            ${WriteLog} "VC++ redist installer exited: $R1"
            Goto vcredist_done
        vcredist_notfound:
            ${WriteLog} "VC++ redist not bundled — proceeding anyway (may fail at runtime)."
            MessageBox MB_ICONEXCLAMATION \
                "The Visual C++ 2015-2022 runtime was not found on this system.$\r$\n$\r$\nIf ${PRODUCT_NAME} fails to launch, install the Visual C++ Redistributable from microsoft.com." \
                /SD IDOK
        vcredist_done:
    ${Else}
        ${WriteLog} "VC++ runtime present."
    ${EndIf}

    ; ── Extract files ─────────────────────────────────────────────────────────
    DetailPrint "Installing files..."
    ${WriteLog} "Extracting files to $INSTDIR..."
    SetOutPath "$INSTDIR"

    File "staging\veyra.exe"
    File "staging\veyra-service.exe"
    File "staging\veyra-apo.dll"
    File "staging\veyra-overlay.exe"
    File "staging\LICENSE"
    File "staging\INSTALLATION.txt"
    File "staging\setup-audio-driver.cmd"

    SetOutPath "$INSTDIR\hrtf\diffuse"
    File /nonfatal /r "staging\hrtf\diffuse\*.*"

    SetOutPath "$INSTDIR\resources\lang"
    File /nonfatal /r "staging\resources\lang\*.json"

    SetOutPath "$INSTDIR\resources\themes"
    File /nonfatal /r "staging\resources\themes\*.json"

    SetOutPath "$INSTDIR\resources\autoeq"
    File /nonfatal /r "staging\resources\autoeq\*.*"

    SetOutPath "$INSTDIR\driver"
    File "staging\driver\register-apo.ps1"
    File "staging\driver\associate-apo.ps1"
    File "staging\driver\uninstall-apo.ps1"
    File "staging\driver\apo-helper.ps1"

    SetOutPath "$INSTDIR"
    ${WriteLog} "Files extracted."

    ; ── Register APO COM server ────────────────────────────────────────────────
    DetailPrint "Registering audio processing component..."
    ${WriteLog} "Running regsvr32 /s '$INSTDIR\veyra-apo.dll'..."
    ExecWait '"$SYSDIR\regsvr32.exe" /s "$INSTDIR\veyra-apo.dll"' $R0
    ${WriteLog} "regsvr32 exited: $R0"
    ${If} $R0 != "0"
        ${WriteLog} "WARN: COM registration failed. APO path will not work. Bridge mode will be used."
        DetailPrint "Audio component registration failed — Bridge mode will be used."
    ${Else}
        ; Verify the registration
        nsExec::ExecToLog '"${PS_EXE}" ${PS_FLAGS} -File "${APO_HELPER}" -Action verify-com -LogFile "${LOG_FILE}"'
        Pop $R1
        ${If} $R1 == "0"
            ${WriteLog} "COM registration verified."
            DetailPrint "Audio component registered successfully."
        ${Else}
            ${WriteLog} "WARN: COM verify failed after regsvr32. APO may not load."
        ${EndIf}
    ${EndIf}

    ; ── Install Windows service ────────────────────────────────────────────────
    DetailPrint "Installing Veyra Audio Service..."
    ${WriteLog} "Installing service: '$INSTDIR\veyra-service.exe' --install..."
    ExecWait '"$INSTDIR\veyra-service.exe" --install' $R0
    ${WriteLog} "Service install exited: $R0"
    ${If} $R0 != "0"
        ${WriteLog} "WARN: Service install returned $R0."
        DetailPrint "Service installation returned $R0 — retrying via sc.exe..."
        ; The service may already exist (upgrade path) — try anyway
        nsExec::ExecToLog '"$SYSDIR\sc.exe" config ${SERVICE_NAME} start= auto'
        Pop $0
    ${EndIf}

    ; ── Start the service ──────────────────────────────────────────────────────
    DetailPrint "Starting Veyra Audio Service..."
    ${WriteLog} "Starting service..."
    ExecWait '"$SYSDIR\sc.exe" start ${SERVICE_NAME}' $R0
    ${WriteLog} "sc start exited: $R0"
    Sleep 1500

    ; ── Set startup with Windows ──────────────────────────────────────────────
    WriteRegStr HKCU "${STARTUP_KEY}" "${STARTUP_VALUE}" \
        '"$INSTDIR\veyra.exe" --minimized'
    ${WriteLog} "Startup key written."

    ; ── Shortcuts ─────────────────────────────────────────────────────────────
    DetailPrint "Creating shortcuts..."
    ${WriteLog} "Creating shortcuts..."
    CreateDirectory "$SMPROGRAMS\Veyra Sounds"
    CreateShortcut "$SMPROGRAMS\Veyra Sounds\Veyra Sounds.lnk" \
                   "$INSTDIR\veyra.exe" "" "$INSTDIR\veyra.exe" 0
    CreateShortcut "$SMPROGRAMS\Veyra Sounds\Veyra Overlay.lnk" \
                   "$INSTDIR\veyra-overlay.exe" "" "$INSTDIR\veyra-overlay.exe" 0
    CreateShortcut "$SMPROGRAMS\Veyra Sounds\Setup Audio Driver (Advanced).lnk" \
                   "$INSTDIR\setup-audio-driver.cmd" "" "$SYSDIR\cmd.exe" 0
    CreateShortcut "$SMPROGRAMS\Veyra Sounds\Uninstall Veyra Sounds.lnk" \
                   "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0

    ; ── Uninstaller ───────────────────────────────────────────────────────────
    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; ── Programs & Features / Settings → Apps ─────────────────────────────────
    ${GetSize} "$INSTDIR" "/S=0K" $R0 $R1 $R2
    IntFmt $R0 "0x%08X" $R0

    WriteRegStr   HKLM "${UNINST_KEY}" "DisplayName"          "${PRODUCT_NAME}"
    WriteRegStr   HKLM "${UNINST_KEY}" "DisplayVersion"       "${VERSION}"
    WriteRegStr   HKLM "${UNINST_KEY}" "Publisher"            "${PRODUCT_PUBLISHER}"
    WriteRegStr   HKLM "${UNINST_KEY}" "URLInfoAbout"         "${PRODUCT_URL}"
    WriteRegStr   HKLM "${UNINST_KEY}" "InstallLocation"      "$INSTDIR"
    WriteRegStr   HKLM "${UNINST_KEY}" "UninstallString"      "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr   HKLM "${UNINST_KEY}" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
    WriteRegStr   HKLM "${UNINST_KEY}" "DisplayIcon"          "$INSTDIR\veyra.exe"
    WriteRegStr   HKLM "${UNINST_KEY}" "HelpLink"             "${PRODUCT_URL}"
    WriteRegDWORD HKLM "${UNINST_KEY}" "EstimatedSize"        "$R0"
    WriteRegDWORD HKLM "${UNINST_KEY}" "NoModify"             1
    WriteRegDWORD HKLM "${UNINST_KEY}" "NoRepair"             1
    ${WriteLog} "Programs & Features entry written."

    ; ── Post-install verification ──────────────────────────────────────────────
    DetailPrint "Verifying installation..."
    ${WriteLog} "--- Post-install verification ---"

    ; 1. Service running?
    nsExec::ExecToStack '"$SYSDIR\sc.exe" query ${SERVICE_NAME}'
    Pop $0  ; exit code (0 = service exists)
    Pop $R1 ; output
    ${If} $0 == "0"
        ${WriteLog} "VERIFY OK: Service installed."
    ${Else}
        ${WriteLog} "VERIFY WARN: Service query failed (exit $0)."
    ${EndIf}

    ; 2. COM registered?
    ReadRegStr $R1 HKLM "SOFTWARE\Classes\CLSID\${RENDER_CLSID}\InprocServer32" ""
    ${If} $R1 != ""
        ${WriteLog} "VERIFY OK: COM CLSID registered: $R1"
    ${Else}
        ${WriteLog} "VERIFY WARN: COM CLSID not found in HKLM\SOFTWARE\Classes."
    ${EndIf}

    ; 3. ProgramData directories?
    ${If} ${FileExists} "$PROGRAMDATA\Veyra\logs"
        ${WriteLog} "VERIFY OK: ProgramData\Veyra\logs exists."
    ${Else}
        ${WriteLog} "VERIFY WARN: ProgramData\Veyra\logs missing."
    ${EndIf}

    ; 4. Startup key?
    ReadRegStr $R1 HKCU "${STARTUP_KEY}" "${STARTUP_VALUE}"
    ${If} $R1 != ""
        ${WriteLog} "VERIFY OK: Startup key set."
    ${Else}
        ${WriteLog} "VERIFY WARN: Startup key not found."
    ${EndIf}

    ${WriteLog} "--- Verification complete ---"
    ${WriteLog} "Installation finished. APO associated: $ApoAssociated  Bridge fallback: $BridgeFallback"

SectionEnd

; =============================================================================
;  Optional section: Desktop shortcut
; =============================================================================
Section /o "Create Desktop shortcut" SecDesktop
    CreateShortcut "$DESKTOP\Veyra Sounds.lnk" \
                   "$INSTDIR\veyra.exe" "" "$INSTDIR\veyra.exe" 0
    ${WriteLog} "Desktop shortcut created."
SectionEnd

; Pre-check the optional desktop shortcut by default
Function .onSelChange
    SectionSetFlags ${SecDesktop} 1
FunctionEnd

; =============================================================================
;  Uninstall section
; =============================================================================
Section "Uninstall"

    ; Ask about preserving user data
    MessageBox MB_ICONQUESTION|MB_YESNO \
        "Do you want to keep your presets, themes, and settings?$\r$\n$\r$\nClick Yes to preserve them in %ProgramData%\Veyra.$\r$\nClick No to delete all ${PRODUCT_NAME} data." \
        /SD IDYES IDYES preserve_data IDNO delete_data

    preserve_data:
        StrCpy $R9 "preserve"
        Goto after_data_question

    delete_data:
        StrCpy $R9 "delete"

    after_data_question:

    ; ── Remove APO associations from all endpoints ────────────────────────────
    DetailPrint "Removing audio driver configuration..."
    nsExec::ExecToLog '"${PS_EXE}" ${PS_FLAGS} -File "$INSTDIR\driver\apo-helper.ps1" -Action unassociate-all -LogFile "${LOG_FILE}"'
    Pop $0

    ; ── Belt-and-suspenders COM unregistration ────────────────────────────────
    ExecWait '"$SYSDIR\regsvr32.exe" /s /u "$INSTDIR\veyra-apo.dll"' $0

    ; ── Stop running processes ────────────────────────────────────────────────
    DetailPrint "Stopping Veyra..."
    ExecWait '"$SYSDIR\taskkill.exe" /F /IM veyra.exe'         $0
    ExecWait '"$SYSDIR\taskkill.exe" /F /IM veyra-overlay.exe' $0
    Sleep 1000

    ; ── Stop and remove Windows service ──────────────────────────────────────
    DetailPrint "Removing Veyra Audio Service..."
    ExecWait '"$INSTDIR\veyra-service.exe" --uninstall' $0
    Sleep 1000

    ; ── Delete install directory ──────────────────────────────────────────────
    DetailPrint "Removing files..."
    RMDir /r "$INSTDIR"

    ; ── Remove user data (only if chosen) ────────────────────────────────────
    ${If} $R9 == "delete"
        RMDir /r "$PROGRAMDATA\Veyra"
        DetailPrint "User data removed."
    ${EndIf}

    ; ── Remove shortcuts ──────────────────────────────────────────────────────
    Delete "$SMPROGRAMS\Veyra Sounds\Veyra Sounds.lnk"
    Delete "$SMPROGRAMS\Veyra Sounds\Veyra Overlay.lnk"
    Delete "$SMPROGRAMS\Veyra Sounds\Setup Audio Driver (Advanced).lnk"
    Delete "$SMPROGRAMS\Veyra Sounds\Uninstall Veyra Sounds.lnk"
    RMDir  "$SMPROGRAMS\Veyra Sounds"
    Delete "$DESKTOP\Veyra Sounds.lnk"

    ; ── Remove startup registry entry ─────────────────────────────────────────
    DeleteRegValue HKCU "${STARTUP_KEY}" "${STARTUP_VALUE}"

    ; ── Remove Programs & Features entry ─────────────────────────────────────
    DeleteRegKey HKLM "${UNINST_KEY}"

    DetailPrint "${PRODUCT_NAME} uninstalled."

SectionEnd
