# Veyra Sounds — Build Progress Log

A running, honest record of what's built. For the precise, feature-by-feature
audit against the engineering spec see [`docs/FEATURE_COVERAGE.md`](docs/FEATURE_COVERAGE.md).

**Repo:** https://github.com/NextGenDev-KSK/veyra
**Current version:** 0.9.0

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
- **Effects DSP in the live chain**: Reverb (Freeverb) on both APO + Bridge paths, loudness, true-peak limiter, width, HRTF — all process audio (delay intentionally omitted).
- **Visualizers**: all 8 modes (Bars/Monstercat/Circular/Waveform/Particle/Wavy/Ferrofluid/3D) — 2D-canvas (OpenGL/shader rewrite ⬜).
- **Presets**: 27 built-ins across Gaming/Music/Movies/Streaming/Voice/Night; category column, search (oval), sort (A–Z/Category), grid/list, **Favorites** (★, persisted), **Recently Used** (persisted), duplicate, import/export; white BUILT-IN badge.
- **Apps**: themed table (App·Detection·Preset·Volume·Auto-mute·Status), Per-App Switching toggle, offline **app index with EXE-icon picker**.
- **Sound Lab**: 7-tool tab bar + real `SoundLabEngine` (tone/sweep/noise/per-channel/polarity + live mic meter).
- **Gamer Mode**: 2×2 dashboard (Sound Tracker / Spatial / Voice & Mic / Night) + **game auto-detection** (process-name, anti-cheat-safe).
- **Mic**: AGC (−16 LUFS), noise gate, **AEC** (NLMS engine, tested), de-esser, presence, side-tone (field plumbed; sidetone audio routing is ⏵ — requires service-side WASAPI render stream fed from the capture APO).
- **Scene-aware detector** (§16 v1): rule-based content classifier (stub interface, no ML), tested.
- **Settings**: sub-nav (Appearance / Audio Engine / Microphone / Spatial / Loudness / Updates / About), real Audio Engine panel.
- **Service**: updater HTTPS check (WinHTTP, daily); crash-recovery banner in the UI.
- **APO Win 10/11 compatibility**: both render + capture APOs now implement `IAudioSystemEffects2` (empty effects list) so Windows 10 v2004+ audio policy does not skip them on modern endpoints. CI-green.
- **App Rules IPC sync**: `AppsScreen::save()` now sends `SetAppRules` to the service via `ServiceClient::setAppRules()` in addition to writing the JSON file — the service's in-memory AppRuleEngine stays in sync without a restart.
- **AudioBridge sample-rate tolerance**: destination client now initialized with `AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY` so VB-CABLE and headphones with different sample rates work without manual format matching.
- **Config version guard**: `Config::fromJson()` now normalises the `version` field to the current schema version so a downgrade can't leave an unknown version tag in the file.
- **Reverb on APO path**: `reverbAmount` added to `VeyraParamsPayload`; `ApoPublisher` and `VeyraApo::refreshParametersFromShared()` wired — reverb now applies on both APO + Bridge paths. CI-green.
- **SharedParameters padding**: seqlock pad formula made edge-case safe for payload sizes that are exact cache-line multiples.

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
- [x] **Limiter upgrade** — 4× oversampled true-peak (inter-sample) limiting: feeds `max(|sample|, ISP)` into the look-ahead window so the exact sample-peak guarantee is kept while inter-sample overshoots are tamed; no added latency. Tested. CI-green.
- [x] **Optional oversampling** — 2× oversampled saturator path (linear-interp up → shape → sub-Nyquist LP → decimate) so waveshaper harmonics are filtered instead of aliasing; top-level `nonlinearOversampling` toggle through the chain + APO + Bridge + a Settings→Audio Engine toggle. Tested (aliased image reduced). CI-green.
- [ ] **Live AEC far-end** wiring (NLMS engine exists)
- [x] **Equal-loudness (ISO 226)** compensation — volume-dependent low/high-shelf lift (`EqualLoudness`) wired after the compressor; carried through the APO + Bridge paths + a Settings→Loudness toggle. Tested. CI-green.
- [x] **Harmonic exciter** — high-band tanh saturation synthesising upper harmonics (presence/air), `EnhancementConfig.exciterAmount` through the chain + APO + Bridge + a new Settings→**Sound Quality** section. Tested. CI-green.
- [x] **Saturation modes** — transparent (odd) / tape (algebraic) / tube (even, asymmetric) full-band waveshaper with DC blocker, `EnhancementConfig.saturationAmount`+`saturationMode` through the chain + APO + Bridge + the Settings→Sound Quality section. Tested (mode-specific harmonics). CI-green.
- [x] **Multiband stereo width** — phase-perfect low/high split (~300 Hz): low band collapsed toward mono + high band widened by amount (`EnhancementConfig.multibandWidth`) through the chain + APO + Bridge + the Sound Quality section. Tested (low-side collapse, high-mid preserved). CI-green.
- [x] **Transient enhancement** — fast/slow envelope-difference attack detector applies a fading gain boost on onsets (`EnhancementConfig.transientAmount`) through the chain + APO + Bridge + the Sound Quality section. Tested (attack emphasised over sustain). CI-green.
- [x] **Adaptive bass management** — psychoacoustic bass enhancer: synthesises low-band harmonics (octave-up) + high-passes them back instead of boosting the clipping-prone fundamental (`EnhancementConfig.bassEnhanceAmount`) through the chain + APO + Bridge + the Sound Quality section. Tested. CI-green.
- [x] **Headphone-safe listening mode** — fatigue-reduction high-shelf cut (toggle) for long sessions, top-level `headphoneSafe` through the chain + APO + Bridge + a Settings→Audio Engine toggle (pair with Loudness Match). Tested. CI-green.
- [x] **Reference Listening Mode** — global flat A/B: bypasses all coloration (EQ/tone/dynamics/width/reverb/spatial/loudness), keeps master volume + safety limiter; through Config + APO + Bridge + a Settings→Audio Engine toggle. Tested. CI-green.

