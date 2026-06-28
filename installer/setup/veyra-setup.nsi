; =============================================================================
;  Veyra Sounds — NSIS Installer Script
;  Requires: NSIS 3.x  (https://nsis.sourceforge.io)
;
;  Build with:
;    pwsh installer/setup/build-installer.ps1 -BinDir build/windows-release/bin
;  Or directly:
;    makensis /DVERSION=0.9.0 installer/setup/veyra-setup.nsi
;
;  What this installer does:
;    1. Extracts all binaries and resources to %ProgramFiles%\Veyra Sounds
;    2. Registers the APO COM server (regsvr32 /s)
;    3. Installs the Windows service (auto-start, NT AUTHORITY\LocalService)
;    4. Starts the service
;    5. Creates Start Menu shortcuts (including audio driver setup helper)
;    6. Creates optional Desktop shortcut
;    7. Writes Programs & Features / Settings → Apps uninstall entry
;    8. Writes the uninstaller (uninstall.exe)
;
;  APO endpoint association is NOT performed automatically — it requires
;  choosing the target audio device. Use the "Setup Audio Driver" shortcut
;  or the Devices screen in the app after install.
; =============================================================================

!include "MUI2.nsh"
!include "LogicLib.nsh"
!include "x64.nsh"
!include "WinVer.nsh"
!include "FileFunc.nsh"

; ---- Version (overridden by /DVERSION=x.y.z on the makensis command line) ---
!ifndef VERSION
  !define VERSION "0.9.0"
!endif

; ---- Product identity -------------------------------------------------------
!define PRODUCT_NAME      "Veyra Sounds"
!define PRODUCT_PUBLISHER "Veyra Sounds"
!define PRODUCT_URL       "https://github.com/NextGenDev-KSK/veyra"
!define SERVICE_NAME      "VeyraAudioService"
!define UNINST_KEY        "Software\Microsoft\Windows\CurrentVersion\Uninstall\VeyraSounds"
!define STARTUP_KEY       "Software\Microsoft\Windows\CurrentVersion\Run"
!define STARTUP_VALUE     "VeyraSounds"

; ---- Output -----------------------------------------------------------------
Name "${PRODUCT_NAME} ${VERSION}"
OutFile "veyra-sounds-setup-${VERSION}-x64.exe"
InstallDir "$PROGRAMFILES64\Veyra Sounds"
InstallDirRegKey HKLM "${UNINST_KEY}" "InstallLocation"
RequestExecutionLevel admin
SetCompressor /SOLID lzma
Unicode true

; ---- MUI2 pages -------------------------------------------------------------
!define MUI_ABORTWARNING
!define MUI_WELCOMEPAGE_TITLE   "Welcome to ${PRODUCT_NAME} ${VERSION}"
!define MUI_WELCOMEPAGE_TEXT    "${PRODUCT_NAME} is a free, open-source system-wide audio enhancer for Windows.$\r$\n$\r$\nClick Next to continue.$\r$\n$\r$\nNote: administrator privileges are required to install the audio service and COM component."
!define MUI_LICENSEPAGE_CHECKBOX
!define MUI_FINISHPAGE_RUN      "$INSTDIR\veyra.exe"
!define MUI_FINISHPAGE_RUN_TEXT "Launch ${PRODUCT_NAME} now"
!define MUI_FINISHPAGE_LINK     "Open quick-start guide"
!define MUI_FINISHPAGE_LINK_LOCATION "$INSTDIR\README-PORTABLE.txt"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE  "staging\LICENSE"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

!insertmacro MUI_LANGUAGE "English"

; ---- Version info block (shows in file properties) --------------------------
VIProductVersion "${VERSION}.0"
VIAddVersionKey /LANG=1033 "ProductName"     "${PRODUCT_NAME}"
VIAddVersionKey /LANG=1033 "ProductVersion"  "${VERSION}"
VIAddVersionKey /LANG=1033 "CompanyName"     "${PRODUCT_PUBLISHER}"
VIAddVersionKey /LANG=1033 "LegalCopyright"  "GPLv3 — see LICENSE"
VIAddVersionKey /LANG=1033 "FileDescription" "${PRODUCT_NAME} Setup"
VIAddVersionKey /LANG=1033 "FileVersion"     "${VERSION}.0"

; =============================================================================
; Install section
; =============================================================================

Section "${PRODUCT_NAME}" SecCore

    SectionIn RO  ; required — cannot be deselected

    ; ── Minimum OS check: Windows 10 v2004 (build 19041) ──────────────────
    ${IfNot} ${AtLeastBuild} 19041
        MessageBox MB_ICONSTOP "${PRODUCT_NAME} requires Windows 10 version 2004 (build 19041) or later.$\r$\nYour current build is too old."
        Abort
    ${EndIf}

    ; ── Install files ──────────────────────────────────────────────────────
    SetOutPath "$INSTDIR"

    File "staging\veyra.exe"
    File "staging\veyra-service.exe"
    File "staging\veyra-apo.dll"
    File "staging\veyra-overlay.exe"
    File "staging\LICENSE"
    File "staging\README-PORTABLE.txt"
    File "staging\setup-audio-driver.cmd"

    SetOutPath "$INSTDIR\hrtf\diffuse"
    File /r "staging\hrtf\diffuse\*.*"

    SetOutPath "$INSTDIR\resources\lang"
    File /nonfatal /r "staging\resources\lang\*.json"

    SetOutPath "$INSTDIR\driver"
    File "staging\driver\register-apo.ps1"
    File "staging\driver\associate-apo.ps1"
    File "staging\driver\uninstall-apo.ps1"

    SetOutPath "$INSTDIR"

    ; ── Register APO COM server ────────────────────────────────────────────
    DetailPrint "Registering audio COM component..."
    ExecWait '"$SYSDIR\regsvr32.exe" /s "$INSTDIR\veyra-apo.dll"' $R0
    ${If} $R0 != 0
        MessageBox MB_ICONEXCLAMATION "Audio component registration returned $R0.$\r$\n$\r$\nThe Audio Bridge path will work without it. To use the APO path later, run 'Setup Audio Driver' from the Start Menu (requires test-signing for unsigned builds).$\r$\n$\r$\nContinuing install..."
    ${EndIf}

    ; ── Install Windows service ────────────────────────────────────────────
    DetailPrint "Installing Veyra Audio Service..."
    ExecWait '"$INSTDIR\veyra-service.exe" --install' $R0
    ${If} $R0 != 0
        MessageBox MB_ICONEXCLAMATION "Service installation returned $R0.$\r$\n$\r$\nTo install manually later (elevated prompt):\n  veyra-service.exe --install$\r$\n$\r$\nContinuing install..."
    ${EndIf}

    ; ── Start the service ──────────────────────────────────────────────────
    DetailPrint "Starting Veyra Audio Service..."
    ExecWait '"$SYSDIR\sc.exe" start ${SERVICE_NAME}' $R0
    ; $R0 != 0 is non-fatal (service may already be running, or first-start delay)

    ; ── Shortcuts ──────────────────────────────────────────────────────────
    CreateDirectory "$SMPROGRAMS\Veyra Sounds"
    CreateShortcut "$SMPROGRAMS\Veyra Sounds\Veyra Sounds.lnk" \
                   "$INSTDIR\veyra.exe" "" "$INSTDIR\veyra.exe" 0
    CreateShortcut "$SMPROGRAMS\Veyra Sounds\Veyra Overlay.lnk" \
                   "$INSTDIR\veyra-overlay.exe" "" "$INSTDIR\veyra-overlay.exe" 0
    CreateShortcut "$SMPROGRAMS\Veyra Sounds\Setup Audio Driver (Advanced).lnk" \
                   "$INSTDIR\setup-audio-driver.cmd" "" "$SYSDIR\cmd.exe" 0
    CreateShortcut "$SMPROGRAMS\Veyra Sounds\Uninstall Veyra Sounds.lnk" \
                   "$INSTDIR\uninstall.exe" "" "$INSTDIR\uninstall.exe" 0

    CreateShortcut "$DESKTOP\Veyra Sounds.lnk" \
                   "$INSTDIR\veyra.exe" "" "$INSTDIR\veyra.exe" 0

    ; ── Uninstaller ────────────────────────────────────────────────────────
    WriteUninstaller "$INSTDIR\uninstall.exe"

    ; ── Programs & Features / Settings → Apps registry entry ──────────────
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
    WriteRegDWORD HKLM "${UNINST_KEY}" "EstimatedSize"        "$R0"
    WriteRegDWORD HKLM "${UNINST_KEY}" "NoModify"             1
    WriteRegDWORD HKLM "${UNINST_KEY}" "NoRepair"             1

SectionEnd

; =============================================================================
; Uninstall section
; =============================================================================

Section "Uninstall"

    ; ── Remove APO associations + unregister COM ───────────────────────────
    DetailPrint "Removing audio driver associations..."
    nsExec::ExecToLog 'powershell.exe -NonInteractive -ExecutionPolicy Bypass \
        -File "$INSTDIR\driver\uninstall-apo.ps1" \
        -DllPath "$INSTDIR\veyra-apo.dll"'

    ; ── Stop and uninstall the service ────────────────────────────────────
    DetailPrint "Removing Veyra Audio Service..."
    ExecWait '"$INSTDIR\veyra-service.exe" --uninstall' $R0

    ; ── Stop running processes ─────────────────────────────────────────────
    ExecWait '"$SYSDIR\taskkill.exe" /F /IM veyra.exe'         $R0
    ExecWait '"$SYSDIR\taskkill.exe" /F /IM veyra-overlay.exe' $R0

    ; ── Belt-and-suspenders COM unregistration ─────────────────────────────
    ExecWait '"$SYSDIR\regsvr32.exe" /s /u "$INSTDIR\veyra-apo.dll"' $R0

    ; ── Delete install directory ───────────────────────────────────────────
    RMDir /r "$INSTDIR"

    ; ── Remove shortcuts ───────────────────────────────────────────────────
    Delete "$SMPROGRAMS\Veyra Sounds\Veyra Sounds.lnk"
    Delete "$SMPROGRAMS\Veyra Sounds\Veyra Overlay.lnk"
    Delete "$SMPROGRAMS\Veyra Sounds\Setup Audio Driver (Advanced).lnk"
    Delete "$SMPROGRAMS\Veyra Sounds\Uninstall Veyra Sounds.lnk"
    RMDir  "$SMPROGRAMS\Veyra Sounds"
    Delete "$DESKTOP\Veyra Sounds.lnk"

    ; ── Remove startup entry (if the app wrote one via Settings) ──────────
    DeleteRegValue HKCU "${STARTUP_KEY}" "${STARTUP_VALUE}"

    ; ── Remove Programs & Features entry ──────────────────────────────────
    DeleteRegKey HKLM "${UNINST_KEY}"

SectionEnd
