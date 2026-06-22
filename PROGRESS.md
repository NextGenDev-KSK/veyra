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

## Roadmap to 1.0 and beyond (Phases 16–20) — PROMPT 2 TODO

Legend: `[ ]` not started · `[~]` in progress · `[x]` done · ⏵ runtime/hardware-only · ⬜ needs external asset.

### Phase 16 — Sound Quality Engine  `[ ]`
- [x] **RNNoise** as the default mic NS, custom suppressor as fallback — submodule @ v0.1.1 (small embedded model), builds on MSVC (VLA→alloca patch), opaque RT-safe `RnnoiseDenoiser` wired into the capture APO; unit-tested (stationary noise suppressed). CI-green.
- [ ] **Live AEC far-end reference** wiring (render→capture reference) so echo cancel works on real devices  *(NLMS engine done; ⏵ live)*
- [ ] **Limiter upgrade** — oversampling + look-ahead + inter-sample-peak (ISP) protection + soft clip
- [ ] **Optional oversampling** (2×/4×/8×) for nonlinear DSP modules (anti-alias)
- [ ] **Equal-loudness (ISO 226)** compensation — natural bass/treble at low volume
- [ ] **Harmonic exciter** (transparent presence/clarity)
- [ ] **Saturation** modes — tape / tube / transparent (subtle harmonics)
- [ ] **Multiband stereo width** — mono lows, widen highs
- [ ] **Transient enhancement** (attack/detail)
- [ ] **Adaptive bass management** (punch without clipping)
- [ ] **Headphone-safe listening mode** (loudness norm + fatigue reduction)
- [ ] **Reference Listening Mode** (flat, bypass all coloration)

### Phase 17 — Headphone & Spatial Suite  `[ ]`
- [x] **AutoEQ database** — pick headphone model → auto-load parametric corrections. Parser (`veyra::AutoEq`) for the AutoEq ParametricEQ.txt format + 16 vendored oratory1990 corrections (BinaryData-embedded) + an "AutoEQ" picker on the EQ card. Tested. CI-green. *(1.0 Required)*
- [ ] **Advanced crossfeed** — frequency-dependent, delay-compensated (Bauer/Meier)
- [ ] **Headphone optimization profiles** (impedance / driver / open-vs-closed)
- [ ] **Multiple HRTF databases** — KEMAR (done) + CIPIC + IRCAM, user-selectable
- [ ] **Personalized HRTF** (head width, ear size, preference)
- [ ] **Better HRTF interpolation** (eliminate localization jumps)
- [ ] **Diffuse-field / free-field** headphone compensation
- [ ] **Room simulation / speaker virtualization** (cinematic)
- [ ] **Binaural speaker emulation** (Waves NX / Dolby Headphone style)

### Phase 18 — Sound Lab Pro + Personalization  `[ ]`
- [ ] **Pro measurement tools** in Sound Lab — frequency-response analysis, channel matching
- [ ] **Hearing-test-based EQ personalization** — generate per-user correction curves

### Phase 19 — UI / Product Polish + Docs + First-run  `[ ]`
- [ ] **Home** pixel-perfect alignment vs reference
- [ ] **Eliminate dead space** across Presets / Devices / Apps / Gamer Mode / Sound Lab
- [ ] **GPU/OpenGL visualizer** rendering (replace 2D)
- [ ] **Docs** — screenshots, demo videos, architecture diagrams, per-feature docs
- [ ] **Polished first-run** — auto-detect devices/headphones + recommend presets

### Phase 20 — Release Validation (runtime / hardware)  `[ ]`
- [ ] ⏵ **Hardware APO validation** on multiple Windows PCs (audio confirmed through `audiodg.exe`)
- [ ] ⏵ **8-hour soak** — fix leaks / CPU spikes / dropouts
- [ ] ⏵ **Competitor benchmark** vs FxSound / Sonar / Boom 3D / Dolby Access / Equalizer APO (CPU, latency, quality)
- [~] **Translations** — fill-in templates generated for ZH-CN/AR/HI/TA (+ ES sample), bundled + documented (docs/TRANSLATIONS.md); RTL flag set for Arabic. Human translation of the values + a live language picker / `tr()` routing remain. ⬜
- [ ] ⏵ **Production code signing**

---

## Veyra 1.0 release target

**Required:** Stable APO ⏵ · RNNoise ✅ · AutoEQ ✅ · Parametric EQ ✅ · KEMAR ✅ · Good presets ✅ · Apps page ✅ · Devices page ✅ · Localization framework ✅ (+ 5 locale templates) · Documentation `[ ]`
**Nice-to-have:** CIPIC · IRCAM · Equal-loudness · Advanced crossfeed
**Future:** Personalized HRTF · Room simulation · Binaural speaker emulation

---

_Log started 2026-06-14. `FEATURE_COVERAGE.md` is the detailed §1–§16 audit._
