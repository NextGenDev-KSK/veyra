# Veyra Sounds — Run & Test Runbook

A hands-on, step-by-step pass to **build, run, hear, and test** Veyra. Do the
stages in order. Stage A (no admin) proves the app + service + IPC. Stage B is
the one that makes you **actually hear effects** on your headphones (incl.
Bluetooth) with no driver. Stages C/D are optional/advanced.

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
2. ✅ Drag **EQ bands** / **knobs**, toggle **Master**, move the **master volume** → Terminal 1 prints `ConfigManager: applied + saved config` each change. `%APPDATA%\Veyra\config.json` updates live.
3. ✅ **Sidebar → Settings → Appearance**: theme tiles recolor the whole UI live; **UI Opacity** lowers (you'll see the desktop blurred through the glass = acrylic); **Reduce Motion** freezes the visualizer.
4. ✅ **Settings** also has **Microphone**, **Spatial**, **Loudness**, and **About** cards — move their controls, watch Terminal 1 log the config writes.
5. ✅ **Presets** (sidebar): apply a built-in (Bass Boost, FPS Footstep Boost…); the EQ/knobs jump to it. Save current, import/export `.vpreset`.
6. ✅ **Devices** (sidebar): the **Audio Bridge** card lists your output devices (incl. the Nord Buds when connected).
7. ✅ **Sidebar → Mini Mode** → compact always-on-top widget (preset, master, peak bars); expand returns. **Tray** icon menu works.

This proves everything *except* audio passing through the DSP. That's Stage B.

Stop: close the UI, `Ctrl+C` Terminal 1.

---

## 3. Stage B — HEAR the effects (Audio Bridge, no driver)  ⭐

This is the realistic path for **Bluetooth** (your Nord Buds): a virtual sink
catches app audio, Veyra processes it, and renders to the Buds. No admin, no
test-signing.

1. **Install a virtual audio cable** (one-time): e.g. **VB-CABLE**
   (https://vb-audio.com/Cable/) → run its installer → reboot. It adds
   "CABLE Input" (playback) + "CABLE Output" (recording).
2. **Make the cable the default output:** Windows Sound settings → Output →
   **CABLE Input**. (Now Spotify/YouTube play into the cable — you'll hear
   nothing yet; that's expected.)
3. **Match formats:** in Sound → both **CABLE Input** and **Nord Buds** →
   Properties → Advanced → set the same format, e.g. **16-bit/24-bit, 48000 Hz**.
   (The bridge currently expects matching rates.)
4. **Start the service** (`veyra-service.exe --console`) and **the UI**.
5. In the UI → **Devices**:
   - **Source** = `CABLE Output` (what the apps feed),
   - **Output** = `OnePlus Nord Buds 3 Pro`,
   - toggle **Enable** on.
6. **Play music.** You should now hear it through the Buds. Move the **EQ /
   Bass Boost / Treble**, switch **Presets**, toggle **Spatial → Cinematic**
   (crossfeed + HRTF virtualisation), **Night Mode**, **Loudness Match** — each
   should audibly change the sound in real time.

> Latency: loopback adds ~30–80 ms — fine for music, slightly noticeable for
> video. The APO path (Stage C) is lower latency.

❓ If you hear nothing: confirm CABLE Input is the **default** output and music
is actually playing into it (its volume meter moves in Sound settings), the
service Terminal shows the bridge started, and Source/Output are set in Devices.

---

## 4. Stage C — APO in the audio path (lower latency; admin + test-signing)

Optional, lower-latency alternative to the bridge. ⚠️ Bluetooth endpoints often
reject custom APOs — Stage B is the reliable route for the Buds. Use Stage C on
a **wired/built-in** output. Full steps in [BUILD_GUIDE.md](BUILD_GUIDE.md) §2:
enable test-signing + reboot → `installer\driver\register-apo.ps1` (elevated) →
set the endpoint's `PKEY_FX_PostMixEffectClsid` to the Veyra CLSID → run the
service → play audio. Tell me your wired device and I'll give exact registry steps.

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
| 10-band EQ + curve | Home | Drag nodes → tooltip (Hz + dB); audio changes (Stage B) |
| Bass/Treble/Volume/Width/Compression knobs | Home | Live value text; audio changes |
| Presets (8 built-in + user) | Presets | Apply/save/import/export `.vpreset` |
| Per-app rules | `%APPDATA%\Veyra\app_rules.json` | Focus an app → its preset auto-applies (see below) |
| Spatial (crossfeed + virtualization) | Settings → Spatial | Cinematic = wide/out-of-head; Competitive = neutral |
| Night Mode | Settings → Loudness | Quiet parts lifted, peaks tamed |
| Loudness Match | Settings → Loudness | Levels normalize toward target LUFS |
| Sleep Timer | Settings → Loudness | System volume fades to silence over the tail |
| Mic chain | Settings → Microphone | (needs the capture path; config persists) |
| Theme / opacity / reduce-motion | Settings → Appearance | Live + persists across restart |

**Per-app rules quick test:** create `%APPDATA%\Veyra\app_rules.json`:
```json
[ { "match": "chrome", "preset_uuid": "v-bass-boost", "priority": 0, "enabled": true },
  { "match": "spotify", "preset_uuid": "v-cinematic", "priority": 5, "enabled": true } ]
```
Focus Chrome → EQ jumps to Bass Boost; focus Spotify → Cinematic. (No file = nothing changes.)

---

## 7. Where things live / troubleshooting

- Config: `%APPDATA%\Veyra\config.json` · Logs: `%APPDATA%\Veyra\logs\veyra-service.log`
- Presets: `%APPDATA%\Veyra\presets\*.vpreset` · Crashes: `%APPDATA%\Veyra\crashes\`
- App-rules: `%APPDATA%\Veyra\app_rules.json`
- **Build fails** → paste the first error from `cmake --build`.
- **LED stays amber** → the service isn't running / a stale `veyra-service.exe` is; restart Terminal 1.
- **No audio in Stage B** → see the ❓ note in §3.
- **Send back:** which ✅ passed, any ❓/❌ with the exact console error or
  `veyra-service.log` tail (or a screenshot for visual issues).
