# VEYRA SOUNDS — BACKEND ENGINEERING PROMPT (FOR CLAUDE)

> Paste this entire document to Claude as a single prompt. Claude should generate the full project scaffold, all source modules, build files, installer config, and documentation, working through the build phases in order. The result is a buildable Windows 64-bit C++ codebase under GPLv3, free and open source.

---

## 0. ROLE & GOAL

You are a senior C++ Windows audio engineer with 15+ years of experience shipping real-time DSP, APO/AVStream drivers, and JUCE applications. You're building **Veyra Sounds**, a free open-source system-wide audio enhancer for Windows 10/11 (64-bit). It must outperform FxSound, Boom 3D, Equalizer APO, Voicemeeter, and Nahimic — combined — while being faster, lighter, and prettier than all of them.

**Performance targets (non-negotiable):**
- Idle CPU < 0.3% on a modern 4-core CPU
- RAM < 50 MB resident
- Round-trip audio latency < 5 ms (target 1–3 ms, ultra-low mode < 2 ms)
- Installer < 15 MB (target 5–10 MB)
- Cold start < 300 ms (target < 150 ms)

**Quality bar:** every DSP block runs in 32-float internally, sample-accurate, with smooth parameter interpolation (no zipper noise). Zero clicks/pops on parameter changes. Zero buffer dropouts. Zero memory allocations in the real-time audio thread.

**License:** GPLv3. Pure GPL is fine (JUCE Personal tier is free under GPLv3). All bundled libraries must be GPL-compatible.

---

## 1. TECHNOLOGY STACK

- **Language:** C++20.
- **Build:** CMake 3.25+, Ninja generator.
- **UI:** JUCE 7+ (Personal/GPL tier — free).
- **DSP:** JUCE DSP + custom modules. Use `juce::dsp::FFT`, `juce::dsp::Convolution`, `juce::dsp::IIR` as foundations. Custom DSP for EQ, compression, surround.
- **Mic noise suppression:** RNNoise (BSD — bundle as static lib).
- **Headphone HRTF:** MIT KEMAR HRTF dataset (public domain) bundled as resource. Custom HRTF convolution using JUCE's `juce::dsp::Convolution` with partitioned overlap-add.
- **APO COM component:** native Win32 + Windows Audio Processing Object SDK (no JUCE — pure COM/C++).
- **Service:** Windows Service API (native).
- **IPC:** Named pipes (Win32) with a versioned binary protocol (FlatBuffers or custom — pick FlatBuffers for forward compatibility, but it must be small).
- **Installer:** Windows MSIX packaging tools + a portable ZIP fallback.
- **Updater:** Squirrel.Windows or a custom implementation calling GitHub Releases API.
- **Crash reporting:** Google Breakpad or a lightweight custom implementation (write minidumps to %APPDATA%\\Veyra\\crashes).
- **Logging:** spdlog (MIT, GPL-compatible).
- **Localization:** JUCE's TRANS() macro + .lang files per supported language (EN default; ZH-CN, ES, AR, HI, TA shipped).
- **Game detection / overlay rendering:** Win32 only — no DirectX or graphics-API hooks. Ban-safe.

---

## 2. ARCHITECTURE

Veyra runs as **three cooperating processes** plus an **APO COM DLL** loaded into the Windows audio stack.

```
┌─────────────────────────────────────────────────────────────┐
│  Windows Audio Engine (audiodg.exe)                         │
│  └─ Veyra APO (veyra_apo.dll)        ←─ MFX / EFX APO       │
│       │  Real-time DSP. Lives inside audiodg.                │
│       │  Reads parameters via shared memory from service.    │
│       │  Never blocks. Never allocates.                      │
└────────────────┬────────────────────────────────────────────┘
                 │ shared memory (parameters) + ring buffer (analyzer/tracker)
                 ▼
┌─────────────────────────────────────────────────────────────┐
│  Veyra Audio Service (veyra-service.exe)                    │
│  └─ Windows Service, runs as SYSTEM, autostarts              │
│       - Mediates between UI and APO                          │
│       - Holds canonical config + preset state                │
│       - Performs Sound Tracker DSP analysis on capture       │
│         from a loopback session of the system mixer          │
│       - Game detection (process enumeration polling)         │
│       - Updater                                              │
│       - Crash log collector                                  │
└────────┬───────────────────────────────────┬────────────────┘
         │ named pipe \\.\pipe\veyra-control  │ named pipe \\.\pipe\veyra-tracker
         ▼                                   ▼
┌────────────────────────┐         ┌────────────────────────┐
│  Veyra UI App          │         │  Veyra Overlay         │
│  (veyra.exe — JUCE)    │         │  (veyra-overlay.exe)   │
│  - Main UI, presets,   │         │  - Window-overlay-only │
│    settings, vis,      │         │    Sound Tracker HUD   │
│    mini-mode           │         │  - WS_EX_TRANSPARENT + │
│  - Sends config to     │         │    WS_EX_LAYERED +     │
│    service             │         │    WS_EX_TOPMOST       │
│  - Visualizer reads    │         │  - Draws with Direct2D │
│    ring buffer from    │         │    on a per-pixel-alpha│
│    APO                 │         │    layered window      │
└────────────────────────┘         └────────────────────────┘
```

### Why this split

