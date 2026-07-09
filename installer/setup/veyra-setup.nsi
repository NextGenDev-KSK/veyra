; =============================================================================
;  Veyra Sounds — Production Installer
;  NSIS 3.x  |  Unicode  |  No PowerShell  |  No scripts shown to users
;
;  Install flow:
;    Welcome → License → Directory → Install Files → Device Setup → Finish
;
;  What this installer does automatically (no user intervention):
;    1.  Windows 10 build 19041+ check
;    2.  Upgrade detection (stop old service, preserve user data)
;    3.  VC++ 2015-2022 x64 runtime check / install
;    4.  Extracts all binaries + resources to %ProgramFiles%\Veyra\
;    5.  Creates %ProgramData%\Veyra\{logs,crashes,presets,themes} + ACLs
;    6.  Registers veyra-apo.dll COM server (regsvr32 /s — no window)
;    7.  Installs VeyraAudioService (veyra-service.exe --install)
;    8.  Starts service (sc.exe start)
;    9.  Lists active playback devices via VeyraSetupHelper.exe (C++ native)
;   10.  User picks output device (friendly names — no GUIDs exposed)
;   11.  Associates APO via VeyraSetupHelper.exe --associate <guid>
;   12.  On association failure: informs the user (Audio Bridge is the manual
;        fallback — docs/AUDIO_BRIDGE.md; nothing is switched automatically)
;   13.  Post-install verification (service, COM, directories)
;   14.  Start Menu + optional Desktop shortcuts
;   15.  Programs & Features / Settings → Apps entry
;   16.  Logs every step to %ProgramData%\Veyra\logs\install.log
;
;  Uninstaller:
;    - Asks "Preserve presets and settings?" (default: Yes)
;    - VeyraSetupHelper.exe --unassociate-all (C++ native, no PS)
;    - regsvr32 /s /u  (COM unregistration, no window)
;    - veyra-service.exe --uninstall
;    - Removes all files, shortcuts, registry entries
;
;  Build:
;    pwsh installer/setup/build-installer.ps1 -BinDir build/windows-release/bin
; =============================================================================

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"
!include "WinVer.nsh"
!include "FileFunc.nsh"
!include "nsDialogs.nsh"

; ── Version (passed by build-installer.ps1 as /DVERSION=x.y.z) ───────────────
!ifndef VERSION
  !define VERSION "1.1.0"
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
!define FX_POSTMIX_KEY    "{D04E05A6-594B-4FB6-A80D-01AF5EED7D1D},6"

; Native helper — handles audio API calls NSIS cannot make directly
!define HELPER            "$INSTDIR\VeyraSetupHelper.exe"
!define LOG_FILE          "$DataDir\Veyra\logs\install.log"

; ── Output ────────────────────────────────────────────────────────────────────
Name "${PRODUCT_NAME} ${VERSION}"
OutFile "veyra-sounds-setup-${VERSION}-x64.exe"
InstallDir "$PROGRAMFILES64\Veyra"
InstallDirRegKey HKLM "${UNINST_KEY}" "InstallLocation"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
Unicode true
BrandingText "${PRODUCT_NAME} ${VERSION}"

; ── Runtime variables ─────────────────────────────────────────────────────────
Var IsUpgrade
Var OldVersion
Var DeviceCount
Var SelectedIndex
Var SelectedGuid
Var SelectedName
Var ApoAssociated
Var hDevicePage
Var hDeviceList
Var DataDir     ; resolved ProgramData path — $DataDir is not a built-in on all NSIS builds

; ComboBox messages — defined in WinMessages.nsh (included by nsDialogs.nsh in
; NSIS 3.06+). Guard with !ifndef so this builds on older NSIS versions too.
!ifndef CB_ADDSTRING
  !define CB_ADDSTRING  0x0143
!endif
!ifndef CB_GETCURSEL
  !define CB_GETCURSEL  0x0147
!endif
!ifndef CB_SETCURSEL
  !define CB_SETCURSEL  0x014E
!endif

