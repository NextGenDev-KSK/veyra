# Changelog

All notable changes to Veyra Sounds are documented here.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Versioning: [Semantic Versioning](https://semver.org/).

---

## [Unreleased]

### Fixed
- **Denormal protection** — Added `_mm_setcsr` (FTZ+DAZ) on the first `APOProcess` call in
  both render and capture APOs via a `denormalsSet_` flag. Without this, decayed filter states
  on x86 CPUs without hardware DAZ/FTZ cause 10–100× CPU overhead during silence. Biquad also
  adds a `1.0e-25f` anti-denormal guard to the y1 feedback state as secondary defence for the
  AudioBridge path (which runs without APO context).
- **Freeverb quality** — Upgraded reverb from 4 comb + 2 allpass per channel to the full
  standard Freeverb complement of 8 comb + 4 allpass with the complete Schroeder tuning set
  (1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 / 556, 441, 341, 225). `kCombMax` raised
  to 3700 to fit the largest tuning at 96 kHz. Reverb tail is now denser and more musical.
- **FeatureExtractor mid-band energy** — Fixed physically incorrect RMS subtraction for the
  Sound Tracker's mid-band energy. Mid-band power is now computed as
  `max(0, totalPow − lowPow − highPow)` in the power domain before `sqrt`. Voice
  classification accuracy improves as a result.
- **INF ExtensionId** — Replaced all-zeros placeholder GUID in `veyra_apo.inf` with
  `{E7C3A8F2-4B6D-4E1F-9D0A-B3C5F7E2A841}`. Windows rejects all-zeros ExtensionIds;
  this was a hard blocker for production APO deployment.
- **Overlay config polling** — Reduced config.json disk poll in the overlay render loop from
  every 15 frames (~0.5 s) to every 300 frames (~10 s), eliminating render stalls on HDDs
  and network-mapped `%APPDATA%` paths.
- **GamerMode toggle layout** — Replaced hardcoded `+50 px` horizontal toggle offset with
  `cellW − toggleWidth − 4` (right-aligned within cell). Label draw width is now computed
  from available space rather than a fixed 48 px. Fixes layout for longer locale strings.

---

## [0.3.0] — DSP + Spatial Complete

### Added
- Full 30-effect DSP chain: EQ, compressor, transient shaper, bass enhancer, harmonic exciter,
  saturation (3 modes, 2× oversampled), stereo widener, multiband width, reverb, room simulator,
  crossfeed, HRTF, field compensation, night mode, equal loudness, loudness normaliser,
  true-peak limiter, spectrum analyser.
- MIT KEMAR HRTF database (full + diffuse sets); ITD-aware interpolation.
- RNNoise denoiser (v0.1.1, MSVC VLA→alloca patch) — PIMPL wrapper for use in capture APO.
- NLMS Acoustic Echo Canceller (256 taps, allocation-free after `prepare()`).
- 16-band Parametric EQ with node editor.
- AutoEQ — 16 oratory1990 headphone profiles included.
- Hearing Test → personalised EQ shaping.
- EBU R128 Loudness Normaliser and ISO-226 Equal Loudness curve.
- True-Peak Limiter: 64-sample lookahead, 4-phase 8-tap windowed-sinc ISP detection.
- Sound Tracker: WASAPI loopback → heuristic classifier → radar HUD overlay.
- VoiceChain: HPF → RNNoise → compressor → de-esser → AGC → presence → output gain.

---

## [0.2.0] — UI + Presets + Themes

### Added
- Full JUCE 8.0.13 UI: Home, Presets, Apps, Devices, Sound Lab, Gamer Mode, Settings.
- 11 built-in themes with live switching; custom accent colour.
- 27 built-in presets with categories, favourites, and curated popular list.
- Mini Mode, global hotkeys, system tray, DWM Acrylic backdrop.
- Onboarding wizard, per-app rule engine, crash capture + recovery banner.
- 8 visualiser modes, Sound Lab (7 diagnostic tools).

---

## [0.1.0] — Service + IPC Backbone

### Added
- Four-binary project: APO DLL, service, UI, overlay.
- Windows service with SCM registration; `--console` for development.
- Named-pipe binary protocol (`VYRA` magic, version field, 12-byte framing).
- Seqlock shared memory for APO parameters.
- Config round-trip (JSON, atomic save), Preset schema, AppRules.
- CI: GitHub Actions, MSVC x64, CMake preset, CTest, portable ZIP artifact.
