# Veyra Sounds v1.0.0 — Release Notes

**Release date:** 2026-06-30  
**Platform:** Windows 10 version 2004 (build 19041) or later, 64-bit

---

## What is Veyra Sounds?

Veyra Sounds is a free, open-source, system-wide audio enhancer for Windows. Its
primary design is a Windows APO (Audio Processing Object) — the same in-driver
hook used by Dolby Atmos and DTS. The APO path requires a signed build (see
"Known limitations"); this open-source release is unsigned, so for now the same
DSP runs through the **Audio Bridge**, which processes audio in the service and
needs no signing. Both paths share one DSP engine.

**30+ DSP effects** including 10-band Graphic EQ, 16-band Parametric EQ, Spatial
HRTF Virtualisation, Compressor, Reverb, Transient Shaper, Bass Enhancer, and a
True-Peak Limiter. Full microphone processing chain with RNNoise denoiser, AEC,
and Noise Gate. Gamer Mode Sound Tracker radar HUD. 27 built-in presets + 16
AutoEQ headphone correction profiles.

---

## Download

| File | Description |
|------|-------------|
| `veyra-sounds-setup-1.0.0-x64.exe` | Windows installer (recommended) |
| `veyra-portable-1.0.0-x64.zip` | Portable ZIP (no install required) |
| `SHA256SUMS.txt` | Checksums for the files above |
| Source code | Available on GitHub |

---

## Code signing