; ── Version info block ────────────────────────────────────────────────────────
VIProductVersion "${VERSION}.0"
VIAddVersionKey /LANG=1033 "ProductName"     "${PRODUCT_NAME}"
VIAddVersionKey /LANG=1033 "ProductVersion"  "${VERSION}"
VIAddVersionKey /LANG=1033 "CompanyName"     "${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=1033 "LegalCopyright"  "GPLv3 — see LICENSE"
VIAddVersionKey /LANG=1033 "FileDescription" "${PRODUCT_NAME} Setup"
VIAddVersionKey /LANG=1033 "FileVersion"     "${VERSION}.0"

; ── MUI2 configuration ────────────────────────────────────────────────────────
; MUI2 owns .onGUIInit internally. Use its hook to run our init code after
; sections are created (so ${SecDesktop} is valid).
!define MUI_CUSTOMFUNCTION_GUIINIT VeyraGUIInit

!define MUI_ABORTWARNING
!define MUI_ABORTWARNING_TEXT "Cancel the ${PRODUCT_NAME} installation?"

!define MUI_WELCOMEPAGE_TITLE "${PRODUCT_NAME} ${VERSION}"
!define MUI_WELCOMEPAGE_TEXT \
    "${PRODUCT_NAME} is a free, open-source audio enhancer for Windows 10/11.$\r$\n$\r$\n\
You will choose your playback device on the next page. See the Quick Start \
guide after install for how to hear the effects on this release.$\r$\n$\r$\n\
Click Next to begin."

!define MUI_LICENSEPAGE_CHECKBOX
!define MUI_LICENSEPAGE_CHECKBOX_TEXT "I accept the terms of the GPLv3 license"

!define MUI_FINISHPAGE_TITLE "${PRODUCT_NAME} is ready"
!define MUI_FINISHPAGE_RUN "$INSTDIR\veyra.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${PRODUCT_NAME}"
!define MUI_FINISHPAGE_SHOWREADME "$INSTDIR\INSTALLATION.txt"
!define MUI_FINISHPAGE_SHOWREADME_TEXT "Open Quick Start guide"

; ── Pages ─────────────────────────────────────────────────────────────────────
!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "staging\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
Page custom DevicePageCreate DevicePageLeave
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; =============================================================================
;  Log helper — appends a timestamped line to install.log
;  (VeyraSetupHelper.exe also writes there via --log; this macro adds the NSIS
;   side of events so the log is complete.)
; =============================================================================
!macro _AppendLog msg
    Push $9
    FileOpen $9 "${LOG_FILE}" a
    ${If} $9 != ""
        FileSeek $9 0 END
        FileWrite $9 "[NSIS] ${msg}$\r$\n"
        FileClose $9
    ${EndIf}
    Pop $9
!macroend
!define AppendLog "!insertmacro _AppendLog"

; =============================================================================
;  .onInit — 64-bit validation, upgrade detection, state initialisation
; =============================================================================
Function .onInit
    ${If} ${RunningX64}
        ; Disable WOW64 file-system redirection so $SYSDIR evaluates to the real
        ; 64-bit C:\Windows\System32 instead of C:\Windows\SysWow64.
        ; Without this, regsvr32.exe from SysWow64 (32-bit) is called to register
        ; a 64-bit DLL, which silently fails with ERROR_BAD_EXE_FORMAT.
        ${DisableX64FSRedirection}
        SetRegView 64
    ${Else}
        MessageBox MB_ICONSTOP \
            "${PRODUCT_NAME} requires a 64-bit version of Windows 10 or later."
        Abort
    ${EndIf}

    ; Detect existing installation
    ReadRegStr $OldVersion HKLM "${UNINST_KEY}" "DisplayVersion"
    ${If} $OldVersion != ""
        StrCpy $IsUpgrade "1"
    ${Else}
        StrCpy $IsUpgrade "0"
    ${EndIf}

    ; Resolve ProgramData path. $DataDir is not a recognised built-in on all
    ; NSIS distributions, so read it from the environment / registry instead.
    ReadEnvStr $DataDir "PROGRAMDATA"
    ${If} $DataDir == ""
        ReadRegStr $DataDir HKLM \
            "SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" \
            "Common AppData"
    ${EndIf}
    ${If} $DataDir == ""
        StrCpy $DataDir "C:\ProgramData"
    ${EndIf}

    ; Initialise state
    StrCpy $SelectedGuid  ""
    StrCpy $SelectedName  ""
    StrCpy $ApoAssociated "0"
    StrCpy $DeviceCount   "0"
    StrCpy $SelectedIndex "0"
