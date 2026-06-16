# Veyra Sounds — Local Verification Runbook

A hands-on pass to actually **run** what CI only compiles. Do the tiers in
order: **Tier A proves the most with the least risk (no admin, no reboot).**
Only go to Tier B/C once A looks right.

> Build host: Windows 11. Everything below is verified against the current
> `main`. Report back what you see at each ✅/❓ and I'll fix from there.

---

## 0. One-time toolchain setup

Install **Visual Studio 2022 Build Tools** (free) with the
**"Desktop development with C++"** workload — this gives MSVC `cl`, the
Windows 11 SDK, **CMake**, and **Ninja** in one shot. Git you already have.

- Installer: https://visualstudio.microsoft.com/downloads/ → "Tools for Visual Studio" → "Build Tools for Visual Studio 2022".
- In the installer, tick **Desktop development with C++**. (That includes
  "C++ CMake tools for Windows", which brings CMake + Ninja.)

No WDK is needed to build — the APO is implemented against the base SDK.

---

## 1. Build

Open **"x64 Native Tools Command Prompt for VS 2022"** (Start menu → search it).
This puts `cl`, the SDK, CMake, and Ninja on `PATH` — the presets assume that.

```sh
cd C:\Users\renuk\Documents\veyra

git submodule update --init --recursive          REM JUCE + spdlog

cmake --preset windows-release -DVEYRA_BUILD_TESTS=ON
cmake --build --preset windows-release
```

First configure pulls/builds JUCE, so it takes a few minutes. When it finishes,
all four binaries are in **`build\windows-release\bin\`**:

```
veyra.exe            (the UI)
veyra-service.exe    (the background service)
veyra-apo.dll        (the audio effect)
veyra-overlay.exe    (the Gamer Mode HUD)
```

Optional — run the unit tests locally (same ones CI runs):

```sh
ctest --test-dir build\windows-release --output-on-failure
```

---

## 2. Tier A — UI + service + IPC  (no admin, START HERE)

This verifies the JUCE rendering, theming, the named-pipe handshake, and the
full config round-trip — **without** touching the system audio path. It's the
highest-value check and completely safe.

**Terminal 1** (the same dev prompt is fine — no admin):

```sh
build\windows-release\bin\veyra-service.exe --console
```

You should see `Veyra Audio Service vX.Y.Z (console mode)` and `Running.`
It creates the config at `%APPDATA%\Veyra\config.json`, the shared-memory
parameter blocks, and the control pipe.

**Terminal 2:**

```sh
build\windows-release\bin\veyra.exe
```

Check, in order:

- ✅ Borderless **glass window** opens (ambient blobs, frosted cards) — not flat/black.
- ✅ The brand LED next to the "V" logo in the top bar turns **green** (= connected to the service). Amber means it can't reach the service — check Terminal 1 is still running.
- ✅ Drag the **EQ band sliders** and **knobs**, toggle **master**, move **master volume** → Terminal 1 logs config being applied/saved (watch `%APPDATA%\Veyra\logs\veyra-service.log`).
- ✅ **Settings → Appearance**: click theme tiles → the whole UI recolors live. Move opacity, toggle reduce-motion (visualizer should freeze).
- ✅ **Sidebar → Mini Mode** → compact always-on-top bar appears; its expand button returns to full.
- ✅ **System tray** icon + its menu (if present in the build).
- ❓ Anything that looks broken/ugly/misaligned — screenshot it.

Close the UI; `Ctrl+C` in Terminal 1 to stop the service.

> What this does **not** test: audio actually passing through the DSP. That's
> Tier B.

---

## 3. Tier B — APO in the real audio path  (admin + test-signing + reboot)

Only do this after Tier A. It loads `veyra-apo.dll` into the protected
`audiodg.exe`, which requires test-signing. Full mechanics are in
[BUILD_GUIDE.md](BUILD_GUIDE.md) §2; the short version:

1. **Enable test-signing** (elevated PowerShell), then **reboot**:
   ```powershell
   bcdedit /set testsigning on
   ```
2. **Register the COM server** (elevated PowerShell):
   ```powershell
   cd C:\Users\renuk\Documents\veyra\installer\driver
   .\register-apo.ps1 -DllPath ..\..\build\windows-release\bin\veyra-apo.dll
   ```
3. **Associate it with your output device** — the device-specific step: set the
   endpoint's `PKEY_FX_PostMixEffectClsid` to
   `{7E9C2B14-3F6A-4D8E-9B21-5C0A1F2E3D44}`, then disable/enable the device so
   `audiodg.exe` reloads. (This is the fiddly part; tell me your output device
   and I'll give exact registry steps.)
4. **Run the service** (`veyra-service.exe --console`) so the parameter block
   exists, play audio, and confirm the EQ/knobs/master in the UI change what
   you hear in real time.

> ⚠️ This is the riskiest tier (protected-process DLL load, registry edits).
> Turn test-signing back off when done: `bcdedit /set testsigning off` + reboot.

---

## 4. Tier C — Gamer Mode overlay

```sh
build\windows-release\bin\veyra-overlay.exe
```

- In the UI, enable **Gamer Mode** (Settings/sidebar) → the overlay reads the
  config and shows the **radar frame** (the 3 styles: competitive / rich /
  compass) centered near the top of the screen.
- ❓ Live **blips** require the service's loopback Sound Tracker to be writing
  detections — that producer thread is the one remaining runtime piece (not yet
  built), so expect an empty radar for now. The frame + styles + transparency
  are what to eyeball here.

---

## What to send back (for the "you run it, I fix it" loop)

- Which ✅ passed and which ❓/❌ failed.
- For failures: the exact error text from the dev prompt, or the tail of
  `%APPDATA%\Veyra\logs\veyra-service.log`, or a screenshot for visual issues.
- If the **build** fails: paste the first error from `cmake --build`.

I'll fix forward from whatever you report.
