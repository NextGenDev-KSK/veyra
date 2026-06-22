# Veyra Sounds — User Guide

A free, system-wide audio enhancer for Windows. This guide covers what each
screen does and how to use it. For building/running see [VERIFY.md](../VERIFY.md)
and [BUILD_GUIDE.md](../BUILD_GUIDE.md); for the architecture see
[ARCHITECTURE.md](../ARCHITECTURE.md).

> Screenshots / demo videos: capture these from a real run (the layout is the
> fixed 1200×675 canvas). The reference design lives in `Reference theme/`.

## Getting sound through Veyra

Veyra processes audio two ways:

- **APO (system-wide, lowest latency)** — registers into the Windows audio engine
  (`audiodg.exe`). Needs the driver package + test-signing/admin (see
  BUILD_GUIDE §2). Once associated with your output endpoint, *everything* is
  processed.
- **Audio Bridge (no driver)** — Devices → Audio Bridge captures a source endpoint
  (loopback), runs the DSP, and renders to a target endpoint (incl. Bluetooth).
  Pick **Source** = your default playback, **Output** = your headphones, toggle
  **Enable**. No admin needed.

The **service** (`veyra-service.exe`) holds the canonical config and drives the
DSP; the **app** (`veyra.exe`) is the UI. The brand LED is green when connected.

## Screens

### Home
Master enable + volume + the live **active-preset** chip in the top bar. A live
spectrum **visualizer** (8 modes: Bars / Monstercat / Circular / Waveform /
Particle / Wavy / Ferrofluid / 3D — pick top-right, or fullscreen). The **10-band
EQ** with a **Graphic ↔ Parametric** toggle, six effect knobs (Bass, Treble,
Volume, Width, Reverb, Compression), and an **ⓘ** hint on each feature (hover for
what it does + shortcut).

### Equalizer (on Home)
- **Graphic**: 10 fixed bands.
- **Parametric**: a draggable node editor — drag a node to set frequency + gain,
  mouse-wheel for Q, right-click for filter type (bell/shelf/notch/HP/LP),
  double-click to reset a node, double-click empty to add (up to 16). The curve
  is the summed biquad response.
- **AutoEQ**: the **AutoEQ** button → pick your headphone model → its measured
  correction loads into the Parametric EQ (16 popular models bundled offline).

### Presets
Category column (Gaming / Music / Movies / Streaming / Voice / Night / Custom),
oval search, sort (A–Z / Category), grid/list, **Favorites** (★, persisted),
**Recently Used**, plus apply / save / duplicate / import / export. 27 built-ins.

### Apps
Per-app rules table (App · Detection · Preset · Volume · Auto-mute · Status) +
**Per-App Switching** master toggle. **+ Add App** opens an offline picker built
from your Start-Menu apps with their real EXE icons. When an app takes focus its
rule applies automatically.

### Devices
Output + input **device cards** (name, form-factor badge, status; the active
output shows the preset + volume, the active input its mic profile) over the
**Audio Bridge** router (the no-driver path above).

### Sound Lab
Seven calibration tools as a tab bar — Speaker Test, 7.1 Surround, Microphone,
Frequency Sweep, Hearing Range, Polarity/Phase, Noise Generator — each emitting
real test signals (and a live mic-input meter).

### Gamer Mode
A 2×2 dashboard: **Sound Tracker** (loopback → classify footsteps/gunshots/voice
→ overlay radar; auto-activates for known games), **Spatial Audio** (HRTF
Cinematic/Competitive, virtual headset, crossfeed; measured MIT KEMAR by
default), **Voice & Microphone** (RNNoise + gate + AEC + AGC + de-esser +
side-tone), and **Night Mode**.

### Settings
Sub-nav: Appearance (11 themes, opacity, reduce-motion), Audio Engine (sample
rate / buffer / latency / hardware accel + **Reference Mode** — a flat A/B that
bypasses all coloration), Microphone, Spatial, Loudness (Night Mode, Sleep
Timer, Loudness Match, **Equal Loudness**), **Sound Quality** (Harmonic Exciter,
Saturation + mode, Multiband Width, Transient Punch), Updates (version + GitHub
releases), About (version/build, diagnostics, open logs, reset). Language packs
live in `resources/lang` (see [TRANSLATIONS.md](TRANSLATIONS.md)).

### Sound Quality (advanced)
Beyond the six Home knobs, Settings → Sound Quality adds: **Harmonic Exciter**
(high-band tanh harmonics for air/presence), **Saturation** (transparent / tape /
tube voicings for warmth), **Multiband Width** (mono lows + wider highs), and
**Transient Punch** (attack emphasis). All are subtle by design and run in both
the system-wide APO and the no-driver Audio Bridge paths.

## Mic processing
RNNoise is the default suppressor (custom expander is the fallback), then noise
gate → AEC → leveling/AGC (−16 LUFS) → de-esser → presence → side-tone.

## Where state lives
`%APPDATA%\Veyra\` — `config.json`, `presets\`, `app_rules.json`, `logs\`,
`crashes\`. Veyra never sends anything without consent.
