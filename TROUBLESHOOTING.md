# Veyra Sounds — Troubleshooting Guide

This guide covers the most common installation and runtime problems, their root causes, and step-by-step fixes. For hardware validation, see [HARDWARE_VALIDATION.md](HARDWARE_VALIDATION.md).

---

## Quick diagnosis

Before diving into specific issues, run these two checks first — they tell you whether each layer is healthy.

**Check 1 — Is the service running?** Open the Windows Run dialog (`Win + R`), type `services.msc`, press Enter. Look for **Veyra Audio Service**. It should show Status = **Running** and Startup Type = **Automatic**.

**Check 2 — Is the Veyra tray icon present?** Look in the system tray (bottom-right corner of the taskbar). The Veyra icon should be visible. If not, launch Veyra from Start → Veyra Sounds.

**Check 3 — Does the top bar LED show green?** In the Veyra app, the circular LED in the top bar is green when connected to the service. Amber means the UI is reconnecting. Grey means disconnected.

> **For developers and advanced users:** Command-line diagnostics are covered in [BUILD_GUIDE.md](BUILD_GUIDE.md).

---

## Problem 1 — UI brand LED stays amber (service not connected)

**Symptom:** The circular LED in the Veyra app top bar is amber or grey, not green.

**Root cause:** The UI cannot reach the service's named pipe `\\.\pipe\veyra-control`.

**Diagnostic steps:**

