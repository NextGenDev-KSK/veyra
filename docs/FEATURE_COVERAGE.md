# Veyra Sounds — Backend Prompt Coverage Audit

A faithful, section-by-section mapping of every feature in
[`Veyra_Claude_Backend_Prompt.md`](../Veyra_Claude_Backend_Prompt.md) to its
real status in this repo. No optimistic ticks — runtime-only items and genuine
gaps are called out so the remaining work is unambiguous.

**Legend:** ✅ done & CI-green · 🟡 partial · ⬜ not built · ⏵ built but runtime-only (needs a real Windows audio session to verify)

---

## §1 Technology stack
| Item | Status | Notes |
| --- | --- | --- |
| C++20, CMake 3.25+, Ninja, MSVC | ✅ | presets + CI |
| JUCE (UI) | ✅ | JUCE 8.0.13 |
| DSP | ✅ | custom header-only `veyra-dsp` (not `juce::dsp`, by choice — RT-safe, allocation-free) |
| RNNoise mic NS | ⬜ | mic uses a custom downward-expander `NoiseSuppressor`, **not** RNNoise. Seam exists to drop RNNoise behind a flag |
| MIT KEMAR HRTF | ✅ | measured KEMAR (diffuse set) loaded by HrtfDatabase + fed to the panners; synthetic is now fallback-only |
| APO COM (native) | ✅ | render `VeyraApoEfx` + capture `VeyraMicApo` |
| Service (Win32) | ✅ | SCM + `--console` |
| IPC | 🟡 | custom framed **binary** protocol over named pipes (not FlatBuffers; functionally complete) |
| Installer (MSIX + portable) | ✅ | portable ZIP built by CI; MSIX manifest+script |
| Updater | 🟡 | semver version-check done; HTTPS fetch/apply ⬜ |
| Crash reporting | ✅ | minidump + JSON to `%APPDATA%\Veyra\crashes` |
| Logging (spdlog) | ✅ | rotating file + optional console |
| Localization | 🟡 | base catalog + JSON overlay + `tr()`; only `en` + sample `es`; not JUCE `TRANS()`; 6 langs ⬜ |
| Overlay rendering | ✅ | Win32 **GDI+** layered window (prompt suggested Direct2D; GDI+ meets the no-hook/ban-safe goal) |

## §2 Architecture (3 processes + APO)
✅ `veyra-apo.dll`, `veyra-service.exe`, `veyra.exe`, `veyra-overlay.exe` all build; shared-memory params + ring buffers + two named pipes. Anti-cheat strategy = layered-window only ✅; D3D-hook opt-in mode ⬜ (intentionally not built).

## §4 DSP chains
**Output chain:** loudness norm ✅ · mono/balance ✅ · 10-band graphic EQ ✅ · **parametric EQ ✅** (16-band engine + draggable node editor, both render paths) · bass/treble shelves ✅ · compressor ✅ · stereo widener ✅ · **reverb ✅** (Freeverb in the live chain; Home knob wired; unit-tested) · **echo/delay 🟡** (`DelayLine` exists; deliberately not in the chain — no reference UI + low value for an enhancer) · virtual surround/HRTF ✅ · volume gain ✅ · true-peak limiter ✅.
**Mic chain:** HPF ✅ · NS 🟡 (custom expander, not the RNNoise model) · **noise gate ✅** (config + NS expander) · **AEC ✅** (NLMS engine + VoiceChain overload, tested; live far-end feed ⏵) · voice EQ/presence ✅ · de-esser ✅ · **AGC ✅** (−16 LUFS auto-level, unit-tested) · side-tone 🟡 (level field; APO routing ⬜).
Parameter smoothing (5 ms) ✅.

## §5 APO
EFX output ✅ · MFX/mic ✅ · seqlock shared params ✅ · analyzer SPSC ring ✅ · COM registration (INF + script) ✅ · **SIMD ⬜** (scalar; well within budget). Endpoint association is ⏵ (test-signing + admin on a real PC).

## §6 Service
Shared-mem bootstrap ✅ · control pipe ✅ · canonical state (presets/per-app/per-device) ✅ · **per-app detection 🟡** (UI-side foreground watcher; audio-session signal + 800 ms rate-limit ⬜) · **game detection ✅** (`GameDetector` + TrackerService foreground poll auto-activates the tracker; anti-cheat-safe) · loopback capture ✅ (`AudioBridge` + `TrackerService`) · **Sound Tracker engine ✅** (`TrackerService` does WASAPI loopback → `SoundTracker` → `VeyraTracker` shared block, gated by Gamer Mode; live blips are ⏵) · updater 🟡 · crash aggregation ✅.