FunctionEnd

; Remove the device-enumeration scratch file once the install finishes.
Function .onInstSuccess
    Delete "$TEMP\veyra-devices.ini"
FunctionEnd

Function .onInstFailed
    Delete "$TEMP\veyra-devices.ini"
FunctionEnd

; =============================================================================
;  Main install section
; =============================================================================
Section "${PRODUCT_NAME}" SecMain
    SectionIn RO

    ; ── OS requirement ────────────────────────────────────────────────────────
    ${IfNot} ${AtLeastBuild} 19041
        MessageBox MB_ICONSTOP \
            "${PRODUCT_NAME} requires Windows 10 version 2004 (build 19041) or later.$\r$\n\
Please update Windows and run Setup again."
        Abort
    ${EndIf}

    ; ── Create data directories first (needed for logging) ───────────────────
    ; Use $LOCALAPPDATA\..\.. to derive ProgramData on NSIS versions where
    ; $DataDir is not yet a recognised built-in (works on all 3.x builds).
    CreateDirectory "$DataDir\Veyra"
    CreateDirectory "$DataDir\Veyra\logs"
    CreateDirectory "$DataDir\Veyra\crashes"
    CreateDirectory "$DataDir\Veyra\presets"
    CreateDirectory "$DataDir\Veyra\themes"

    ; Abort with a clear message if the log directory could not be created
    ; (e.g. installer somehow not elevated — should not happen with RequestExecutionLevel admin).
    ${IfNot} ${FileExists} "$DataDir\Veyra\logs\*.*"
        MessageBox MB_ICONSTOP \
            "Could not create $DataDir\Veyra\logs.$\r$\n\
Please ensure you are running Setup as Administrator and that$\r$\n\
%ProgramData% is writable (usually C:\ProgramData)."
        Abort
    ${EndIf}

    ; Grant LocalService (audio service account) read+write access
    nsExec::ExecToLog '"$SYSDIR\icacls.exe" "$DataDir\Veyra" /grant "NT AUTHORITY\LocalService:(OI)(CI)M" /T /Q'
    Pop $0

    ; ── Begin logging ─────────────────────────────────────────────────────────
    ${AppendLog} "======================================"
    ${If} $IsUpgrade == "1"
        ${AppendLog} "${PRODUCT_NAME} ${VERSION} — Upgrade from $OldVersion"
        DetailPrint "Upgrading ${PRODUCT_NAME} from $OldVersion to ${VERSION}..."
    ${Else}
        ${AppendLog} "${PRODUCT_NAME} ${VERSION} — Fresh install"
        DetailPrint "Installing ${PRODUCT_NAME} ${VERSION}..."
    ${EndIf}

    ; ── Upgrade: stop running processes ──────────────────────────────────────
    ; A running service holds veyra-service.exe open, so it must reach STOPPED
    ; before the File extraction below — otherwise the new binary silently fails
    ; to overwrite and the upgrade keeps the old service. A fixed Sleep raced and
    ; did exactly that, so we use the freshly-staged helper's --stop-service,
    ; which polls the SCM for STOPPED. The on-disk helper is the OLD version and
    ; may not know the command, so extract the new one to the temp plugins dir.
    ${If} $IsUpgrade == "1"
        DetailPrint "Stopping existing installation..."
        ${AppendLog} "Stopping existing processes..."
        nsExec::ExecToLog '"$SYSDIR\taskkill.exe" /F /IM veyra.exe /IM veyra-overlay.exe'
        Pop $0

        InitPluginsDir
        SetOutPath "$PLUGINSDIR"
        File "staging\VeyraSetupHelper.exe"
        SetOutPath "$INSTDIR"
        nsExec::ExecToLog '"$PLUGINSDIR\VeyraSetupHelper.exe" --log "${LOG_FILE}" --stop-service'
        Pop $0
        ${AppendLog} "stop-service exited: $0"
    ${EndIf}

    ; ── VC++ 2015-2022 x64 runtime ────────────────────────────────────────────
    ; HAVE_VCREDIST is defined by build-installer.ps1 only when vc_redist.x64.exe
    ; was copied into staging/. The File instruction packs files at compile time,
    ; so it must be guarded with !ifdef to avoid a build-time "file not found" error.
    DetailPrint "Checking Visual C++ runtime..."
    ReadRegDWORD $R0 HKLM "SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64" "Installed"
    ${If} $R0 != "1"