Free code signing for Veyra Sounds is provided by the
[**SignPath Foundation**](https://signpath.org), a non-profit that grants
code-signing certificates to open-source projects. Signed Windows binaries are
rolled out as the certificate is provisioned; until then the assets on this
page are unsigned and Windows SmartScreen / Smart App Control may warn or block
them (see "Known limitations").

---

## Installation

1. Download `veyra-sounds-setup-1.0.0-x64.exe`
2. Double-click it

> **Windows SmartScreen:** Windows may show a "Windows protected your PC" blue
> screen on first launch. This is expected for a new unsigned open-source project.
> Click **"More info"** → **"Run anyway"** to proceed. This warning will disappear
> as more users install Veyra and SmartScreen builds its reputation.

3. Follow the installer: Next → I Accept → Next → Install
4. Pick your speakers or headphones from the device list
5. Click Finish → optionally launch Veyra now

The installer handles everything automatically: APO COM registration, Windows
Service installation, device association, and startup configuration.

---

## What's in v1.0.0

### New and improved in this release

- **Zero-PowerShell installer** — `VeyraSetupHelper.exe` (static C++, no VC++
  dependency) handles all audio operations. No PowerShell window ever opens during
  install or uninstall.
- **Native device picker** — endpoint friendly names, never GUIDs. Powered by
  `IMMDeviceEnumerator` in a native C++ helper, not a script.
- **Service crash recovery** — `VeyraAudioService` now auto-restarts via SCM
  failure actions (5 s / 10 s / 60 s). Previously the service stayed stopped on
  any unexpected exit.
- **Upgrade detection** — installing over an older version skips the device picker
  and preserves all settings, presets, and app rules.
- **Complete uninstaller** — removes all APO associations, COM registration,
  service, shortcuts, and registry entries. Prompts whether to keep presets.
- **Desktop shortcut is now truly optional** — was incorrectly force-checked on
  every installer interaction; now uses `.onInit` for a one-time pre-check only.

### Audio engine (CI-verified)

- 10-band Graphic EQ + 16-band Parametric EQ with draggable node editor
- KEMAR HRTF Virtualisation (measured MIT KEMAR set, ITD-aware interpolation)
- Freeverb Room Reverb, Bauer/Meier Crossfeed, Room Simulator
- Compressor, Transient Shaper, Harmonic Exciter, Saturation (transparent/tape/tube)
- True-Peak Limiter (BS.1770, 4× polyphase ISP, 64-sample lookahead)
- EBU R128 Loudness Normaliser, ISO-226 Equal Loudness curve
- RNNoise ML denoiser, NLMS AEC, Noise Gate, AGC, De-esser
- Adaptive Bass Enhancer, Headphone Safe mode, Multiband Stereo Width

### Application

- 11 themes with live switching and DWM acrylic backdrop (Windows 11)
- 27 built-in presets + 16 AutoEQ headphone correction profiles (oratory1990)
- Sound Lab: 7 diagnostic tools including hearing-test-based EQ personalization
- Gamer Mode: anti-cheat-safe layered overlay, Sound Tracker radar HUD
- Per-app rule engine, game auto-detection
- 8 visualizer modes, Mini Mode, global hotkeys, system tray

---

## Known limitations

### APO on unsigned builds

`audiodg.exe` (the Windows audio engine that hosts APOs) runs as a protected
process and loads only **digitally signed** APO DLLs. Because this open-source
build is unsigned, the APO (`veyra-apo.dll`) will not load into `audiodg.exe` on
a stock consumer machine, and audio is not modified through the APO path.

**This is a signing requirement, not merely a test-signing switch.** Enabling
test-signing (`bcdedit /set testsigning on`) is *not* sufficient on its own —
test-signing lets Windows accept a *test-certificate-signed* binary, but the DLL
still has to be signed with a certificate trusted by the machine. An unsigned DLL
never loads regardless of the test-signing state.

The older developer override `DisableProtectedAudioDG` (which used to run
`audiodg` unprotected so unsigned APOs could load) **no longer disables the
protection on recent Windows 11 builds.** This was verified on Windows 11 build
26200.8655 (2026-07-01): with the flag set and after a full reboot, `audiodg`
still started protected and refused the unsigned DLL. Do not rely on this flag.

**Impact:** The installer completes successfully and every other feature works.
Audible processing through the APO path requires a signed build (see the signing
roadmap). For the current open-source release, use the Audio Bridge below.

**Audio Bridge — the supported no-signing path.** The Audio Bridge runs the
identical DSP chain in the service by loopback-capturing a source device and
rendering the processed audio to your output device. It needs no driver signing
and is the recommended path for this release — it is also the only working
option for Bluetooth A2DP endpoints, which never host custom APOs. In v1.0.0
the Bridge is configured via `%ProgramData%\Veyra\config.json` (UI controls for
it return post-1.0); [docs/AUDIO_BRIDGE.md](docs/AUDIO_BRIDGE.md) has the
step-by-step setup.

See [installer/SIGNING.md](installer/SIGNING.md) for the signing roadmap.

### Hardware not yet validated

The `HARDWARE_VALIDATION.md` checklist (54 items) has not been completed on
physical hardware. IPolicyConfig (used for Preferred Output), sleep/resume
recovery, hot-plug detection, and multi-monitor overlay are verified by code
review only. Hardware validation is planned for post-release.

### No sleep/resume notification handler in service

The service does not handle `WM_POWERBROADCAST`. After sleep/resume, the
AudioBridge will retry (750 ms backoff) and the UI will reconnect via the IPC
backoff (max 30 s). The APO reloads automatically through the Windows audio
engine. No manual recovery needed in most cases.

---

## Security notes

- Named pipe DACL: `D:(A;;FA;;;SY)(A;;FA;;;BA)(A;;GRGW;;;IU)` — Interactive Users
  R+W only; non-console sessions rejected
- Shared memory DACL: Everyone read-only; System + Admins full
- Service account: `NT AUTHORITY\LocalService` (reduced privilege)
- Config saved atomically (temp file + rename — crash-safe)
- IPC payload capped at 4 MiB

---

## Building from source

```powershell
git clone https://github.com/NextGenDev-KSK/veyra.git
cd veyra
git submodule update --init --recursive
cmake --preset windows-release
cmake --build --preset windows-release
```

See [BUILD_GUIDE.md](BUILD_GUIDE.md) for full instructions including APO
test-signing and installer build steps.

---

## Checksums

SHA-256 hashes for all release artifacts will be published alongside this release
on the GitHub Releases page.

---

## License

GNU General Public License v3.0. See [LICENSE](LICENSE).

Third-party licenses: `third_party/*/LICENSE`.
MIT KEMAR HRTF dataset: non-commercial research license (see `third_party/hrtf/`).
