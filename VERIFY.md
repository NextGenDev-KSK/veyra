# Veyra Sounds — Run & Test Runbook

A hands-on, step-by-step pass to **build, run, hear, and test** Veyra.

**Stage order:**
- **Stage A** — app + service + IPC (no admin, no audio). Proves connectivity.
- **Stage B** — APO path ⭐ (test-signing + admin). The primary way to hear effects.
- **Stage C** — Audio Bridge (no admin, needs VB-CABLE). Advanced fallback for
  Bluetooth endpoints that reject the APO.
- **Stage D** — Overlay / Gamer Mode.

> Host: Windows 11. Report back per stage with ✅ / ❓ / ❌ and I'll fix forward.

---

## 0. One-time toolchain setup

Install **Visual Studio 2022 Build Tools** (free) → tick the **"Desktop
development with C++"** workload. That single workload gives MSVC `cl`, the
Windows 11 SDK, **CMake**, and **Ninja**. Git you already have. (No WDK needed.)

---

## 1. Build

Open **"x64 Native Tools Command Prompt for VS 2022"** (Start menu), then:

```sh
cd C:\Users\renuk\Documents\veyra
git submodule update --init --recursive            REM JUCE + spdlog (first time)
cmake --preset windows-release -DVEYRA_BUILD_TESTS=ON
cmake --build --preset windows-release
```

First configure builds JUCE (a few minutes). Output → **`build\windows-release\bin\`**:
`veyra.exe` (UI), `veyra-service.exe` (service), `veyra-apo.dll` (effect),
`veyra-overlay.exe` (Gamer HUD).

Run the unit tests (same as CI) to confirm the engines:

```sh
ctest --test-dir build\windows-release --output-on-failure
```
Expect `dsp_tests` and `common_tests` both **passed**.

---

## 2. Stage A — App + Service + IPC  (no admin, START HERE)

Two terminals (the dev prompt is fine; no admin):

```sh
REM Terminal 1 — the service (keep it open; it prints activity)
build\windows-release\bin\veyra-service.exe --console