!ifdef HAVE_VCREDIST
        ${AppendLog} "VC++ runtime missing — installing bundled redistributable..."
        DetailPrint "Installing Visual C++ 2015-2022 runtime..."
        File /oname=$TEMP\vc_redist_veyra.exe "staging\vc_redist.x64.exe"
        ExecWait '"$TEMP\vc_redist_veyra.exe" /install /quiet /norestart' $R1
        Delete "$TEMP\vc_redist_veyra.exe"
        ${AppendLog} "vc_redist installer exited: $R1"
!else
        ${AppendLog} "VC++ runtime not bundled — skipping"
        DetailPrint "VC++ runtime not bundled. Download from microsoft.com/en-us/download/details.aspx?id=48145 if needed."
!endif
    ${Else}
        ${AppendLog} "VC++ runtime present."
    ${EndIf}

    ; ── Extract files ─────────────────────────────────────────────────────────
    DetailPrint "Installing files..."
    ${AppendLog} "Extracting files to $INSTDIR..."
    SetOutPath "$INSTDIR"

    File "staging\veyra.exe"
    File "staging\veyra-service.exe"
    File "staging\veyra-apo.dll"
    File "staging\veyra-overlay.exe"
    File "staging\VeyraSetupHelper.exe"
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

    ; Driver scripts are developer tools — installed for "Setup Audio Driver" shortcut
    SetOutPath "$INSTDIR\driver"
    File "staging\driver\register-apo.ps1"
    File "staging\driver\associate-apo.ps1"
    File "staging\driver\uninstall-apo.ps1"

    SetOutPath "$INSTDIR"
    ${AppendLog} "Files extracted."

    ; ── Register APO COM server (regsvr32, no window, no PowerShell) ─────────
    DetailPrint "Registering audio component..."
    ${AppendLog} "Running regsvr32 /s veyra-apo.dll..."
    ExecWait '"$SYSDIR\regsvr32.exe" /s "$INSTDIR\veyra-apo.dll"' $R0
    ${AppendLog} "regsvr32 exited: $R0"
    ${If} $R0 != "0"
        ${AppendLog} "WARN: COM registration returned $R0"
        DetailPrint "Audio component registration returned $R0 — continuing..."
    ${EndIf}

    ; Verify registration via native helper (no PowerShell)
    nsExec::ExecToLog '"${HELPER}" --log "${LOG_FILE}" --verify-com'
    Pop $R1
    ${If} $R1 == "0"
        DetailPrint "Audio component verified."
        ${AppendLog} "COM registration verified."
    ${Else}
        DetailPrint "Audio component could not be verified (exit $R1)."
        ${AppendLog} "WARN: COM verify returned $R1"
    ${EndIf}

    ; ── Install Windows service ───────────────────────────────────────────────
    DetailPrint "Installing Veyra Audio Service..."
    ${AppendLog} "Installing service..."
    ExecWait '"$INSTDIR\veyra-service.exe" --install' $R0
    ${AppendLog} "Service install exited: $R0"
    ${If} $R0 != "0"
        ${AppendLog} "ERROR: Service installation failed (exit $R0)"
        MessageBox MB_ICONSTOP \
            "Could not register the ${PRODUCT_NAME} Windows Service (exit code: $R0).$\r$\n$\r$\n\
Possible causes:$\r$\n\
  • Installer is not running as Administrator$\r$\n\
  • A previous incomplete install left a locked service entry$\r$\n$\r$\n\
Check %ProgramData%\Veyra\logs\install.log for details."
        Abort
    ${EndIf}

    DetailPrint "Starting Veyra Audio Service..."
    ${AppendLog} "Starting service..."
    nsExec::ExecToLog '"$SYSDIR\sc.exe" start ${SERVICE_NAME}'
    Pop $0
    ${AppendLog} "sc.exe start exited: $0"
    Sleep 1500

    ; Confirm the service actually reached SERVICE_RUNNING before continuing.
    nsExec::ExecToLog '"${HELPER}" --log "${LOG_FILE}" --verify-service'
    Pop $R1
    ${If} $R1 == "0"
        DetailPrint "Veyra Audio Service is running."
        ${AppendLog} "Service verified RUNNING."
    ${Else}
        DetailPrint "Warning: service may not have started (exit $R1 — see install.log)."
        ${AppendLog} "WARN: verify-service returned $R1"
    ${EndIf}

    ; ── Enumerate devices for the picker page (runs before page displays) ────
    DetailPrint "Detecting audio devices..."
    ${AppendLog} "Listing render endpoints..."
    nsExec::ExecToLog '"${HELPER}" --log "${LOG_FILE}" --list-devices "$TEMP\veyra-devices.ini"'
    Pop $0
    ReadINIStr $DeviceCount "$TEMP\veyra-devices.ini" "Devices" "Count"
    ${If} $DeviceCount == ""
        StrCpy $DeviceCount "0"
    ${EndIf}
    ${AppendLog} "Detected $DeviceCount active render endpoint(s)."

    ; ── Start with Windows ────────────────────────────────────────────────────
    WriteRegStr HKCU "${STARTUP_KEY}" "${STARTUP_VALUE}" '"$INSTDIR\veyra.exe" --minimized'
    ${AppendLog} "Startup key written."

    ; ── Uninstaller ──────────────────────────────────────────────────────────
    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; ── Programs & Features / Settings → Apps ─────────────────────────────────
    ${GetSize} "$INSTDIR" "/S=0K" $R0 $R1 $R2
    IntFmt $R0 "0x%08X" $R0
    WriteRegStr   HKLM "${UNINST_KEY}" "DisplayName"          "${PRODUCT_NAME}"
    WriteRegStr   HKLM "${UNINST_KEY}" "DisplayVersion"       "${VERSION}"
    WriteRegStr   HKLM "${UNINST_KEY}" "Publisher"            "${PRODUCT_PUBLISHER}"
    WriteRegStr   HKLM "${UNINST_KEY}" "URLInfoAbout"         "${PRODUCT_URL}"
    WriteRegStr   HKLM "${UNINST_KEY}" "InstallLocation"      "$INSTDIR"
    WriteRegStr   HKLM "${UNINST_KEY}" "UninstallString"      '"$INSTDIR\uninstall.exe"'
    WriteRegStr   HKLM "${UNINST_KEY}" "QuietUninstallString" '"$INSTDIR\uninstall.exe" /S'
    WriteRegStr   HKLM "${UNINST_KEY}" "DisplayIcon"          "$INSTDIR\veyra.exe"
    WriteRegStr   HKLM "${UNINST_KEY}" "HelpLink"             "${PRODUCT_URL}"
    WriteRegDWORD HKLM "${UNINST_KEY}" "EstimatedSize"        "$R0"
    WriteRegDWORD HKLM "${UNINST_KEY}" "NoModify"             1
    WriteRegDWORD HKLM "${UNINST_KEY}" "NoRepair"             1
    ${AppendLog} "Programs & Features entry written."

    ; ── Start Menu shortcuts ──────────────────────────────────────────────────
    DetailPrint "Creating shortcuts..."
    CreateDirectory "$SMPROGRAMS\Veyra Sounds"
    CreateShortcut "$SMPROGRAMS\Veyra Sounds\Veyra Sounds.lnk" \
                   "$INSTDIR\veyra.exe" "" "$INSTDIR\veyra.exe" 0
    CreateShortcut "$SMPROGRAMS\Veyra Sounds\Veyra Overlay.lnk" \
                   "$INSTDIR\veyra-overlay.exe" "" "$INSTDIR\veyra-overlay.exe" 0
    CreateShortcut "$SMPROGRAMS\Veyra Sounds\Setup Audio Driver (Advanced).lnk" \
                   "$INSTDIR\setup-audio-driver.cmd" "" "$SYSDIR\cmd.exe" 0
    CreateShortcut "$SMPROGRAMS\Veyra Sounds\Uninstall Veyra Sounds.lnk" \
                   "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0

    ${AppendLog} "Install phase complete. Device picker next."

