# Veyra Sounds — Build Progress Log

A running, honest record of what's built. For the precise, feature-by-feature
audit against the engineering spec see [`docs/FEATURE_COVERAGE.md`](docs/FEATURE_COVERAGE.md).

**Repo:** https://github.com/NextGenDev-KSK/veyra
**Current version:** 0.3.0

> Verification model: this machine has no local C++ toolchain, so every change is
> proven by the GitHub Actions `windows-latest` build (compile + link +
> `dsp_tests` + `common_tests`). **CI green = compiles, links, unit tests pass.**
> Items marked ⏵ are runtime-only — they need a real Windows audio session
> (test-signing, devices, live audio) and can't be CI-verified. Items marked ⬜
> need external assets/data that can't be produced in-repo.

---

## Phases 0–15 (spec §17) — all `[x]` and CI-green

| Phase | What | Status |
| --- | --- | --- |
| 0 Scaffolding | 4 binaries, CMake/presets, CI, docs | ✅ |
| 1 Service + UI + IPC | SCM service, named-pipe control, config round-trip, reconnect | ✅ |
| 2 APO passthrough + DSP | render `VeyraApoEfx`, seqlock shared params, COM reg | ✅ (endpoint assoc ⏵) |
| 3 Core DSP chain | 10-band EQ, tone, comp, limiter, smoothing, analyzer | ✅ (unit-tested) |
| 4 UI (JUCE) | glass theme (11), TopBar/Sidebar, EQ, knobs, mini, tray | ✅ |
| 5 Presets / Per-app / Per-device | `.vpreset`, library, rule engine, device profiles | ✅ |
| 6 Voice / Mic chain | HPF→NS→comp→de-ess→presence→gain, capture APO | ✅ (RNNoise model ⬜) |
| 7 Spatial / HRTF | partitioned convolver, crossfeed, virtual surround | ✅ (measured MIT KEMAR default; synthetic fallback) |
| 8 Gamer Mode + Sound Tracker | loopback producer, classifier, direction, overlay | ✅ (live blips ⏵) |
| 9 Sound Lab + Night + Sleep | 7 tools + tone engine, night mode, sleep fade | ✅ |
| 10 Sound Sharing | OutputRouter + delay comp; AudioBridge no-driver path | ✅ |
| 11 Onboarding + Settings + Hotkeys | 4-step wizard, sub-nav, global RegisterHotKey | ✅ |
| 12 Loudness | BS.1770 meter, true-peak, loudness-match | ✅ |
| 13 Updater + Crash + i18n | crash capture+banner, updater check; framework i18n | ✅ (6 langs ⬜, fetch✅/apply ⏵) |
| 14 MSIX + Portable + Signing | portable ZIP (CI artifact), MSIX, signing docs | ✅ (production signing ⏵) |
| 15 Performance pass | RT-factor benchmark + soak (no NaN, ceiling held) | ✅ (8 h soak / competitor bench ⏵) |

---

## Beyond the roadmap (delivered to match the reference UI + commercial spec)

All CI-green:

- **Window**: fixed 1200×675 canvas (centered, no resize/maximise); padding-trimmed taskbar icon.
- **Home**: realigned master cluster (toggle + MASTER label + slider + %), live active-preset chip, per-feature info "ⓘ" tooltips (what / does / shortcut).
- **Equalizer**: **Parametric EQ end-to-end** — 16-band engine (bell/shelf/notch/HP/LP) + draggable node editor (drag freq/gain, wheel = Q, right-click = type, double-click = reset) + summed response curve; Graphic↔Parametric toggle real on both APO + bridge paths.
- **Effects DSP in the live chain**: Reverb (Freeverb), loudness, true-peak limiter, width, HRTF — all process audio (delay intentionally omitted).
- **Visualizers**: all 8 modes (Bars/Monstercat/Circular/Waveform/Particle/Wavy/Ferrofluid/3D) — 2D-canvas (OpenGL/shader rewrite ⬜).
- **Presets**: 27 built-ins across Gaming/Music/Movies/Streaming/Voice/Night; category column, search (oval), sort (A–Z/Category), grid/list, **Favorites** (★, persisted), **Recently Used** (persisted), duplicate, import/export; white BUILT-IN badge.
- **Apps**: themed table (App·Detection·Preset·Volume·Auto-mute·Status), Per-App Switching toggle, offline **app index with EXE-icon picker**.
- **Sound Lab**: 7-tool tab bar + real `SoundLabEngine` (tone/sweep/noise/per-channel/polarity + live mic meter).
- **Gamer Mode**: 2×2 dashboard (Sound Tracker / Spatial / Voice & Mic / Night) + **game auto-detection** (process-name, anti-cheat-safe).
- **Mic**: AGC (−16 LUFS), noise gate, **AEC** (NLMS engine, tested), de-esser, presence, side-tone.
- **Scene-aware detector** (§16 v1): rule-based content classifier (stub interface, no ML), tested.
- **Settings**: sub-nav (Appearance / Audio Engine / Microphone / Spatial / Loudness / Updates / About), real Audio Engine panel.
- **Service**: updater HTTPS check (WinHTTP, daily); crash-recovery banner in the UI.

---

## Genuinely remaining

**Needs external assets / data — cannot be produced in-repo (would be faking it):**
- ⬜ **Real RNNoise model** — the mic uses a real custom spectral/expander noise suppressor; dropping in the trained RNNoise model needs its weights + lib vendored behind the existing seam.
- ✅ **MIT KEMAR HRTF** — the measured diffuse set is vendored + loaded by `HrtfDatabase` and is the default; synthetic HRIR is the fallback. (Done.)
- ⬜ **Human translations** (ZH-CN, ES, AR, HI, TA) — the localization framework + English exist; accurate catalogs need translators (not machine-faked). RTL pass pending.

**Runtime-only — real code exists, but can't be CI-verified (needs a PC):**
- ⏵ APO endpoint registration + hearing audio through `audiodg` (test-signing/admin).
- ⏵ AEC live wiring (far-end render→capture reference feed) — the NLMS engine is done + tested.
- ⏵ Live Sound-Tracker blips; production code-signing; updater download/apply; 8-hour soak + competitor benchmarks.

**Deferred by the "quality > feature count" rule:**
- OpenGL/shader-grade visualizer rewrite (8 modes already ship as 2D); echo/delay in the chain (no reference UI, low value); overlay hold-to-interact + per-game memory; pitch/time.

---

_Log started 2026-06-14. `FEATURE_COVERAGE.md` is the detailed §1–§16 audit._