| Step | Command | Expected result |
|------|---------|-----------------|
| 1 | `sc query VeyraAudioService` | `STATE: 4 RUNNING` |
| 2 | `Get-ChildItem \\.\pipe\` \| `Where Name -eq veyra-control` | One entry |
| 3 | Check Event Viewer → Windows Logs → Application, Source=VeyraAudioService | No ERROR entries |

**Fixes by scenario:**

*Service not installed:*
```cmd
# Elevated prompt:
veyra-service.exe --install
sc start VeyraAudioService
```

*Service installed but stopped:*
```cmd
sc start VeyraAudioService
```

*Service crashes on start:*
```cmd
# Run in console mode to see the error directly:
veyra-service.exe --console
```
Look for error lines; common cause is `%ProgramData%\Veyra` not writable by `NT AUTHORITY\LocalService`. Fix: `icacls "C:\ProgramData\Veyra" /grant "NT AUTHORITY\LocalService:(OI)(CI)M"`.

*Pipe exists but UI still shows amber:*
The service session validation may be rejecting the UI. Check the service log at `%ProgramData%\Veyra\logs\veyra-service.log` for `PipeServer: rejected pid=` lines. If the machine has multiple users logged in (RDP), only the active console session can connect — this is by design.

---

## Problem 2 — No audio effect heard (APO path)

**Symptom:** Veyra is running, the LED is green, but EQ changes have no effect on audio.

**Check 1 — Is the APO loaded? (advanced)**
Download [Process Explorer](https://learn.microsoft.com/en-us/sysinternals/downloads/process-explorer) (free Microsoft tool), find `audiodg.exe`, open its lower DLL pane, and search for `veyra-apo`. If it is absent, the APO did not load — proceed to the fixes below. If it is present, the APO is loaded and the issue may be in the device association (Check 2).

**Check 2 — Is Veyra set as the audio processor for the correct output?**
Open Veyra → Devices. The device listed under "Preferred Output" should match the output you are playing through. If no device is selected, choose your speakers or headphones from the list.

**Fix A — Re-run audio device setup:**
1. Open Start → **Veyra Sounds** → **Setup Audio Driver (Advanced)**
2. Follow the on-screen steps to re-associate Veyra with your output device
3. Restart your computer and try again

**Fix B — Reinstall Veyra:**
If Fix A does not work, uninstall Veyra via Settings → Apps → Veyra Sounds → Uninstall, then download and reinstall from the [releases page](https://github.com/NextGenDev-KSK/veyra/releases). The installer re-registers all components automatically.

> **Note for open-source builds:** If you built Veyra from source without a production-signed driver package, the APO will not load into `audiodg.exe` on a standard Windows installation. This is a Windows security requirement. See [installer/SIGNING.md](installer/SIGNING.md) for signing options. This does not affect users who installed from the official VeyraSetup.exe.

**Fix C — Windows Audio service crash loop (advanced):**
If Windows plays a short burst of audio then stops, or Task Manager shows `audiodg.exe` repeatedly restarting, open Event Viewer → Windows Logs → System, filter by Source = AudioDg, and look for crash events. If crashes began after installing Veyra, uninstall and reinstall to reset the component registration. If you are building from source, see [BUILD_GUIDE.md](BUILD_GUIDE.md) for console-mode diagnostics.

---

## Problem 3 — No audio effect heard (Audio Bridge — Bluetooth fallback)

> The APO is Veyra's primary design, but it only loads on a **signed** build. On this unsigned open-source release the APO does not load into `audiodg.exe`, so the **Audio Bridge is the working audio path** — and it is the *only* option for Bluetooth A2DP endpoints, which never host a custom APO regardless of signing. On a future signed build the APO becomes primary and the Bridge is needed only for Bluetooth. See the APO note in the [Release Notes](RELEASE_NOTES.md).

**Symptom:** Audio Bridge is enabled and configured, but audio through the
Bluetooth headphones is unchanged.

**Check 1 — Is the virtual cable set as the Windows default output?**
Windows Sound Settings → Output must show `CABLE Input` (or equivalent) as the default.
Apps play *into* the cable; the Bridge captures from `CABLE Output` and renders to the headphones.

**Check 2 — Is the Bridge active?**
Open `%ProgramData%\Veyra\config.json` and confirm the `bridge` block has
`"enabled": true` with the correct `source_device_id` / `target_device_id`
(setup steps in [docs/AUDIO_BRIDGE.md](docs/AUDIO_BRIDGE.md)). Then check
`%ProgramData%\Veyra\logs\veyra-service.log` for `AudioBridge:` lines.

**Check 3 — Source and Output must be different devices.**
Source = `CABLE Output` (capture); Output = headphones. They cannot be the same device.

**Fix A — Bridge fails to open the source device:**
Common error: `AUDCLNT_E_DEVICE_IN_USE` — another app has exclusive mode. Close it or switch to shared mode.

**Fix B — Sample rate mismatch:**
Set CABLE Input and your headphones to the same rate (48 kHz, 16-bit) in Windows Sound → Properties → Advanced. The Bridge logs and idles on mismatch.

**Fix C — Bridge runs but output volume is zero:**
Check the Master Volume slider in Veyra and the system volume on the Output endpoint.

---

## Problem 4 — App rules not persisting after restart

**Symptom:** Per-app rules saved in the Apps screen disappear after restarting Veyra.

**Root cause (pre-v1.0.0):** Rules were written to `%APPDATA%\Veyra\app_rules.json` by the UI but read from `%ProgramData%\Veyra\app_rules.json` by the service. Fixed in v1.0.0 — both now use `%ProgramData%\Veyra\app_rules.json`.

**Fix:** Update to v1.0.0 or later. If upgrading from a pre-1.0 version and your rules disappeared, uninstall and reinstall Veyra — the installer creates the correct data directory structure automatically.

---

## Problem 5 — Overlay radar not showing blips

**Symptom:** `veyra-overlay.exe` is running and Gamer Mode is on, but no blips appear on the radar.

**Check 1 — Is the tracker actually running?**
In Veyra → Gamer Mode, the Sound Tracker card should show "Active". If it shows "Stopped", Gamer Mode is off or the service couldn't open the loopback capture.

**Check 2 — Is shared memory accessible?**
The overlay reads from `Global\VeyraTracker_v1`. When the service runs as an installed service (Session 0), this is a cross-session object. When running `--console`, it falls back to `Local\VeyraTracker_v1` in the same session, so both must be in the same mode.

**Fix — session mismatch:**
Always run both the service and the overlay in the same mode:
- For dev: `veyra-service.exe --console` + run overlay in the same session.
- For production: install the service (`--install`) + overlay runs as a user process.

**Fix — no loopback capture device:**
The tracker uses WASAPI loopback on the default render endpoint. If the default output changes (e.g. Bluetooth disconnects), the tracker loses its capture handle. Restart the service to reconnect: `sc stop VeyraAudioService && sc start VeyraAudioService`.

---

## Problem 6 — Crash recovery banner appears on every launch

**Symptom:** The "Veyra recovered from a crash" banner appears every time you start the app.

**Cause:** The crash report file in `%ProgramData%\Veyra\crashes\` is not being deleted after it is shown (it is kept for diagnostics).

**Fix:** Click "Open Folder" in the banner, then delete the `.vcrash` files manually. The banner will no longer appear.

---

## Problem 7 — Veyra won't launch (second-instance check)

**Symptom:** Double-clicking `veyra.exe` does nothing; no window appears.

**Cause:** `moreThanOneInstanceAllowed()` returns `false` in `Main.cpp`. If a previous instance is still alive (e.g. the tray icon is present), a new launch is silently dropped.

**Fix:** Right-click the Veyra tray icon → Quit, then relaunch. Or: `taskkill /F /IM veyra.exe`.

---

## Problem 8 — High CPU from audiodg.exe

**Symptom:** Task Manager shows `audiodg.exe` using 10–30% CPU during audio playback with Veyra's APO active.

**Normal range:** < 3% on a mid-range CPU (Ryzen 5 5600 / Core i5-12400).

**Likely causes and fixes:**

| Cause | Fix |
|-------|-----|
| Denormals not suppressed (no FTZ/DAZ set) | Update to v0.4.0+; the APO sets FTZ/DAZ on first `APOProcess` call |
| Very high sample rate (192 kHz) | Set Windows audio to 48 kHz in Sound → Playback → Properties |
| Reverb amount at 100% with large buffer | Reduce reverb or increase audio buffer size |
| HRTF path active with large KEMAR set | Expected; the HRTF convolution is compute-intensive at high quality |

---

## Problem 9 — Settings → Launch at Startup doesn't work

**Symptom:** The "Launch at startup" toggle in Settings → About is on, but Veyra doesn't launch with Windows.

**Cause:** The startup entry is written to `HKCU\SOFTWARE\Microsoft\Windows\CurrentVersion\Run` by the UI. This key is user-specific and works for the logged-in user.

**Verify:**
```powershell
Get-ItemProperty "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" -Name VeyraSounds -EA SilentlyContinue
```

**Fix if missing:** Toggle the switch off and back on in Settings → About. If still missing, the app may not have write access to the Run key — this is unusual but can happen with managed enterprise machines.

---

## Log files and diagnostics

| File | Contents |
|------|---------|
| `%ProgramData%\Veyra\logs\veyra-service.log` | Service startup, IPC, config, APO publisher, tracker |
| `%ProgramData%\Veyra\logs\veyra-audio-bridge.log` | AudioBridge WASAPI events |
| `%ProgramData%\Veyra\crashes\*.vcrash` | Minidump + JSON from crashed sessions |
| Event Viewer → Windows Logs → System → Source=AudioDg | APO load/unload and crash info |
| Event Viewer → Windows Logs → System → Source=Service Control Manager | Service start/stop failures |

---

## Full clean uninstall (manual)

If the uninstaller fails or is unavailable:

```powershell
# 1. Stop and remove service (elevated)
sc stop VeyraAudioService
sc delete VeyraAudioService

# 2. Remove APO associations
pwsh "C:\Program Files\Veyra Sounds\driver\uninstall-apo.ps1" `
     -DllPath "C:\Program Files\Veyra Sounds\veyra-apo.dll"

# 3. Unregister COM
regsvr32 /s /u "C:\Program Files\Veyra Sounds\veyra-apo.dll"

# 4. Delete files
Remove-Item -Recurse -Force "C:\Program Files\Veyra Sounds"

# 5. Remove shortcuts
Remove-Item -Recurse -Force "$env:APPDATA\Microsoft\Windows\Start Menu\Programs\Veyra Sounds"
Remove-Item -Force "$env:USERPROFILE\Desktop\Veyra Sounds.lnk" -EA SilentlyContinue

# 6. Remove registry
Remove-Item -Recurse "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\VeyraSounds" -EA SilentlyContinue
Remove-ItemProperty "HKCU:\SOFTWARE\Microsoft\Windows\CurrentVersion\Run" -Name VeyraSounds -EA SilentlyContinue

# 7. (Optional) Remove app data
Remove-Item -Recurse -Force "$env:ProgramData\Veyra"
```