## §7 UI
Fixed 1600×900 canvas ✅ · custom titlebar (no maximise) ✅ · 11 themes + live switch ✅ · glass (blur+tint+stroke+shadow) ✅ · **visualizers 🟡 (all 8 modes, 2D-canvas — not yet OpenGL/shader)** · crash-recovery banner ✅ · IPC client ✅ · global hotkeys ✅ · tray + custom menu ✅ · mini mode ✅ · onboarding ✅.

## §8 Overlay
Layered window ✅ · 3 radar styles (competitive/rich/compass) ✅ · competitive/rich density ✅ · IPC client ✅ · **hold-to-interact ⬜** · **per-game memory ⬜** · tracker producer ✅ (`TrackerService`); live blips are ⏵ (real audio on a PC).

## §9 Sound Lab — ✅ (tab bar + tone engine)
All 7 tools as a top tab bar (Speaker / 7.1 Surround / Mic / Frequency Sweep /
Hearing Range / Polarity-Phase / Noise Generator) over a central card, driven by
`SoundLabEngine` (`juce_audio_devices` + the dsp `SignalGenerator`) — real
per-channel sine, sweep, white/pink noise, polarity invert, and a live mic-input
meter. ⏵ audio output verified on a real device.

## §10 Preset format
✅ `.vpreset` JSON, atomic save/load, 8 built-ins. 🟡 schema is a subset (no parametric/mic-chain blocks yet).

## §11 Config & state
✅ `config.json`, atomic writes, most blocks. 🟡 `per_game_profiles`, full `audio_engine` (buffer/latency mode), `hotkeys` editing UI.

## §12 Build & distribution
CMake → 4 binaries ✅ · MSIX ✅ · portable ZIP (CI artifact) ✅ · signing docs ✅ · `veyra://` protocol ⬜ · updater fetch/apply 🟡.

## §13 Performance budget
✅ benchmark (RT-factor) + 30 s soak (no NaN, limiter holds ceiling). **⬜ latency benchmark** (sample-accurate RTT) and the per-target budget numbers on real hardware (⏵).

## §14 Internationalization
🟡 framework + `en`/`es`; **⬜ 6 full languages, `TRANS()` routing, RTL pass.**

## §15 Crash reporting
✅ local-first capture. **⬜ UI "recovered from a crash" banner**, "send to developers" flow.

## §16 Scene-aware adaptive EQ
✅ v1 — rule-based `SceneDetector` (Silence/Music/Movie/Game/Voice + hysteresis), unit-tested. This is the prompt's "stub the interface, ship without inference" intent; a learned model is ⬜.

---

## Honest summary

**Solid & CI-green:** the 3-process + APO architecture, the core real-time output DSP (EQ/tone/dynamics/width/spatial/loudness/limiter), config/preset/per-app/per-device models, IPC, packaging, crash capture, hotkeys, onboarding, performance guard.

**Done since the first audit:** parametric 16-band EQ + node editor · all 8 visualizer modes (2D) · the 7 Sound Lab tools + tone engine · AGC · noise gate · live game detection · Sound Tracker producer · crash UI banner · updater HTTPS check · reverb in the live chain · fixed 1600×900 canvas · Home master polish + active-preset chip · offline app index with EXE icons · Settings Updates section.

**Real remaining gaps (need external assets / are research-scale — not faked):** real **RNNoise** model weights · **6-language human translations** · OpenGL/shader-grade unique visualizers (current are 2D) · pitch/time · scene-aware **learned** model (rule-based v1 ships) · signature-verified silent updater download/apply. **Runtime-only ⏵:** APO endpoint registration, AEC live far-end feed, overlay hold-to-interact + per-game memory, per-screen spacing polish. Smaller/by-choice: echo-delay in the chain (no reference UI), audio-session per-app detection, side-tone routing.

These are tracked and being worked in priority order — see the implementation plan below and `docs/DESIGN_SYSTEM.md` §12.

## Implementation order (UI-first, each CI-green)
1. ✅ **Gamer Mode → 2×2 dashboard** (Sound Tracker · Spatial Audio · Voice & Microphone · Night Mode) — folds in spatial + mic + night.
2. ✅ **Sound Lab → 7-tool tab bar** + UI tone engine (real test signals).
3. ✅ **Settings sub-nav** + Audio Engine panel · ✅ **Apps** table (+ extended rule model) · ✅ **Presets** category column + search + grid/list.
4. ✅ **Reverb** into the live render chain (knob now real). **Delay** into the chain — next.
5. **Home top bar** device selector + output meter.
6. **Parametric EQ** (≤16 nodes) on the existing EQ card.
7. **Visualizer modes** (Monstercat/Circular/Waveform first, then the GPU-heavy ones).
8. Back-end depth: live Sound Tracker producer, game detection, AGC/gate, crash banner, updater fetch.