### Phase 17 — Headphone & Spatial Suite  `[ ]`
- [x] **AutoEQ database** — pick headphone model → auto-load parametric corrections. Parser (`veyra::AutoEq`) for the AutoEq ParametricEQ.txt format + 16 vendored oratory1990 corrections (BinaryData-embedded) + an "AutoEQ" picker on the EQ card. Tested. CI-green. *(1.0 Required)*
- [x] **Advanced crossfeed** — Bauer ITD-delay + head-shadow bleed now carries Meier-style frequency-dependent tonal compensation (a high-shelf that restores the highs the bleed's low-pass removes), so centred content stays tonally flat. Upgraded `Crossfeed` in place (no new plumbing — the existing crossfeed amount drives it). Tested (mono stays ~flat across band). CI-green.
- [x] **Headphone optimization profiles** — handled by the **AutoEQ** system above: pick your exact model to load its *measured* correction (oratory1990 data), which is strictly better than generic open/closed/impedance guesses. Generic-profile presets intentionally dropped in favour of measured data.
- [ ] **Multiple HRTF databases** — KEMAR (done) + CIPIC + IRCAM, user-selectable
- [ ] **Personalized HRTF** (head width, ear size, preference)
- [x] **Better HRTF interpolation** — ITD-aware (onset-aligned) azimuth interpolation (`hrirInterpAligned`): aligns the two bracketing HRIRs by their direct-arrival peak before crossfading and places the result at the interpolated delay, so the ITD moves continuously instead of combing/jumping. Wired into `HrtfDatabase`; endpoints reduce to the measured IR. Tested. CI-green.
- [x] **Diffuse-field / free-field** headphone compensation — `FieldCompensation` target selector (Natural / Diffuse-field / Free-field): the free-field mode adds the ~3 kHz frontal ear-gain (+ a little air) the diffuse-field target lacks; diffuse relaxes it. `SpatialConfig.fieldComp` through the chain + APO + Bridge + a "Headphone Target" selector on the Settings→Spatial card. Tested. CI-green.
- [x] **Room simulation / speaker virtualization** (cinematic) — `RoomSimulator` early-reflection network (6 irregular taps, cross-fed L↔R for width) for out-of-head depth; `SpatialConfig.room` through the chain + APO + Bridge + a "Room" slider on the Settings→Spatial card (Cinematic mode sets it to 40%). Tested. CI-green.
- [x] **Binaural speaker emulation** (Waves NX / Dolby Headphone style) — the Cinematic spatial mode now combines `VirtualSurround` (measured-KEMAR HRTF virtualisation) + `RoomSimulator` (early reflections) + advanced crossfeed, i.e. virtual speakers in a room. (Head-*tracked* emulation needs a head-tracker sensor — out of scope without hardware.)

### Phase 18 — Sound Lab Pro + Personalization  `[ ]`
- [x] **Pro measurement tools** in Sound Lab — `MeasureBands` analyzer (10 octave band-pass filters + leaky-integrated RMS) drives a **live frequency-response spectrum** from mic capture in the Mic Test (feed flat noise to read room/system response). Engine processes input → per-band levels → spectrum display. Tested. CI-green.
- [x] **Hearing-test-based EQ personalization** — correction algorithm (`personalizeFromHearingTest`, audiology half-gain rule, tested) **+ the interactive capture wizard** in Sound Lab: a "Personalize EQ" flow steps 250 Hz→8 kHz, the listener lowers each tone to threshold, and a calibration-free self-relative profile (each band vs the most-sensitive one) feeds the algorithm; the resulting parametric bands are applied as the EQ (reflected on Home). CI-green.

### Phase 19 — UI / Product Polish + Docs + First-run  `[ ]`
- [~] **Home** pixel-perfect alignment vs reference — needs a runtime screenshot pass to compare against `Reference theme/` (can't be verified headless).
- [~] **Eliminate dead space** across Presets / Devices / Apps / Gamer Mode / Sound Lab — Devices reworked (scrollable grid + fixed Bridge); the rest is a runtime visual pass.
- [~] **GPU/OpenGL visualizer** rendering — `juce::juce_opengl` linked + an `OpenGLContext` path built, but **disabled**: at runtime, attaching GL to the window blanks the UI (only the acrylic backdrop paints — GL doesn't composite with the Windows-11 DWM acrylic this app uses). `setHardwareAcceleration` no longer attaches GL; the proven CPU 2D path is always used. Re-enable only after the GL/acrylic compositing is solved (likely needs an opaque GL render target or dropping DWM acrylic when GL is on).
- [~] **Docs** — per-feature `docs/USER_GUIDE.md` + ARCHITECTURE/BUILD_GUIDE/VERIFY/TRANSLATIONS/FEATURE_COVERAGE. Screenshots + demo videos still need a runtime capture pass.
- [x] **Polished first-run** — onboarding's "Your setup" step detects the default output (form factor) + mic and applies a recommended preset (`recommendedPresetForOutput`, tested) on finish.

### Phase 20 — Release Validation (runtime / hardware)  `[ ]`
- [ ] ⏵ **Hardware APO validation** on multiple Windows PCs (audio confirmed through `audiodg.exe`)
- [ ] ⏵ **8-hour soak** — fix leaks / CPU spikes / dropouts
- [ ] ⏵ **Competitor benchmark** vs FxSound / Sonar / Boom 3D / Dolby Access / Equalizer APO (CPU, latency, quality)
- [~] **Translations** — fill-in templates generated for ZH-CN/AR/HI/TA (+ ES sample), bundled + documented (docs/TRANSLATIONS.md); RTL flag set for Arabic. Human translation of the values + a live language picker / `tr()` routing remain. ⬜
- [ ] ⏵ **Production code signing**

---

## Veyra 1.0 release target

**Required:** Stable APO ⏵ · RNNoise ✅ · AutoEQ ✅ · Parametric EQ ✅ · KEMAR ✅ · Good presets ✅ · Apps page ✅ · Devices page ✅ · Localization framework ✅ (+ 5 locale templates) · Documentation 🟡 (USER_GUIDE done; screenshots/video pending)
**Nice-to-have:** CIPIC · IRCAM · Equal-loudness · Advanced crossfeed
**Future:** Personalized HRTF · Room simulation · Binaural speaker emulation

---

_Log started 2026-06-14. `FEATURE_COVERAGE.md` is the detailed §1–§16 audit._