SectionEnd

; =============================================================================
;  Optional: Desktop shortcut (pre-checked)
; =============================================================================
Section /o "Create Desktop shortcut" SecDesktop
    CreateShortcut "$DESKTOP\Veyra Sounds.lnk" \
                   "$INSTDIR\veyra.exe" "" "$INSTDIR\veyra.exe" 0
    ${AppendLog} "Desktop shortcut created."
SectionEnd

; VeyraGUIInit MUST appear after "Section /o ... SecDesktop" so that
; ${SecDesktop} is defined when NSIS (single-pass) compiles this function.
; Called via MUI_CUSTOMFUNCTION_GUIINIT — pre-checks the optional shortcut.
Function VeyraGUIInit
    SectionSetFlags ${SecDesktop} 1
FunctionEnd

; =============================================================================
;  Device Setup custom page (after InstFiles, before Finish)
;  Populated by VeyraSetupHelper.exe --list-devices (C++ IMMDeviceEnumerator).
;  No PowerShell. No endpoint GUIDs shown to user.
; =============================================================================
Function DevicePageCreate
    IfSilent 0 +2
    Abort   ; skip in silent mode — associate default device later

    !insertmacro MUI_HEADER_TEXT "Audio Device Setup" \
        "Choose where ${PRODUCT_NAME} should process your audio."

    nsDialogs::Create 1018
    Pop $hDevicePage
    ${If} $hDevicePage == error
        Abort
    ${EndIf}

    ${If} $DeviceCount == "0"
        ; No devices found — show informational message only
        ${NSD_CreateLabel} 0 0u 100% 60u \
            "No active playback devices were detected.$\r$\n$\r$\n\
You can pick your output device in the Devices screen after Veyra launches.$\r$\n$\r$\n\
To hear the effects, follow the Audio Bridge guide (docs/AUDIO_BRIDGE.md)."
        Pop $0
    ${Else}
        ; Normal: show device picker
        ${NSD_CreateLabel} 0 0u 100% 24u \
            "Veyra will enhance audio on the device you select. \
You can change this at any time in the Devices screen."
        Pop $0

        ${NSD_CreateLabel} 0 30u 64u 14u "Playback device:"
        Pop $0

        ${NSD_CreateDropList} 68u 28u 232u 120u ""
        Pop $hDeviceList

        ; Populate dropdown from INI file written by VeyraSetupHelper
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

        SendMessage $hDeviceList ${CB_ADDSTRING} 0 "STR:Configure later in Devices screen"

        ; Pre-select the system default device
        ReadINIStr $R0 "$TEMP\veyra-devices.ini" "Devices" "Default"
        ${If} $R0 == ""
            StrCpy $R0 0
        ${EndIf}
        SendMessage $hDeviceList ${CB_SETCURSEL} $R0 0

        ${NSD_CreateLabel} 0 48u 100% 36u \
            "Windows Audio restarts briefly after you click Next. \
You may hear a short audio interruption — this is normal."
        Pop $0
    ${EndIf}

    nsDialogs::Show
