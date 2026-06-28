@echo off
:: Veyra Sounds — Audio Driver Setup Helper
:: Launches the APO endpoint association wizard with administrator privileges.
:: Called from the "Setup Audio Driver (Advanced)" Start Menu shortcut.

setlocal

:: Resolve the directory where this .cmd file lives (the install directory)
set "VEYRA_DIR=%~dp0"

:: Check if we are already elevated
net session >nul 2>&1
if %ERRORLEVEL% EQU 0 goto :run_elevated

:: Not elevated — re-launch this script with UAC prompt
echo Requesting administrator privileges...
powershell -NoProfile -NonInteractive -Command ^
  "Start-Process cmd -ArgumentList '/c \"%~f0\"' -Verb RunAs -Wait"
exit /b

:run_elevated
echo.
echo  Veyra Sounds - Audio Driver Setup
echo  ==================================
echo.
echo  This wizard associates the Veyra audio effect with a render (output)
echo  endpoint so it processes audio at the system level — no virtual cable
echo  required. Test-signing must be enabled for unsigned builds.
echo.
echo  Prerequisites:
echo    1. test-signing ON:  bcdedit /set testsigning on  (then reboot once)
echo    2. Service running:  Start Veyra Sounds first (brand LED green)
echo.
echo  Press Ctrl+C to cancel, or
pause

:: Run the PowerShell association script interactively
powershell.exe -NoProfile -ExecutionPolicy Bypass ^
  -File "%VEYRA_DIR%driver\associate-apo.ps1"

echo.
echo  Done. If the LED turns green in Veyra Sounds and audio plays through
echo  your chosen endpoint, the APO is active.
echo.
pause
endlocal
