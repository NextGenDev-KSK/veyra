# Veyra Sounds v1.1.0 — Release Notes

**Release date:** 2026-07-09  
**Platform:** Windows 10 version 2004 (build 19041) or later, 64-bit

---

## What is Veyra Sounds?

Veyra Sounds is a free, open-source, system-wide audio enhancer for Windows.
**30+ DSP effects** including 10-band Graphic EQ, 16-band Parametric EQ, Spatial
HRTF Virtualisation, Compressor, Reverb, Transient Shaper, Bass Enhancer, and a
True-Peak Limiter. Gamer Mode Sound Tracker radar HUD. 27 built-in presets + 16
AutoEQ headphone correction profiles.

On this unsigned open-source release the audio path is the **Audio Bridge**: apps
play into a virtual output device, the Veyra service captures that audio, runs
the DSP chain, and renders the processed sound to your real headphones or
speakers. This is the same architecture FxSound uses (virtual device + user-mode
DSP) and it needs no driver signing. The APO path (in-engine processing at
< 5 ms) exists in the codebase but requires a signed build — see "Known
limitations".

---

## v1.1.0 highlights — the Bridge gets a real UI

- **Audio Bridge controls in the app.** Devices → Audio Bridge: an on/off
  switch, a Capture picker, and a Play to picker. No more editing config.json.
- **Automatic setup.** Turning the Bridge on auto-detects a virtual cable
  (VB-CABLE, Voicemeeter, and similar) for Capture and picks the device you are
  currently listening on for Play to.
- **Veyra keeps the routing alive.** While the Bridge is on, Veyra automatically
  keeps the capture device set as the Windows default output, so apps always
  play into the bridge — even after Windows or an app changes the default.
- **A live status line** on the Devices screen tells you exactly what the Bridge
  is doing, and warns in plain language when the routing cannot work.
- **Feedback protection in the service.** If capture and playback ever resolve
  to the same endpoint, the service refuses the session and idles instead of
  doubling or echoing your audio.
- **Fresh-install checklist.** New [FRESH_INSTALL.md](FRESH_INSTALL.md) walks a
  brand-new Windows 11 machine from first boot to processed audio, including the
  Smart App Control decision you must make before installing anything unsigned.

---

## Download

| File | Description |
|------|-------------|
| `veyra-sounds-setup-1.1.0-x64.exe` | Windows installer (recommended) |
| `veyra-portable-1.1.0-x64.zip` | Portable ZIP (no install required) |
| `SHA256SUMS.txt` | Checksums for the files above |
| Source code | Available on GitHub |

---

## Installation (short version)

1. Download `veyra-sounds-setup-1.1.0-x64.exe` and run it.

> **Windows SmartScreen:** Windows may show a "Windows protected your PC" blue
> screen on first launch. This is expected for a new unsigned open-source
> project. Click **"More info"** → **"Run anyway"**. If the button is missing
> entirely, Smart App Control is enforcing on your machine — see
> [FRESH_INSTALL.md](FRESH_INSTALL.md).

2. Follow the installer, pick your speakers or headphones when asked.
3. Install the free [VB-CABLE](https://vb-audio.com/Cable/) virtual device and
   reboot.
4. Open Veyra → **Devices** → turn **Audio Bridge** on. Play music, toggle an
   effect, and hear the difference.

Full walkthrough for a freshly reinstalled PC: [FRESH_INSTALL.md](FRESH_INSTALL.md).

---

## Code signing status (honest version)

The assets on this page are **unsigned**. We applied to the SignPath Foundation
for a free open-source code-signing certificate and were declined for now — the
Foundation program requires established external signals (stars, articles,
independent discussions) that a young project does not have yet. We will
reapply as the project grows; commercial signing (Certum Open Source or Azure
Trusted Signing) remains on the roadmap. Until one of those lands:

- SmartScreen will warn on first run (Run anyway works).
- Enforced Smart App Control blocks unsigned installers outright (no override).
- The APO path stays dormant (see below); the Audio Bridge is unaffected.

Details and roadmap: [installer/SIGNING.md](installer/SIGNING.md).

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

**Impact:** the installer completes successfully and every playback feature works
through the Audio Bridge. The microphone processing chain (RNNoise, AEC, noise
gate) runs inside the mic APO and therefore also requires a signed build to be
active. Setup guide: [docs/AUDIO_BRIDGE.md](docs/AUDIO_BRIDGE.md).

### Bridge latency

The loopback capture adds roughly 30–80 ms. That is fine for music, films, and
casual gaming; if you need the < 5 ms APO path, it requires a signed build.

### Hardware not yet fully validated

The `HARDWARE_VALIDATION.md` checklist (54 items) has not been completed on
physical hardware. IPolicyConfig (used to keep the default device), sleep/resume
recovery, hot-plug detection, and multi-monitor overlay are verified by code
review only.

### No sleep/resume notification handler in service

The service does not handle `WM_POWERBROADCAST`. After sleep/resume, the
AudioBridge retries (750 ms backoff) and the UI reconnects via the IPC backoff
(max 30 s). No manual recovery needed in most cases.

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

SHA-256 hashes for all release artifacts are published in `SHA256SUMS.txt` on
this release page.

---

## License

GNU General Public License v3.0. See [LICENSE](LICENSE).

Third-party licenses: `third_party/*/LICENSE`.
MIT KEMAR HRTF dataset: non-commercial research license (see `third_party/hrtf/`).