FunctionEnd

Function DevicePageLeave
    ${If} $DeviceCount == "0"
        ${AppendLog} "Device page: no endpoints found. Skipping APO association."
        Return
    ${EndIf}

    ; Read selected index
    SendMessage $hDeviceList ${CB_GETCURSEL} 0 0 $SelectedIndex

    ; "Configure later" = last item (index == DeviceCount)
    ${If} $SelectedIndex >= $DeviceCount
        StrCpy $SelectedGuid ""
        StrCpy $SelectedName "Not configured"
        ${AppendLog} "Device page: user chose 'Configure later'."
        Return
    ${EndIf}

    ; Look up GUID (kept internal — never shown to user)
    ReadINIStr $SelectedGuid "$TEMP\veyra-devices.ini" "Devices" "Guid$SelectedIndex"
    ReadINIStr $SelectedName "$TEMP\veyra-devices.ini" "Devices" "Name$SelectedIndex"
    ${AppendLog} "Device selected: $SelectedName"

    ; Associate APO via native helper (no PowerShell)
    DetailPrint "Activating audio enhancement on $SelectedName..."
    nsExec::ExecToLog '"${HELPER}" --log "${LOG_FILE}" --associate "$SelectedGuid"'
    Pop $R0

    ${If} $R0 == "0"
        StrCpy $ApoAssociated "1"
        ${AppendLog} "APO association succeeded: $SelectedName"
        DetailPrint "Audio enhancement activated on $SelectedName."

        ; Verify the FxProperties entry was written and is readable.
        nsExec::ExecToLog '"${HELPER}" --log "${LOG_FILE}" --verify-association "$SelectedGuid"'
        Pop $R2
        ${If} $R2 == "0"
            ${AppendLog} "APO association registry entry verified."
        ${Else}
            ${AppendLog} "WARN: APO association verify returned $R2 (see log)"
            DetailPrint "Warning: APO association could not be confirmed in registry."
        ${EndIf}
    ${Else}
        ${AppendLog} "APO association failed (exit $R0): $SelectedName"
        MessageBox MB_ICONINFORMATION \
            "The audio enhancement could not be activated on $SelectedName.$\r$\n$\r$\n\
This is common with Bluetooth headphones. To hear the effects, set up the \
Audio Bridge — see the Quick Start guide or docs/AUDIO_BRIDGE.md in the \
${PRODUCT_NAME} repository." \
            /SD IDOK
    ${EndIf}

    ; Post-install verification (silent — failures only surface in the log)
    ${AppendLog} "--- Post-install verification ---"

    ; 1. Service installed?
    ReadRegStr $R1 HKLM "SYSTEM\CurrentControlSet\Services\${SERVICE_NAME}" "ImagePath"
    ${If} $R1 != ""
        ${AppendLog} "VERIFY OK: Service registered."
    ${Else}
        ${AppendLog} "VERIFY WARN: Service not found in registry."
    ${EndIf}

    ; 2. COM registered?
    ReadRegStr $R1 HKLM "SOFTWARE\Classes\CLSID\${RENDER_CLSID}\InprocServer32" ""
    ${If} $R1 != ""
        ${AppendLog} "VERIFY OK: COM CLSID registered: $R1"
    ${Else}
        ${AppendLog} "VERIFY WARN: COM CLSID not registered."
    ${EndIf}

    ; 3. ProgramData dirs?
    ${If} ${FileExists} "$DataDir\Veyra\logs\*.*"
        ${AppendLog} "VERIFY OK: ProgramData\Veyra\logs exists."
    ${Else}
        ${AppendLog} "VERIFY WARN: ProgramData\Veyra\logs missing."
    ${EndIf}

    ${AppendLog} "--- Verification complete. ApoAssociated=$ApoAssociated ---"