REM Terminal 2 — the UI
build\windows-release\bin\veyra.exe
```

Check, in order:

1. ✅ Glass window opens; the **brand LED** (top-left, by the "V") turns **green** = connected to the service. (Amber = service not reachable → check Terminal 1.)
2. ✅ Drag **EQ bands** / **knobs**, toggle **Master**, move the **master volume** → Terminal 1 prints `ConfigManager: applied + saved config` each change. `%ProgramData%\Veyra\config.json` updates live.
3. ✅ **Sidebar → Settings → Appearance**: theme tiles recolor the whole UI live; **UI Opacity** lowers (you'll see the desktop blurred through the glass = acrylic); **Reduce Motion** freezes the visualizer.
4. ✅ **Settings** also has **Microphone**, **Spatial**, **Loudness**, and **About** cards — move their controls, watch Terminal 1 log the config writes.
5. ✅ **Presets** (sidebar): apply a built-in (Bass Boost, FPS Footstep Boost…); the EQ/knobs jump to it. Save current, import/export `.vpreset`.
6. ✅ **Devices** (sidebar): the **Audio Bridge** card lists your output devices (incl. the Nord Buds when connected).
7. ✅ **Sidebar → Mini Mode** → compact always-on-top widget (preset, master, peak bars); expand returns. **Tray** icon menu works.

This proves everything *except* audio passing through the DSP. That's Stage B.

Stop: close the UI, `Ctrl+C` Terminal 1.

---

## 3. Stage B — APO path: HEAR the effects ⭐  (admin + test-signing)

This is the **primary** path. The APO loads into `audiodg.exe` and processes
every app's audio on your chosen output — no virtual cable, no rerouting.
Full steps in [BUILD_GUIDE.md](BUILD_GUIDE.md) §2:

1. **Enable test-signing and reboot** (one-time; admin):
   ```sh
   bcdedit /set testsigning on
   ```
   Reboot. Confirm: `bcdedit /enum | findstr testsigning` → `testsigning Yes`.

2. **Register the COM server** (elevated, from the repo root):
   ```sh
   cd installer\driver
   powershell -ExecutionPolicy Bypass .\register-apo.ps1 -DllPath ..\..\build\windows-release\bin\veyra-apo.dll
   ```

3. **Associate the APO with your output endpoint** (elevated; interactive picker):
   ```sh
   powershell -ExecutionPolicy Bypass .\associate-apo.ps1
   ```
   Pick your wired/built-in/USB output. For the mic: add `-Capture`. To remove: `-Unassociate`.
   The script restarts `AudioSrv` so `audiodg.exe` reloads the chain.

4. **Start the service and UI**:
   ```sh
   build\windows-release\bin\veyra-service.exe --console
   build\windows-release\bin\veyra.exe
   ```

5. **Play audio** through the associated endpoint. Move the EQ, switch Presets,
   toggle Spatial → Cinematic — effects should be audible in real time (< 5 ms).

Verify the APO is loaded: **Process Explorer** → `audiodg.exe` → lower pane → DLLs → filter `veyra-apo`.

⚠️ Bluetooth endpoints often reject custom APOs. If you hear no effect (or silence)
after association, try the Audio Bridge path (Stage C) for those endpoints.

---

## 4. Stage C — Audio Bridge (advanced; Bluetooth fallback)

Use Stage C **only** if Stage B does not work on your target endpoint (typically
Bluetooth). The bridge loopback-captures a virtual sink, processes audio through
the DSP, and renders to your headphones. No admin needed but requires a virtual
cable installation.

1. **Install VB-CABLE** (one-time): vb-audio.com/Cable → run installer → reboot.
   It adds `CABLE Input` (playback) + `CABLE Output` (capture).
2. **Set CABLE Input as the Windows default output** (Sound Settings → Output).
   Apps now play into the cable; you'll hear nothing yet.
3. **Match formats:** CABLE Input and your headphones → Properties → Advanced →
   same rate (e.g. 48 kHz 16-bit).
4. **Start the service and UI.**
5. In the UI → **Devices**:
   - **Source** = `CABLE Output`
   - **Output** = your Bluetooth headphones
   - toggle **Enable** on.
6. **Play music.** Effects should be audible through the headphones.

> Latency: ~30–80 ms (loopback). Use Stage B / APO path where possible.

❓ If you hear nothing: confirm CABLE Input is the default output and its volume
meter moves in Sound Settings; check the service terminal for `AudioBridge:` lines.

---

## 5. Stage D — Gamer Mode overlay + Sound Tracker

```sh
build\windows-release\bin\veyra-overlay.exe
```
Enable **Gamer Mode** in the UI → the radar HUD appears (competitive / rich /
compass styles). With the service running and audio playing into the loopback,
the tracker writes **blips** (footsteps/gunshots/voice) with direction.

---

## 6. Feature test checklist

| Feature | Where | Expected |
| --- | --- | --- |
| 10-band EQ + curve | Home | Drag nodes → tooltip (Hz + dB); audio changes (Stage B APO) |
| Bass/Treble/Volume/Width/Compression knobs | Home | Live value text; audio changes |
| Presets (8 built-in + user) | Presets | Apply/save/import/export `.vpreset` |
| Per-app rules | `%ProgramData%\Veyra\app_rules.json` | Focus an app → its preset auto-applies (see below) |
| Spatial (crossfeed + virtualization) | Settings → Spatial | Cinematic = wide/out-of-head; Competitive = neutral |
| Night Mode | Settings → Loudness | Quiet parts lifted, peaks tamed |
| Loudness Match | Settings → Loudness | Levels normalize toward target LUFS |
| Sleep Timer | Settings → Loudness | System volume fades to silence over the tail |
| Mic chain | Settings → Microphone | (needs the capture path; config persists) |
| Theme / opacity / reduce-motion | Settings → Appearance | Live + persists across restart |

**Per-app rules quick test:** create `%ProgramData%\Veyra\app_rules.json`:
```json
[ { "match": "chrome", "preset_uuid": "v-bass-boost", "priority": 0, "enabled": true },
  { "match": "spotify", "preset_uuid": "v-cinematic", "priority": 5, "enabled": true } ]
```
Focus Chrome → EQ jumps to Bass Boost; focus Spotify → Cinematic. (No file = nothing changes.)

---

## 7. Where things live / troubleshooting

- Config: `%ProgramData%\Veyra\config.json` · Logs: `%ProgramData%\Veyra\logs\veyra-service.log`
- Presets: `%ProgramData%\Veyra\presets\*.vpreset` · Crashes: `%ProgramData%\Veyra\crashes\`
- App-rules: `%ProgramData%\Veyra\app_rules.json`
- **Build fails** → paste the first error from `cmake --build`.
- **LED stays amber** → the service isn't running / a stale `veyra-service.exe` is; restart Terminal 1.
- **No audio in Stage B** → see the ❓ note in §3.
- **Send back:** which ✅ passed, any ❓/❌ with the exact console error or
  `veyra-service.log` tail (or a screenshot for visual issues).
