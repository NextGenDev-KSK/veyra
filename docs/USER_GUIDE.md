# Veyra Sounds — User Guide

A free, system-wide audio enhancer for Windows. This guide covers what each
screen does and how to use it. For building/running see [VERIFY.md](../VERIFY.md)
and [BUILD_GUIDE.md](../BUILD_GUIDE.md); for the architecture see
[ARCHITECTURE.md](../ARCHITECTURE.md).

> Screenshots / demo videos: capture these from a real run (the layout is the
> fixed 1200×675 canvas). The reference design lives in `Reference theme/`.

## Getting sound through Veyra

Veyra's primary audio design is the **APO** — a Windows Audio Processing Object
that loads directly into `audiodg.exe` alongside Dolby Atmos and DTS. Once
associated with your output endpoint, every app's audio passes through Veyra
automatically at < 5 ms latency. No virtual cable, no rerouting, no change to
your Windows default output device.

> **On this unsigned open-source release the APO does not load** — Windows
> only loads code-signed APOs into `audiodg.exe` (see the
> [Release Notes known limitations](../RELEASE_NOTES.md#known-limitations)).
> Until a signed release ships, use the **Audio Bridge** below to hear the
> effects. Both paths run the identical DSP chain.

**Getting started:**
1. Install Veyra using the [installer](https://github.com/NextGenDev-KSK/veyra/releases).
   It picks your playback device and configures the APO association
   automatically (active once a signed build is installed).
2. Launch Veyra — the brand LED turns green when the app is connected to the
   Veyra service.
3. To hear the effects on this release: set up the **Audio Bridge** (below).
4. To change the device later: **Devices → Preferred Output**.

See [INSTALLATION.md](../INSTALLATION.md) for the full installation guide.

**Audio Bridge (the working path on this release; required for Bluetooth):**  
The Bridge provides a WASAPI loopback path: apps play into a virtual device,
Veyra processes the audio in its service, and renders it to your headphones.
Since v1.1.0 it is configured entirely from **Devices → Audio Bridge**: one
switch plus Capture and Play to pickers, with a status line that validates the
routing. Install the free VB-CABLE virtual device first for the cleanest
whole-system capture ([AUDIO_BRIDGE.md](AUDIO_BRIDGE.md) has the details). The
Bridge adds ~30–80 ms latency, fine for music and video; on a signed build the
APO path is the better choice for competitive gaming. Bluetooth A2DP endpoints
never host custom APOs, so the Bridge is the permanent path for Bluetooth
regardless of signing.

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
Output + input **device cards** showing each endpoint's name, form-factor badge,
and status (the active output shows the current preset + volume; the active input
shows its mic profile). Below the cards sits the **Audio Bridge** card: an on/off
switch, a **Capture** picker (the virtual device apps play into — Veyra
auto-detects VB-CABLE and similar), a **Play to** picker (your real headphones or
speakers), and a status line that says in plain language whether the routing is
live or what is wrong with it. While the Bridge is on, Veyra keeps the capture
device set as the Windows default output automatically. The **MIC BRIDGE**
section (v1.2.0) routes your real microphone through the voice chain (RNNoise
denoiser, compressor, de-esser, AGC) into a second virtual cable that apps use
as their mic — its own switch, **Microphone** and **Sends to** pickers, and
status line. A **Get VB-CABLE** button appears whenever routing needs a cable
that isn't installed. The **Preferred Output** picker at the bottom belongs to
the signed-APO path and applies only while the Bridge is off.

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
`%ProgramData%\Veyra\` — `config.json`, `presets\`, `app_rules.json`, `logs\`,
`crashes\`. Accessible by both the service (`NT AUTHORITY\LocalService`) and the
interactive user. Veyra never sends anything without consent.