- **APO** lives inside `audiodg.exe`, the Windows audio engine — the *only* way to do true system-wide low-latency processing. It MUST be lean, non-blocking, allocation-free in real-time.
- **Service** is the orchestrator. It can run as SYSTEM, do filesystem I/O, network calls, process enumeration. The UI never touches the APO directly — the service mediates.
- **UI** is a normal user-mode JUCE app. Crashes don't take down audio.
- **Overlay** is its own process — when a game crashes the overlay (rare but possible with broken GPU drivers), it won't take down the UI or audio. It uses **layered-window overlay only**, never D3D hooks, so it's anti-cheat safe.

### Anti-cheat strategy

Per G1: default to **layered-window overlay** (Win32 `WS_EX_LAYERED + WS_EX_TRANSPARENT + WS_EX_TOPMOST`). This is just a transparent topmost window — it does NOT hook any graphics API and is not detected as a cheat by Vanguard, EAC, BattlEye, or Faceit AC.

Per G7: when the service detects a competitive title via process name (using the auto-updating blocklist from G8), it shows a warning to the user and lets the user decide whether to enable the optional D3D-hook mode (which exists for non-anti-cheat games where the layered window doesn't render in exclusive fullscreen). The blocklist JSON is fetched from a signed source at app launch and cached.

---

## 3. REPOSITORY STRUCTURE

```
veyra/
├── CMakeLists.txt                   (top-level, defines all targets)
├── LICENSE                          (GPLv3)
├── README.md
├── CONTRIBUTING.md
├── .github/
│   └── workflows/
│       ├── build.yml                (CI: build, sign, package)
│       └── release.yml              (tag → MSIX + portable + GitHub Release)
├── third_party/
│   ├── JUCE/                        (git submodule, GPL tier)
│   ├── rnnoise/                     (git submodule, BSD)
│   ├── spdlog/                      (git submodule, MIT)
│   ├── flatbuffers/                 (git submodule, Apache 2.0)
│   ├── kissfft/                     (git submodule, BSD)  // optional, JUCE FFT works too
│   └── hrtf/
│       ├── mit_kemar/               (HRTF dataset, public domain)
│       └── ReadMe.md
├── resources/
│   ├── icons/                       (Phosphor SVGs)
│   ├── themes/                      (11 .json theme tokens)
│   ├── presets/                     (built-in .vpreset files)
│   ├── lang/                        (EN, ZH-CN, ES, AR, HI, TA)
│   └── audio_test/                  (test tones, pink/white noise WAVs)
├── ipc/
│   └── schema/
│       ├── control.fbs              (UI ↔ service messages)
│       └── tracker.fbs              (service → overlay messages)
├── modules/
│   ├── veyra-common/                (shared utilities, config, IPC, logging)
│   │   ├── include/
│   │   └── src/
│   ├── veyra-dsp/                   (shared DSP library — header-only where possible)
│   │   ├── include/
│   │   │   ├── eq/                  (graphic + parametric)
│   │   │   ├── dynamics/            (compressor, limiter, gate, AGC)
│   │   │   ├── enhancers/           (bass, treble, stereo widener, reverb, echo)
│   │   │   ├── spatial/             (HRTF, virtual surround, headset mode)
│   │   │   ├── voice/               (rnnoise wrapper, AEC, de-esser, voice EQ)
│   │   │   ├── pitch_time/          (pitch shift, time stretch via PSOLA/phase vocoder)
│   │   │   ├── loudness/            (EBU R128, true-peak limiter)
│   │   │   ├── analyzer/            (FFT spectrum, VU, clipping detection)
│   │   │   ├── tracker/             (sound feature classifier, direction estimator)
│   │   │   └── chain/               (DSP graph, parameter smoothing, automation)
│   │   └── src/
│   └── veyra-tracker-classifier/    (sound category classifier — DSP-only, no ML)
├── apps/
│   ├── veyra-apo/                   (APO COM DLL)
│   │   ├── CMakeLists.txt
│   │   ├── dllmain.cpp
│   │   ├── VeyraAPO.h
│   │   ├── VeyraAPO.cpp             (implements IAudioProcessingObject)
│   │   ├── VeyraAPOMFX.cpp          (mode effects)
│   │   ├── VeyraAPOEFX.cpp          (endpoint effects)
│   │   ├── SharedParameters.h       (shared memory layout — must be cache-line aligned)
│   │   └── VeyraAPO.rgs             (COM registration)
│   ├── veyra-service/               (Windows service)
│   │   ├── CMakeLists.txt
│   │   ├── main.cpp
│   │   ├── ServiceMain.cpp
│   │   ├── ConfigManager.cpp
│   │   ├── PresetManager.cpp
│   │   ├── PerAppManager.cpp
│   │   ├── PerDeviceManager.cpp
│   │   ├── GameDetector.cpp
│   │   ├── BlocklistFetcher.cpp
│   │   ├── TrackerEngine.cpp        (runs the DSP classifier + direction estimator)
│   │   ├── LoopbackCapture.cpp      (WASAPI loopback for analyzer + tracker)
│   │   ├── UpdaterClient.cpp
│   │   ├── CrashReporter.cpp
│   │   └── ControlServer.cpp        (named-pipe server)
│   ├── veyra-ui/                    (JUCE app)
│   │   ├── CMakeLists.txt
│   │   ├── Source/
│   │   │   ├── Main.cpp
│   │   │   ├── MainComponent.cpp
│   │   │   ├── Theme/
│   │   │   ├── Components/          (glass card, knob, slider, etc.)
│   │   │   ├── Screens/
│   │   │   │   ├── HomeScreen.cpp
│   │   │   │   ├── PresetsScreen.cpp
│   │   │   │   ├── AppsScreen.cpp
│   │   │   │   ├── DevicesScreen.cpp
│   │   │   │   ├── SoundLabScreen.cpp
│   │   │   │   ├── GamerModeScreen.cpp
│   │   │   │   └── SettingsScreen.cpp
│   │   │   ├── Visualizers/         (8 modes: bars, monstercat, circular, ferrofluid, waveform, particle, wavy, 3d bars)
│   │   │   ├── MiniMode.cpp
│   │   │   ├── TrayIcon.cpp
│   │   │   ├── Onboarding/
│   │   │   ├── ServiceClient.cpp    (named-pipe client to service)
│   │   │   └── HotkeyManager.cpp    (Win32 RegisterHotKey)
│   │   └── Resources/
│   └── veyra-overlay/               (Sound Tracker HUD process)
│       ├── CMakeLists.txt
│       ├── main.cpp
│       ├── OverlayWindow.cpp        (WS_EX_LAYERED + Direct2D)
│       ├── Radar.cpp                (renders 3 radar styles)
│       ├── TrackerClient.cpp        (named-pipe client to service)
│       └── ProcessWatcher.cpp       (detects game focus + anti-cheat)
├── installer/
│   ├── msix/
│   │   ├── AppxManifest.xml
│   │   ├── Veyra.appinstaller
│   │   └── makemsix.ps1
│   ├── portable/
│   │   └── package.ps1              (ZIP creation)
│   └── driver/                      (signed audio driver for APO registration)
│       ├── veyra_apo.inf
│       └── signing.md
└── tests/
    ├── dsp_tests/                   (Catch2)
    ├── ipc_tests/
    ├── latency_benchmark/
    └── audio_fixtures/              (WAV files for golden-output tests)
```

---

## 4. THE DSP CHAIN

### System Audio Processing Chain (inside APO)

Order is fixed for predictable mixing behavior. All blocks bypass-able. All parameters interpolated over a 32-sample ramp on change.

```
Input (stereo float32 from Windows mixer)
  │
  ├── Loudness Normalization        [optional — EBU R128 short-term gain]
  │
  ├── Mono / Stereo / Balance       [routes channels]
  │
  ├── Graphic EQ (10-band IIR)      [biquad cascade per channel]
  │   OR Parametric EQ (16-band)    [bell/shelf/HP/LP per band]
  │
  ├── Bass Boost                    [low-shelf at 80 Hz, 0–12 dB]
  │
  ├── Treble Boost                  [high-shelf at 8 kHz, 0–12 dB]
  │
  ├── Dynamic Compressor            [feed-forward, soft-knee, 0–100%]
  │
  ├── Stereo Widener                [Mid-Side processing, 0–200%]
  │
  ├── Reverb                        [JUCE Reverb, 0–100% wet]
  │
  ├── Echo / Delay                  [JUCE DelayLine, optional]
  │
  ├── Virtual Surround / HRTF       [optional — partitioned convolution, Cinematic or Competitive HRTF]
  │
  ├── Volume Gain                   [0–300% — past the system 100%]
  │
  ├── True-Peak Limiter             [oversampled, prevents inter-sample clipping]
  │
  └── Output to Windows audio engine
```

### Microphone Processing Chain (inside APO for input endpoints)

Per G13 — fixed optimized chain, not user-reorderable:

```
Input (mono float32 from mic)
  │
  ├── RNNoise Suppression           [neural noise suppression]
  │
  ├── Noise Gate                    [threshold + hysteresis]
  │
  ├── AEC (Acoustic Echo Cancel)    [speech-band default; full-band optional]
  │
  ├── Voice EQ                      [profile-driven: Gaming/Conference/Streaming/Podcast/Whisper]
  │
  ├── Voice Enhancer                [presence boost, warmth, intelligibility]
  │
  ├── De-esser                      [sibilance tame, 5–8 kHz]
  │
  ├── Voice Stabilizer (AGC)        [target -16 LUFS]
  │
  ├── Side-Tone Split               [routes a copy to output for monitoring]
  │
  └── Output to Windows audio engine
```

### Parameter smoothing (mandatory)

Every continuously-variable parameter (EQ gain, knob value, volume) uses **exponential smoothing** with a 5ms time constant in real-time. Discrete switches (bypass, on/off) use a **5ms cross-fade**. Result: zero audible artifacts on any parameter change.

---

## 5. THE APO — IMPLEMENTATION DETAILS

Veyra's APO is an **EFX (endpoint effect)** APO for output, and a separate **MFX (mode effect)** APO for input. It implements `IAudioProcessingObject`, `IAudioProcessingObjectConfiguration`, and `IAudioProcessingObjectRT`.

### Real-time rules (absolute)

The APO's `APOProcess()` method runs in audiodg.exe's real-time thread. Inside it:
- **No** memory allocation (no `new`, no `malloc`, no STL containers that allocate).
- **No** locks (no mutex, no critical section). Use atomics for parameter snapshots and lock-free SPSC ring buffers for analyzer output.
- **No** filesystem, network, or COM calls.
- **No** logging (defer to a lock-free queue read by a separate thread).
- Use pre-allocated buffers sized at registration time.
- Use SIMD (SSE2 minimum, AVX2 preferred — runtime detect, fall back gracefully).

### Parameter snapshot pattern

Service writes parameters into a memory-mapped file (`Local\\VeyraAPOParameters_v1`). The APO double-buffers them: at the top of each `APOProcess` it `memcpy`s the current published snapshot (under a 64-byte cache-aligned atomic generation counter using sequence-lock) into its local copy. If the service is mid-write, the APO simply uses the previous snapshot.

```cpp
struct alignas(64) VeyraParameters {
    std::atomic<uint64_t> generation;  // sequence lock
    // ---- payload ----
    float masterGainDb;
    float bands[10];                   // EQ gains in dB
    float bassBoostDb;
    float trebleDb;
    float compressionAmount;           // 0..1
    float stereoWidth;                 // 0..2
    float reverbWet;                   // 0..1
    float volumeGain;                  // 0..3 (300%)
    float balance;                     // -1..1
    uint8_t monoMode;
    uint8_t bypassMaster;
    uint8_t hrtfMode;                  // 0=off, 1=Cinematic, 2=Competitive
    uint8_t parametricMode;            // 0=graphic, 1=parametric
    // ... parametric band data
    // ... mic chain data
    uint8_t _padding[remaining_cache_line];
};
```

### Analyzer ring buffer

The APO writes downsampled samples (or post-FFT bin magnitudes) into a lock-free SPSC ring buffer in shared memory (`Local\\VeyraAnalyzer_v1`). The UI reads at its leisure for visualizer + spectrum + VU + clipping detection.

### COM registration

The APO must register via Windows audio driver INF + signed catalog (`.cat`). For the open-source distribution you'll need either:
- A test-mode workflow for developers (`bcdedit /set testsigning on`)
- A donated EV code-signing certificate for production
- Submission to the Microsoft Hardware Dev Center for WHQL signing (free for open source)

Document both paths in `installer/driver/signing.md`.

---

## 6. THE SERVICE

### Responsibilities
1. Bootstrap the APO parameter shared memory.
2. Listen on `\\.\pipe\veyra-control` for the UI.
3. Hold canonical state: presets, per-app rules, per-device profiles, settings.
4. Periodically (every 200ms) enumerate processes, detect foreground app + audio sessions, and apply per-app rules.
5. Game detection — match against the blocklist (anti-cheat games) and gamer auto-switch list.
6. Loopback capture (WASAPI) on the system mixer for the Sound Tracker analysis (only when Sound Tracker is enabled).
7. Run the **Sound Tracker DSP classifier + direction estimator**, push results to `\\.\pipe\veyra-tracker` for the overlay.
8. Update check (Squirrel-style) — daily background poll of GitHub Releases.
9. Crash log aggregation — collects minidumps from UI/overlay/APO.

### Per-app detection logic

Combine 3 safe signals (per G9):

```
For each app rule R:
    if (R.foregroundWindow matches focused process) and (R.detectionMode in {ForegroundOnly, Both}):
        candidate = R
    if (R.audioSessionActive exists) and (R.detectionMode in {AudioSession, Both}):
        candidate = candidate or R
Priority resolution:
    1. ForegroundOnly matches beat AudioSession-only matches (the user is actively using this app)
    2. If multiple matches, most-recently-active wins
    3. If no matches, revert to global default preset
```

Switching must be **rate-limited** to once per 800ms to prevent flicker during alt-tabs.

### Game detection logic

```
1. Read bundled blocklist (anti-cheat games) + user-added Gamer auto-list.
2. Poll process list every 1000ms.
3. If GPU usage >40% sustained for 3s AND foreground process matches a gamer-list entry:
       → enable Gamer Mode, apply per-game profile, send overlay start signal.
4. If foreground matches anti-cheat blocklist:
       → show one-time user warning, force overlay to layered-window-only mode.
5. On game exit (process gone or focus lost for >5s):
       → optionally revert Gamer Mode (user-configurable).
```

GPU usage check uses `D3DKMTQueryStatistics` (no API hooks needed).

### Sound Tracker engine

Lives in the service for security and performance reasons. Pipeline:

```
WASAPI loopback (system mixer, post-APO if you want effected audio, pre-APO for raw)
    ↓
20ms hop, 50% overlap windowed buffer (Hann window)
    ↓
FFT (4096-point at 48kHz → 11.7ms latency)
    ↓
Feature extraction per buffer:
    - Spectral centroid
    - Spectral rolloff
    - Onset/transient detection (energy-based + spectral flux)
    - Sub-band energies (sub-bass, bass, low-mid, mid, high-mid, presence, brilliance)
    - Stereo correlation (mono-ness)
    - Interaural Level Difference (ILD) per band
    - Interaural Time Difference (ITD) via GCC-PHAT
    ↓
Direction estimation:
    angle = arctan2(ILD-derived left/right balance, weighted ITD)
    distance proxy = 1 / (1 + spectral_centroid_drop)  // farther = duller
    ↓
Category classification (DSP-only, no ML):
    Footstep:  sharp transient, mid-band dominant, short duration, repeated
    Gunshot:   very sharp transient, broad band, high peak, brief
    Explosion: long transient, low-band heavy, long decay
    Voice:     stable formants 100Hz–4kHz, high mono correlation
    Vehicle:   sustained low-band with harmonics, slow variation
    Environment: low-energy diffuse, low transient activity
    ↓
Confidence threshold + smoothing (3-frame median filter)
    ↓
Push {angle, distance, category, confidence} to overlay via named pipe at 60Hz
```

For G4 per-category accuracy ~80%+, this DSP-only approach hits the bar. Document where a future small neural classifier could improve.

---

## 7. THE UI APPLICATION (JUCE)

### Window setup
- Default size **1180 × 760**, resizable, min **960 × 620**.
- Custom titlebar (per design brief — glass top bar with master controls).
- Use `juce::ComponentPeer::windowHasTitleBar = false`, draw our own.
- HiDPI aware. Scale all spacing through a `juce::Desktop::getInstance().getDisplays().getMainDisplay().scale` multiplier.

### Theme system
- Themes defined as JSON files in `resources/themes/`. Loaded at runtime.
- Custom theme builder writes to `%APPDATA%\\Veyra\\themes\\custom_*.json`.
- Theme swap is a 400ms cross-fade — accomplished by rendering two overlaid themed views and animating opacity.

### Glass rendering
JUCE doesn't have native backdrop-filter. Implement via:
1. Capture the area behind each glass card by rendering the desktop snapshot + Veyra background to an internal `juce::Image`.
2. Apply gaussian blur (`juce::ImageEffects::applyStackBlur`, radius 24).
3. Apply 140% saturation via HSL mapping.
4. Composite over with the glass tint color.
5. Draw 1px inner stroke + 8px outer shadow.

Cache aggressively — re-blur only when the background changes (e.g., wallpaper change, or every 30s for the desktop capture refresh per the design brief).

### Visualizers (8 modes)
All visualizers read from the APO analyzer ring buffer. Use **JUCE OpenGL renderer** for performance. Target the display refresh rate per G10 — use `juce::OpenGLContext::setContinuousRepainting(true)` and disable internal frame limiting.

Implementation guidance:
- **Bars (classic):** 64 bins, log-frequency mapping, vertical bars.
- **Monstercat:** 64 bins, log-mapping, gravity-decay smoothing (decay envelope = `current = max(input, current * 0.95)`).
- **Circular:** Monstercat layout but arranged radially around center.
- **Ferrofluid:** metaball rendering (signed distance fields via fragment shader) modulated by sub-band energies. Most expensive — provide quality preset (LOW: 16 metaballs, HIGH: 64).
- **Waveform:** raw waveform from analyzer with persistence (alpha-fade trail).
- **Particle:** 500–2000 particles, position influenced by frequency-band gravity wells.
- **Wavy:** stacked sine layers with frequency-band amplitude modulation.
- **3D Bars:** Three-D extruded bar grid with parallax camera rotation, rendered via OpenGL.

All visualizers expose:
- Color (default = theme accent; overridable per visualizer)
- Sensitivity (gain multiplier on input)
- Smoothing (decay/attack times)
- FPS cap (Auto = match display; Manual = 30/60/120/240)

### Visualizer fullscreen mode
Separate `juce::TopLevelWindow` with no border, true black background, the active visualizer at full size. Overlay UI auto-hides per design brief.

### IPC client
`ServiceClient.cpp` uses a single named-pipe connection to `\\.\pipe\veyra-control`. Sends FlatBuffer-encoded messages:

```
SetParameter(id, value)
GetCurrentState()
LoadPreset(uuid)
SavePreset(preset)
EnableGamerMode(bool)
SetMasterBypass(bool)
SubscribeAnalyzer()  → server pushes ring buffer events at 60Hz
SubscribeTracker()
RegisterHotkey(action, keycode)
```

Auto-reconnect on disconnect (service restart) with exponential backoff up to 30s.

### Hotkeys
Use Win32 `RegisterHotKey()` for global hotkeys. Default bindings per the design brief. User can rebind in Settings → Hotkeys.

### Tray icon
Use Win32 `Shell_NotifyIcon` (JUCE has `juce::SystemTrayIconComponent` — use that wrapper). Custom popup menu — NOT the system menu — drawn with JUCE so it matches the app's glass aesthetic (a small frameless TopLevelWindow positioned near the cursor).

---

## 8. THE OVERLAY APP (Sound Tracker HUD)

Tiny separate process. Single executable, < 1MB.

### Window setup
```cpp
HWND hwnd = CreateWindowExW(
    WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE,
    L"VeyraOverlayClass", L"VeyraOverlay",
    WS_POPUP, x, y, w, h, nullptr, nullptr, hInstance, nullptr);

// Per-pixel alpha for proper transparency
SetLayeredWindowAttributes(hwnd, 0, 255, LWA_ALPHA);
```

Draw with **Direct2D** for hardware-accelerated transparent rendering at high refresh rates. No D3D hooking — Direct2D draws to our own layered window, completely safe.

### Hold-to-interact
Default click-through (`WS_EX_TRANSPARENT`). When user holds the configured modifier (default Alt), temporarily remove `WS_EX_TRANSPARENT` so the user can drag the HUD. Restore on key release.

### IPC
Reads tracker updates from `\\.\pipe\veyra-tracker` at 60Hz. Renders at display refresh rate (`DwmFlush()` synced).

### Rendering modes (per G3)
1. **Circular** — top-down radar with concentric distance rings, dots = detected sounds, color per category.
2. **Half-arc** — same data but rendered as a 180° arc in front of the user.
3. **Edge indicators** — arrows on the screen edges nearest each sound source.

### Rich vs Competitive modes (per G6)
- **Competitive (default for esports):** only dots + direction + minimal radar outline. No category icons. No text. ~3% screen estate.
- **Rich:** category icons, color-coded, distance rings, optional sound labels.

### Per-game memory (per G5)
Position + size + opacity stored per detected game in the service's per-game profile and pushed to the overlay on game launch.

---

## 9. AUDIO TESTING TOOLS (SOUND LAB)

Implemented in the UI app, talking to the APO for routing. Each test bypasses the user's EQ for accurate calibration.

- **Speaker Test:** sequence pink noise bursts through each channel.
- **7.1 Surround Test:** route a moving virtual source through HRTF/surround stack; user confirms direction.
- **Mic Test:** loopback the mic input through a "monitor only" channel; show real-time waveform + spectrum.
- **Frequency Sweep:** linear sweep from 20Hz–20kHz over user-set duration.
- **Hearing Range:** binary search guided by user — start 10kHz, push up.
- **Polarity / Phase:** alternate in-phase vs inverted between channels.
- **Noise Generator:** white (gaussian), pink (1/f), brown (1/f²), binaural beats (configurable delta frequency).

All test signal generation runs in the UI app (real-time thread inside JUCE's `AudioProcessorPlayer`) and is routed through the system mixer like normal app audio (so it can be heard through Veyra's processing if desired — or with bypass for calibration mode).

---

## 10. PRESET FILE FORMAT

`.vpreset` — JSON-based (human-readable, easy to share/version-control).

```json
{
  "version": 1,
  "uuid": "01H...",
  "name": "FPS Footstep Boost",
  "category": "Gaming",
  "author": "Veyra",
  "description": "Mid-band lift around 2-4 kHz to make footsteps pop.",
  "tags": ["fps", "competitive"],
  "created": "2026-06-13T...",
  "modified": "...",
  "dsp": {
    "graphic_eq": {
      "bands": [0, 0, -2, 0, 2, 5, 4, 2, 1, 0]
    },
    "parametric_eq": null,
    "bass_boost_db": 1.5,
    "treble_db": 0,
    "compression": 0.2,
    "stereo_width": 1.0,
    "reverb": 0,
    "volume_gain": 1.0,
    "balance": 0,
    "mono_mode": false,
    "hrtf_mode": "Competitive",
    "virtual_surround": true,
    "headset_intensity": 0.4,
    "loudness_normalize": false
  },
  "mic_chain": {
    "profile": "Gaming Chat",
    "rnnoise": true,
    "gate_threshold_db": -45,
    "aec_mode": "speech",
    "side_tone": false
  }
}
```

Built-in presets shipped in `resources/presets/`. User presets in `%APPDATA%\\Veyra\\presets\\`.

---

## 11. CONFIGURATION & STATE

`%APPDATA%\\Veyra\\config.json` — canonical state managed by service.

```json
{
  "version": 1,
  "master_enabled": true,
  "master_volume_gain": 1.0,
  "active_preset_uuid": "...",
  "per_device_settings": [ { "device_id": "...", "preset_uuid": "...", "volume_gain": 1.1 } ],
  "per_app_rules": [ { "exe": "Spotify.exe", "preset_uuid": "...", "detection": "Foreground", "auto_mute_on_unfocus": false } ],
  "per_game_profiles": [ ... ],
  "gamer_mode": { "enabled": false, "auto_enable": true, "sound_tracker": { ... } },
  "appearance": { "theme": "midnight", "background_mode": "blurred_desktop", "window_opacity": 1.0, "show_grain": true },
  "audio_engine": { "sample_rate": 48000, "bit_depth": 24, "buffer_size": 128, "latency_mode": "Standard", "dsp_precision": "float32" },
  "hotkeys": { ... },
  "telemetry_opt_in": false,
  "updates": { "channel": "stable", "auto": true },
  "language": "en"
}
```

Atomic writes (write to temp, rename) to prevent corruption on crash.

---

## 12. BUILD & DISTRIBUTION

### CMake top-level
- Define a top-level `VEYRA_VERSION` variable.
- Produce 4 binaries: `veyra-apo.dll`, `veyra-service.exe`, `veyra.exe`, `veyra-overlay.exe`.
- Embed manifest for `veyra-service.exe` to request SYSTEM-level startup.
- Embed manifest for `veyra.exe` to request normal user (NOT elevated).
- HiDPI manifest entries.

### MSIX packaging
Use `MakeAppx.exe` (Windows SDK). `AppxManifest.xml` declares:
- Veyra Sounds as the app
- Custom protocol `veyra://` for deep links (e.g., `veyra://preset/import?file=...`)
- Tray icon
- Startup-on-login capability
- Audio device APO registration capability (requires elevated install)

### Portable build
A ZIP containing all 4 binaries + resources + a `veyra-install-driver.bat` for users who want APO mode. Without driver, Veyra falls back to WASAPI loopback (user-mode, no admin, but higher latency ~15ms).

### Code signing
Document the signing flow in `installer/driver/signing.md`:
- For free open-source: apply for an Azure Trusted Signing free tier OR get a free EV cert via SignPath.io's open-source program.
- WHQL submission for the audio driver is free for verified Microsoft Hardware Dev Center accounts.

### Squirrel-style updater
`UpdaterClient.cpp` runs in the service:
1. On startup + every 24h: GET `https://api.github.com/repos/<org>/veyra/releases/latest`.
2. If `tag_name > current_version`: download MSIX → verify signature → apply silently on next service restart (or immediately, with grace period for running UI).
3. Major version updates (X.0.0): prompt user via UI before installing.

---

## 13. PERFORMANCE BUDGET (REJECT PRs THAT VIOLATE)

| Metric | Budget |
|---|---|
| APO `APOProcess` per 10ms buffer | < 0.5ms CPU on Ryzen 5 5600 |
| Service idle CPU | < 0.1% |
| UI idle CPU (window hidden) | 0% (suspend rendering) |
| UI active CPU (visualizer running) | < 2% |
| Overlay CPU when active | < 0.5% |
| RAM combined | < 50 MB |
| Round-trip latency (Standard mode) | < 5 ms |
| Round-trip latency (Ultra-low mode, 32-sample buffer) | < 2 ms |
| Cold start (UI to window visible) | < 300 ms |

Include `tests/latency_benchmark/` that runs a known signal through and measures RTT with sample-accurate logging. Fail CI if regressed >10%.

---

## 14. INTERNATIONALIZATION

Ship with 6 languages: EN (default), ZH-CN, ES, AR, HI, TA.

Use JUCE's `TRANS()` macro. Translations in `resources/lang/<locale>.lang`. Include translator credits + a `CONTRIBUTING_TRANSLATIONS.md` for community contributions.

**Critical for AR:** verify RTL mirroring in JUCE (`juce::Component::setComponentEffect` plus manual layout flipping via `juce::Justification::right`). All sliders flip orientation. Test on a real AR locale.

---

## 15. CRASH REPORTING (LOCAL-FIRST PER X8)

`CrashReporter.cpp` in the service:
- Registers `SetUnhandledExceptionFilter` for each process.
- On crash: writes minidump (`.dmp`) + stack trace (`.txt`) + recent log lines to `%APPDATA%\\Veyra\\crashes\\<timestamp>\\`.
- On next launch, UI checks the folder. If crashes exist: shows non-intrusive banner ("Veyra recovered from a crash. View report.").
- User can manually send a crash via "Send to developers" button — opens a confirm dialog showing the exact data being sent, then POSTs to a GitHub Issues-style endpoint or saves to email.
- NEVER auto-sends anything. NEVER includes audio data.

---

## 16. AI SCENE-AWARE ADAPTIVE EQ (THE SECRET SAUCE FROM X1)

**Phase 2 feature, but plan the hook now.** A future module `veyra-dsp/scene_detector/` will:
1. Read short-term spectral statistics from the APO analyzer.
2. Classify the current content into categories (Music · Movie/Dialogue · Game · Voice Call · Silence) using a tiny on-device classifier (RandomForest or 4-layer MLP, < 200KB).
3. Optionally adjust the active preset over a long crossfade (e.g., 8 seconds) — never abrupt.
4. Allow user override with a "Lock preset" button.

For v1: stub the interface, ship without active inference, log the analyzer stats so users opting into telemetry can help train future models.

---

## 17. PHASED BUILD ORDER (CRITICAL — DO NOT SKIP AHEAD)

Build the project in these phases, fully completing each before starting the next. Each phase has acceptance criteria.

### Phase 0 — Scaffolding (2-3 hours)
- CMake top-level + submodule wiring.
- All four target stubs build to no-op binaries.
- CI workflow runs and produces artifacts.
- **Acceptance:** `cmake --build` produces `veyra.exe`, `veyra-service.exe`, `veyra-apo.dll`, `veyra-overlay.exe`. CI passes.

### Phase 1 — Service + UI + IPC backbone (1 day)
- Service installs/uninstalls cleanly. Logs to `%APPDATA%\\Veyra\\logs\\`.
- UI connects to service via named pipe. Sends ping, gets pong.
- Config load/save round-trip works.
- **Acceptance:** UI displays "Service connected · v1.0.0". Killing service shows reconnect spinner.

### Phase 2 — APO skeleton with passthrough (1 day)
- APO registers as EFX on the default output endpoint.
- APO does pure passthrough (no DSP yet).
- Shared memory parameter block established.
- **Acceptance:** With Veyra APO active, audio plays normally. No glitches. Task Manager shows audiodg.exe normal CPU.

### Phase 3 — Core DSP chain (3 days)
- Graphic EQ (10-band) functional.
- Bass boost, treble boost, volume gain, mono/balance.
- Compressor + limiter.
- All parameter changes smooth (no zipper).
- Spectrum analyzer + VU + clipping detection writing to ring buffer.
- **Acceptance:** Sliders in a stub UI move bands and audio reflects it within 50ms.

### Phase 4 — UI proper (3 days)
- Theme system + glass rendering.
- Home screen with full EQ + 8 visualizers + effect knobs.
- Mini-mode.
- Tray icon + custom popup menu.
- **Acceptance:** Home screen looks like the Stitch frontend output. All EQ controls work end-to-end through the APO.

### Phase 5 — Presets + Per-App + Per-Device (2 days)
- Preset save/load/export/import.
- Built-in presets shipped.
- Per-app rule engine in service with rate-limiting + priority resolution.
- Per-device profile memory.
- **Acceptance:** Switch Spotify → Discord, presets auto-swap with < 100ms latency.

### Phase 6 — Voice/Mic chain (2 days)
- Mic APO with the full chain.
- RNNoise integrated and tunable.
- Mic profiles shipped.
- Side-tone routing.
- **Acceptance:** Mic monitor in Sound Lab shows clean voice with background AC hum eliminated.

### Phase 7 — Spatial / HRTF (2 days)
- Partitioned convolution working with MIT KEMAR HRTF.
- Two HRTF presets (Cinematic, Competitive).
- Virtual Headset mode with intensity slider.
- Crossfeed for speaker mode.
- **Acceptance:** A test surround signal moves around your head clearly on stereo headphones.

### Phase 8 — Gamer Mode + Sound Tracker (3 days)
- Loopback capture in service.
- Feature extraction + category classifier.
- Direction estimator.
- Overlay process with 3 radar modes + competitive/rich modes.
- Auto-detection of games + anti-cheat blocklist behavior.
- **Acceptance:** In a non-anti-cheat shooter, the radar shows enemy directions ~accurately, no D3D hooks, no anti-cheat triggers in Valorant.

### Phase 9 — Sound Lab + Night Mode + Sleep Timer (1 day)
- All 7 testing tools functional.
- Night mode compression + bass reduction + dialogue emphasis + schedule.
- Sleep timer with exponential fade-out.
- **Acceptance:** Run all 7 tests successfully. Sleep timer fades to silence over user-set duration.

### Phase 10 — Sound Sharing (multi-output) (1 day)
- Multi-device routing with latency compensation.
- Per-output independent EQ (a second APO instance per additional output? Or post-mix software router? Pick the lighter approach.)
- **Acceptance:** Play Spotify, mirror to both laptop speakers and a Bluetooth headphone. EQ differs per output.

### Phase 11 — Onboarding + Settings + Hotkeys (1 day)
- 4-step onboarding.
- All Settings sub-screens.
- Global hotkey manager.
- **Acceptance:** First-run experience complete. All hotkeys work.

### Phase 12 — Loudness Normalization + Loudness Lab (1 day)
- EBU R128 short-term integrated loudness measurement.
- True-peak limiter.
- "Loudness match" mode that holds perceived volume across sources.

### Phase 13 — Updater + Crash Reporter + Localization (1 day)
- Squirrel-style updater fully wired.
- Crash log capture and UI banner.
- All 6 languages translated.

### Phase 14 — MSIX + Portable + Signing (1 day)
- MSIX builds and installs cleanly.
- Portable ZIP launches without install.
- Code signing documented and tested in dev (test-signing mode).

### Phase 15 — Polish + Performance Pass (1-2 days)
- Profile every target, eliminate hot spots.
- Verify every performance budget number.
- Run for 8 hours straight as a soak test — zero leaks, zero glitches.
- **Acceptance:** All performance targets met. Documented benchmarks vs FxSound / Equalizer APO / Boom 3D.

---

## 18. WHAT NOT TO DO

- Do not use Electron, .NET, or any heavy runtime. Native C++ only.
- Do not hook DirectX, OpenGL, or Vulkan for the overlay by default. Layered window only.
- Do not allocate memory in the APO real-time thread.
- Do not call Windows APIs that may block from the APO.
- Do not include closed-source dependencies. GPL-compatible only.
- Do not include analytics SDKs or telemetry libraries. Telemetry, if any, is opt-in and built from scratch.
- Do not implement VST hosting in v1.
- Do not implement cloud preset sync in v1.
- Do not implement Auto-EQ from headphone curves in v1 (architect for it as v2).
- Do not invent UI elements not in the Stitch design brief.

---

## 19. DELIVERABLES

When asked to "build Veyra Sounds", produce:

1. The full repository structure above with real C++ source files (not placeholders).
2. CMakeLists.txt at every level, building cleanly with `cmake --preset windows-release && cmake --build --preset windows-release`.
3. README.md with build instructions, screenshot, contribution guide reference.
4. CONTRIBUTING.md.
5. The complete DSP module with working EQ, dynamics, spatial, voice processing.
6. APO source + INF + registration scripts.
7. Service source.
8. UI source with all screens scaffolded and Home + Mini-mode + Settings → Appearance fully implemented to a polished standard.
9. Overlay source.
10. Installer source (MSIX manifest + portable scripts).
11. CI workflow YAML.
12. Tests for the DSP layer (golden audio comparisons).
13. A `BUILD_GUIDE.md` covering signing, driver registration, test-mode setup.
14. A `ARCHITECTURE.md` containing the diagram from section 2.

If a single response is too long, complete one phase at a time and announce: "Phase N complete. Ready for phase N+1." Wait for confirmation before continuing.

---

## 20. NOW BEGIN

Start with **Phase 0**. Produce the complete CMake scaffolding, third-party submodule list, repository structure, and all stub targets that build to no-op binaries. Confirm Phase 0 acceptance criteria, then await confirmation before starting Phase 1.

Throughout: write code that you'd be proud to have on the front page of /r/cpp. Comment intent, not mechanics. Name variables like a senior engineer. Use modern C++ idioms (RAII, `std::expected`, ranges where appropriate). Optimize for clarity until profiling demands otherwise — then optimize ruthlessly.

Veyra Sounds is going to be the audio enhancer Windows always deserved. Build it like it's the only one that will ever exist.
