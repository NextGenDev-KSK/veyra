# Veyra Sounds

[![build](https://github.com/NextGenDev-KSK/veyra/actions/workflows/build.yml/badge.svg)](https://github.com/NextGenDev-KSK/veyra/actions/workflows/build.yml)
[![license](https://img.shields.io/badge/license-GPLv3-blue.svg)](LICENSE)
[![platform](https://img.shields.io/badge/platform-Windows%2010%2F11%20x64-lightgrey.svg)]()

**Veyra Sounds** is a free, open-source, system-wide audio enhancer for Windows 10/11 (x64).
It gives you realtime EQ, dynamics, spatial audio, voice processing, a gamer Sound Tracker HUD,
and a 30+ effect DSP chain. Its primary design routes audio through a Windows Audio Processing
Object (APO) — the same in-driver hook used by Dolby Atmos and DTS — at under 5 ms latency.
Licensed under the **GNU GPLv3**.

> **Note on audio output:** the APO path loads only on a **code-signed** build.
> This open-source release is unsigned, so the identical DSP runs through the
> built-in **Audio Bridge** instead (no signing required, and the only option for
> Bluetooth). See [Known limitations](RELEASE_NOTES.md#known-limitations).

Maintainer: **Krithik S** ([@NextGenDev-KSK](https://github.com/NextGenDev-KSK))

---

## Features

### Audio Engine
- **10-band Graphic EQ** and **16-band Parametric EQ** with a node editor
- **Bass Boost**, **Treble Shelf**, **Stereo Widener**, **Multiband Width**
- **Compressor**, **Transient Shaper**, **Harmonic Exciter**
- **Saturation** (transparent / tape / tube) with 2× oversampling
- **Adaptive Bass Enhancer**, **Headphone Safe** mode
- **True-Peak Limiter** — 64-sample lookahead, 4× polyphase ISP detection (BS.1770-style)
- **EBU R128 Loudness Normaliser** and **ISO-226 Equal Loudness** curve
- **Night Mode**, **Reference Mode** (flat A/B bypass for critical listening)

### Spatial Audio
- **KEMAR HRTF Virtualisation** — MIT measured HRIRs, ITD-aware interpolation
- **Bauer/Meier Crossfeed** with frequency-dependent compensation
- **Freeverb Room Reverb** — full 8 comb + 4 allpass per channel (Schroeder tuning)
- **Room Simulator** — 6-tap early reflection network
- **Field Compensation** — free-field / diffuse-field / custom

### Voice & Microphone
- **RNNoise** ML denoiser (v0.1.1), **NLMS Acoustic Echo Canceller**
- **Noise Gate**, **Auto Gain Control** (−16 LUFS), **De-esser**, **Presence EQ**

### Gamer Features
- **Sound Tracker HUD** — anti-cheat-safe layered overlay, footstep / gunshot / voice detection
- **Directional radar** with 3 styles (competitive / rich / compass strip)
- **Game auto-detection** by process name, **per-app rule engine**

### Application
- **11 themes** with live switching and custom accent colour
- **27 built-in presets** + user preset library with categories and favourites
- **16 AutoEQ profiles** (oratory1990 format), **Hearing Test** personalisation
- **Sound Lab** — 7 diagnostic tools (sweep, noise, polarity, 10-octave meter, hearing test)
- **8 visualiser modes**, **Mini Mode**, **global hotkeys**, **system tray**
- **DWM Acrylic backdrop** (Windows 11 native glass), **MSIX + portable ZIP** packaging

---

## Architecture

```
audiodg.exe (SYSTEM)              veyra-service.exe
┌──────────────────────┐          ┌────────────────────────────────────┐
│ VeyraApoEfx          │◄────────►│ ApoPublisher → SharedMemory        │
│ DspChain (30+ fx)    │ seqlock  │ ControlServer (\\.\pipe\veyra-ctrl)│
│ VeyraMicApo          │          │ TrackerService (WASAPI loopback)    │
│ VoiceChain + RNNoise │          │ AudioBridge, Presets, Updater, ...  │
└──────────────────────┘          └────────────────────────────────────┘
                                               ▲ named pipes
                                               ▼
                                  veyra.exe (JUCE 8 UI)
                                  veyra-overlay.exe (GDI+ HUD)
```

Parameters flow service → APO via lock-free seqlock shared memory.
Analyzer and tracker events flow via SPSC ring buffers.
See [ARCHITECTURE.md](ARCHITECTURE.md) for the full data-flow diagram.

---

## Installing

Download `veyra-sounds-setup-x.y.z-x64.exe` from
[Releases](https://github.com/NextGenDev-KSK/veyra/releases) and double-click it.

> **Windows SmartScreen:** You may see a "Windows protected your PC" screen.
> Click **More info → Run anyway**. This is normal for a new unsigned open-source
> project. See [installer/SIGNING.md](installer/SIGNING.md) for the signing roadmap.

The installer automatically:
- Registers the APO COM server
- Installs and starts the Windows service
- Lets you pick your playback device (by friendly name — no GUIDs)
- Associates the APO with that device and restarts Windows Audio
- Falls back gracefully to Audio Bridge mode for Bluetooth headphones

**→ Full guide: [INSTALLATION.md](INSTALLATION.md)**

---

## Building from source

**Requirements:** Visual Studio 2022 with "Desktop development with C++" workload
(includes MSVC, Windows SDK 10.0.22000+, CMake 3.25+, Ninja).

```powershell
# In Developer PowerShell for VS 2022:
git clone https://github.com/NextGenDev-KSK/veyra.git
cd veyra
git submodule update --init --recursive
cmake --preset windows-release
cmake --build --preset windows-release
```

Outputs: `build\windows-release\bin\` → `veyra.exe`, `veyra-service.exe`,
`veyra-apo.dll`, `veyra-overlay.exe`

### Running in development mode (no admin needed)

```powershell
# Terminal 1:
.\build\windows-release\bin\veyra-service.exe --console

# Terminal 2:
.\build\windows-release\bin\veyra.exe
```

Config and logs: `%ProgramData%\Veyra\` (`config.json`, `logs\`, `crashes\`)

### Running tests

```powershell
cmake --preset windows-release -DVEYRA_BUILD_TESTS=ON
cmake --build --preset windows-release
ctest --test-dir build\windows-release --output-on-failure
```

See [BUILD_GUIDE.md](BUILD_GUIDE.md) for APO test-signing, driver scripts, and
building the installer.

---

## DSP chain order

```
Input → Balance/Mono → Graphic EQ → Parametric EQ → Tone Shelves
      → Compressor → Transient → Bass Enhancer → Headphone Safe
      → Equal Loudness → Exciter → Saturator → Multiband Width
      → Stereo Width → Reverb → HRTF Surround → Room Simulator
      → Crossfeed → Field Compensation → Night Mode
      → Volume → Loudness Normaliser → True-Peak Limiter → Analyser → Output
```

---

## Project structure

```
apps/veyra-apo/        ← APO COM DLL (audiodg.exe host)
apps/veyra-service/    ← Orchestrator service
apps/veyra-ui/         ← JUCE 8 UI application
apps/veyra-overlay/    ← GDI+ layered window HUD
modules/veyra-common/  ← Config, IPC, presets, shared memory, localisation
modules/veyra-dsp/     ← Header-only DSP chain (30+ effects, allocation-free)
modules/veyra-rnnoise/ ← RNNoise denoiser PIMPL wrapper
resources/autoeq/      ← 16 oratory1990 headphone correction files
resources/lang/        ← Localisation templates
resources/themes/      ← Design token JSON for 11 themes
tests/                 ← Catch2 unit + performance tests
installer/driver/      ← APO developer scripts (register, associate, uninstall)
installer/setup/       ← NSIS production installer + build scripts
installer/msix/        ← MSIX manifest
```

---

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md). All contributions are GPLv3.

## License

GNU General Public License v3.0 — see [LICENSE](LICENSE).

Third-party licences: `third_party/*/LICENSE`.
MIT KEMAR HRTF dataset: `third_party/hrtf/mit_kemar/LICENSE` (non-commercial research licence).