FunctionEnd

; =============================================================================
;  Uninstaller
; =============================================================================

; The uninstaller is a separate process: it does NOT inherit the installer's
; .onInit state. Without this, $SYSDIR resolves to SysWow64 (so the 32-bit
; regsvr32 silently fails to unregister the 64-bit DLL — the same WOW64 bug
; fixed for install), registry deletes target the WOW6432Node view instead of
; the 64-bit view the installer wrote to, and $DataDir is empty (so "delete my
; data" silently removes nothing).
Function un.onInit
    ${If} ${RunningX64}
        ${DisableX64FSRedirection}
        SetRegView 64
    ${EndIf}

    ReadEnvStr $DataDir "PROGRAMDATA"
    ${If} $DataDir == ""
        ReadRegStr $DataDir HKLM \
            "SOFTWARE\Microsoft\Windows\CurrentVersion\Explorer\Shell Folders" \
            "Common AppData"
    ${EndIf}
    ${If} $DataDir == ""
        StrCpy $DataDir "C:\ProgramData"
    ${EndIf}
FunctionEnd

Section "Uninstall"

    MessageBox MB_ICONQUESTION|MB_YESNO \
        "Keep your presets and settings?$\r$\n$\r$\n\
Click Yes to preserve them in %ProgramData%\Veyra.$\r$\n\
Click No to delete all ${PRODUCT_NAME} data." \
        /SD IDYES IDYES keep_data IDNO del_data

    keep_data: StrCpy $R9 "keep"  ; Goto after_q is implicit with label fallthrough
    Goto after_q
    del_data:  StrCpy $R9 "delete"
    after_q:

    ; Stop running UI processes
    DetailPrint "Stopping Veyra..."
    nsExec::ExecToLog '"$SYSDIR\taskkill.exe" /F /IM veyra.exe /IM veyra-overlay.exe'
    Pop $0
    Sleep 800

    ; Remove APO from all endpoints — native helper, no PowerShell
    DetailPrint "Removing audio driver configuration..."
    nsExec::ExecToLog '"$INSTDIR\VeyraSetupHelper.exe" --log "${LOG_FILE}" --unassociate-all'
    Pop $0

    ; Unregister COM (regsvr32, no window)
    ExecWait '"$SYSDIR\regsvr32.exe" /s /u "$INSTDIR\veyra-apo.dll"' $0

    ; Stop and remove service
    DetailPrint "Removing Veyra Audio Service..."
    nsExec::ExecToLog '"$SYSDIR\sc.exe" stop ${SERVICE_NAME}'
    Pop $0
    Sleep 1500
    ExecWait '"$INSTDIR\veyra-service.exe" --uninstall' $0

    ; Delete install directory
    DetailPrint "Removing files..."
    RMDir /r "$INSTDIR"

    ; Optionally delete user data
    ${If} $R9 == "delete"
        RMDir /r "$DataDir\Veyra"
    ${EndIf}

    ; Shortcuts
    Delete "$SMPROGRAMS\Veyra Sounds\Veyra Sounds.lnk"
    Delete "$SMPROGRAMS\Veyra Sounds\Veyra Overlay.lnk"
    Delete "$SMPROGRAMS\Veyra Sounds\Setup Audio Driver (Advanced).lnk"
    Delete "$SMPROGRAMS\Veyra Sounds\Uninstall Veyra Sounds.lnk"
    RMDir  "$SMPROGRAMS\Veyra Sounds"
    Delete "$DESKTOP\Veyra Sounds.lnk"

    ; Registry cleanup
    DeleteRegValue HKCU "${STARTUP_KEY}" "${STARTUP_VALUE}"
    DeleteRegKey   HKLM "${UNINST_KEY}"

    ; Remove the CLSID entry left by regsvr32 (belt-and-suspenders)
    DeleteRegKey HKLM "SOFTWARE\Classes\CLSID\${RENDER_CLSID}"

    ; Remove the backup key where --associate saved original ModeEffect CLSIDs
    DeleteRegKey HKLM "SOFTWARE\Veyra\Devices"
    DeleteRegKey /ifempty HKLM "SOFTWARE\Veyra"

    ; Remove any FxProperties entries NSIS can reach directly
    ; (belt-and-suspenders after --unassociate-all above)
    StrCpy $0 0
    unasc_loop:
        EnumRegKey $1 HKLM \
            "SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render" $0
        StrCmp $1 "" unasc_done
        ReadRegStr $2 HKLM \
            "SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render\$1\FxProperties" \
            "${FX_POSTMIX_KEY}"
        StrCmp $2 "${RENDER_CLSID}" 0 unasc_next
        DeleteRegValue HKLM \
            "SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render\$1\FxProperties" \
            "${FX_POSTMIX_KEY}"
        unasc_next:
        IntOp $0 $0 + 1
        Goto unasc_loop
    unasc_done:

    DetailPrint "${PRODUCT_NAME} uninstalled."

SectionEnd
