# Veyra Sounds — Build Progress Log

A running record of what was done in each phase. Tick marks (`[x]`) advance as
phases complete and pass CI. Phase acceptance criteria come from the engineering
spec §17.

**Repo:** https://github.com/NextGenDev-KSK/veyra
**Current version:** 0.3.0
**Last green CI:** [run 27585319582](https://github.com/NextGenDev-KSK/veyra/actions/runs/27585319582) (Phase 0–5: + presets/per-app rules/per-device profiles engine, IPC, and Presets UI; builds, dsp_tests + common_tests pass)

> Verification note: this machine has no local C++ toolchain, so every phase is
> proven by the GitHub Actions `windows-latest` build (compile + link + the
> "four binaries exist" check). CI green = compiles & links. Runtime behaviour
> (audio, service install, pipe handshake) still needs a real Windows run.

---

## ✅ Phase 0 — Scaffolding  `[x] COMPLETE` (CI green)

Established the full repository and a buildable no-op skeleton.

- [x] Repo tree per spec §3 (apps/, modules/, third_party/, resources/, ipc/, installer/, tests/) with `.gitkeep` placeholders
- [x] Top-level `CMakeLists.txt` — C++20, `VEYRA_VERSION`, `RC` language, unified `bin/` output, `VEYRA_FETCH_DEPS` option (OFF)
- [x] `CMakePresets.json` — `windows-release` / `windows-debug` (Ninja + MSVC)
- [x] Four no-op stub targets build: `veyra.exe`, `veyra-service.exe`, `veyra-apo.dll`, `veyra-overlay.exe`
- [x] `veyra::common` (static) + `veyra::dsp` (interface) module stubs; generated `version.h`
- [x] App manifests (UI asInvoker + PerMonitorV2 DPI) embedded via `.rc`; MSVC duplicate-manifest pitfall handled
- [x] `.gitmodules` + guarded `third_party/CMakeLists.txt` (JUCE/rnnoise/spdlog/flatbuffers/kissfft declared, not fetched)
- [x] GitHub Actions `build.yml` — configure → build → verify 4 binaries → upload artifacts
- [x] Docs: `README`, `ARCHITECTURE` (full diagram), `BUILD_GUIDE`, `CONTRIBUTING`; `LICENSE` (verbatim GPLv3); `.gitignore` / `.gitattributes`

**Acceptance met:** `cmake --build` produces all four binaries; CI passes.

---

## ✅ Phase 1 — Service + UI + IPC backbone  `[x] COMPLETE` (CI green)

The orchestrator service, a minimal UI shell, and the named-pipe IPC between them.

- [x] **IPC protocol** (`veyra-common/ipc/Protocol.h`) — small versioned framed binary protocol (Ping/Pong, GetVersion, GetConfig/SetConfig, Error)
- [x] **PipeServer** — single-listener `\\.\pipe\veyra-control`, clean `CancelSynchronousIo` shutdown; **PipeClient** — sync request/reply; **PipeIo** — framed message read/write
- [x] **Config** — `nlohmann/json` (vendored, MIT) load/save with atomic `MoveFileEx` replace; **Paths** (`%APPDATA%\Veyra`); **Logging** (thread-safe file logger)
- [x] **Service** runs under the SCM (`ServiceMain`) or `--console`; `--install` / `--uninstall` register an auto-start LocalSystem service
- [x] **ServiceRuntime** wires `ConfigManager` + `ControlServer`; config loads or writes-and-verifies a default (round-trip check)
- [x] **UI** — minimal native Win32 shell showing live status ("Service connected · v0.1.0" / animated reconnect)
- [x] **ServiceClient** — background thread, version handshake, keepalive pings, exponential-backoff auto-reconnect (cap 30s)
- [x] Build fix: `UNICODE`/`_UNICODE`/`NOMINMAX` defined project-wide

**Acceptance (build-verified):** all targets compile & link on CI.
**Still to runtime-verify:** UI displays "Service connected · v…"; killing the service shows the reconnect spinner.

---

## ⬜ Remaining phases (planned)

## ✅ Phase 2 — APO with passthrough + DSP  `[x] COMPLETE` (CI compiles/links; runtime = your PC)

A real output APO built against the base Windows SDK (no WDK), running the
veyra::dsp chain and reading parameters from the service via shared memory.

- [x] `VeyraApoEfx` implements IAudioProcessingObject / RT / Configuration + IAudioSystemEffects directly (no WDK base class)
- [x] APOProcess: deinterleave → DspChain → reinterleave; allocation/lock-free RT path (scratch sized in LockForProcess)
- [x] Shared-memory parameter block (`Local\VeyraAPOParameters_v1`, cache-aligned, **sequence-locked**) + unit test
- [x] DLL class factory + COM exports (.def) + CLSID self-registration
- [x] Service `ApoPublisher` writes config-derived params; `ConfigManager` onChanged republishes live
- [x] INF template + `register-apo.ps1` + BUILD_GUIDE dev-registration steps
- [ ] Register as EFX on an endpoint + confirm passthrough audio — **runtime-only** (needs your PC: test-signing + admin + endpoint association)

**Acceptance (build-verified):** APO compiles & links; `veyra-apo.dll` produced; seqlock test passes.
**Still to runtime-verify:** audio actually plays through Veyra with normal `audiodg.exe` CPU.

## ✅ Phase 3 — Core DSP chain  `[x] COMPLETE` (CI green, DSP tests pass)

Header-only, allocation-free, RT-safe DSP in `veyra-dsp`, with a Catch2 suite.

- [x] 10-band graphic EQ (biquad cascade); bass/treble shelves; volume gain; mono/balance
- [x] Compressor (soft-knee, stereo-linked) + lookahead limiter (guaranteed ceiling)
- [x] Smooth parameter changes (one-pole, zipper-free); spectrum analyzer + VU + clipping → lock-free ring buffer
- [x] DSP tests (Catch2): smoothing, biquad response, EQ boost/cut, shelves, stereo, compressor ratio, limiter ceiling, FFT bin, ring-buffer FIFO, analyzer metering, end-to-end chain
- [x] spdlog integration (header-only, behind the Logger facade; rotating file sink)
- [x] Wire DSP into the APO real-time path (done in Phase 2)

**Acceptance (build + unit-test verified):** all DSP blocks behave correctly under Catch2.
**Still to runtime-verify:** sliders move bands and audio reflects it (needs the APO + a real run).

### Phase 4 — UI proper (JUCE) — sub-phased

**✅ 4a — Foundation `[x] COMPLETE` (CI builds the JUCE app)**
- [x] JUCE 8.0.13 integrated; veyra-ui is a juce_add_gui_app; Orbitron/Inter/JetBrains Mono (OFL) embedded
- [x] Design-token palettes (Midnight + 10 variants) + ThemeManager with live switching
- [x] VeyraLookAndFeel; GlassBackground (ambient blobs + grain) + GlassCard recipe
- [x] Components: ToggleSwitch, Knob, SegmentedControl (+ themed buttons/sliders/combo)
- [x] Foundations gallery view; runnable veyra.exe artifact for visual verification
- [ ] **Runtime-verify by running the artifact** (download from CI, eyeball look + theme switch)

**🟡 4b — Home screen (CI builds; awaiting visual check on the artifact)**
- [x] Real frosted glass (blurred backdrop slice + tint) — fixes the flat 4a look
- [x] Borderless window + custom glass TopBar (logo/wordmark/master/preset/icons/window controls + drag)
- [x] Sidebar: 7 nav items + icons + active accent rail + mini + version
- [x] Visualizer card (animated spectrum + L/R VU + mode dropdown + fullscreen + FPS)
- [x] 10-band EQ card (band sliders + center detent + response curve + Graphic/Parametric + toggles + reset)
- [x] 6 effect knobs + More Effects; mockup-matched layout
- [ ] **Runtime-verify by running the artifact** (look + interactions)
- [x] ServiceClient re-integrated (callback-based, fetches config, coalescing SetConfig push)
- [x] EQ/master/effect knobs drive DSP params end-to-end (UI → SetConfig → ConfigManager → ApoPublisher → shared memory)
- [x] Config extended with `enhancement` block (EQ bands, bass/treble, volume gain, width, compression, reverb); TopBar connection LED

**🟡 4c — Routing + Settings→Appearance + Mini-mode + Tray (CI builds; awaiting visual check)**
- [x] App shell: chrome (background/top bar/sidebar) promoted out of HomeScreen; single working Config
- [x] Sidebar routing between content screens (Home, Settings, on-brand placeholders for the rest)
- [x] Settings→Appearance: 11-theme preview grid (live switch + persisted via config.theme), reduce-motion (freezes visualizer)
- [x] Opacity + background-mode controls present (deep wiring deferred to a persisted-appearance config pass)
- [x] Mini-mode bar (master toggle/volume + expand/close, always-on-top, shares config/client)
- [x] System tray icon + popup menu (open / mini / toggle master / quit); themed standard menu (glass popup later)
- [ ] **Runtime-verify by running the artifact** (routing, theme switch, mini, tray)

### 🟡 Phase 5 — Presets + Per-App + Per-Device (CI builds + unit tests pass; awaiting runtime check)
- [x] `.vpreset` model (enhancement + master + metadata), JSON round-trip, atomic save/load, applyTo(Config)
- [x] 8 built-in presets with stable UUIDs; `presetsToJsonArray`/`presetsFromJsonArray` for the wire
- [x] Service PresetLibrary (built-ins + user dir CRUD, thread-safe); IPC list/load/save/delete
- [x] Per-app rule engine (case-insensitive substring + priority resolution) + RateLimiter; IPC get/set + persisted to app_rules.json
- [x] Per-device profile memory (DeviceProfiles, JSON persistence)
- [x] Presets UI screen (grid, apply, save-current, import/export, delete) wired through ServiceClient
- [x] `common_tests`: preset round-trip/built-ins, rule resolution/priority/disabled, rate limiter, device-profile persistence
- [ ] Live application of per-app rules on focus change (needs foreground tracking — Phase 8)
- [ ] Per-device recall activation (needs device-change detection — later phase)
- [ ] **Runtime-verify by running the artifact** (apply preset, save/import/export)

### 🟡 Phase 6 — Voice / Mic chain (CI builds + unit tests pass; awaiting runtime check)
- [x] Voice DSP chain (HPF → noise suppression → leveling comp → de-esser → presence EQ → output gain), header-only/RT-safe
- [x] NoiseSuppressor (amount-driven downward-expanding gate); phase-correct dynamic high-shelf de-esser; Biquad HPF/BPF + mono compressor
- [x] `VoiceConfig` in Config (JSON round-trip), `VeyraMicSharedParameters` seqlock block, service `MicPublisher`
- [x] Capture APO (`VeyraMicApo`) running the voice chain from the mic shared block; dual-CLSID DLL factory + registration
- [x] Tests: noise-suppressor gate/pass, voice-chain passthrough/HPF/level/de-esser, voice-config round-trip
- [ ] RNNoise as a learned-model backend behind a build flag (NoiseSuppressor is the default seam)
- [ ] Mic settings UI panel; side-tone monitor routing; mic profiles
- [ ] **Runtime-verify by running the artifact** (mic capture path needs the driver INF + test-signing)

### ⬜ Phase 7 — Spatial / HRTF
- [ ] Partitioned convolution with MIT KEMAR; Cinematic + Competitive presets; virtual headset + crossfeed

### ⬜ Phase 8 — Gamer Mode + Sound Tracker
- [ ] Loopback capture; feature extraction + DSP classifier + direction estimator
- [ ] Overlay (3 radar modes, competitive/rich); game + anti-cheat detection (layered-window only)

### ⬜ Phase 9 — Sound Lab + Night Mode + Sleep Timer
- [ ] 7 testing tools; night mode; sleep timer with exponential fade-out

### ⬜ Phase 10 — Sound Sharing (multi-output)
- [ ] Multi-device routing with latency compensation; per-output EQ

### ⬜ Phase 11 — Onboarding + Settings + Hotkeys
- [ ] 4-step onboarding; all settings sub-screens; global hotkey manager

### ⬜ Phase 12 — Loudness Normalization + Loudness Lab
- [ ] EBU R128 measurement; true-peak limiter; loudness-match mode

### ⬜ Phase 13 — Updater + Crash Reporter + Localization
- [ ] Squirrel-style updater; crash capture + UI banner; 6 languages

### ⬜ Phase 14 — MSIX + Portable + Signing
- [ ] MSIX package; portable ZIP; code-signing documented + test-signing

### ⬜ Phase 15 — Polish + Performance Pass
- [ ] Hit every performance budget (§13); 8-hour soak test; benchmarks vs competitors

---

_Log started 2026-06-14. Update the tick marks and the "Last green CI" line as each phase lands._
