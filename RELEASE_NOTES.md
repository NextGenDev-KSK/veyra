# Veyra Sounds v1.2.0 — Release Notes

**Release date:** 2026-07-11  
**Platform:** Windows 10 version 2004 (build 19041) or later, 64-bit

---

## What is Veyra Sounds?

Veyra Sounds is a free, open-source, system-wide audio enhancer for Windows.
**30+ DSP effects** including 10-band Graphic EQ, 16-band Parametric EQ, Spatial
HRTF Virtualisation, Compressor, Reverb, Transient Shaper, Bass Enhancer, and a
True-Peak Limiter. Gamer Mode Sound Tracker radar HUD. 27 built-in presets + 16
AutoEQ headphone correction profiles.

On this unsigned open-source release the audio path is the **Audio Bridge**:
apps play into a virtual output device, the Veyra service captures that audio,
runs the DSP chain, and renders the processed sound to your real headphones or
speakers — the same architecture FxSound uses, no driver signing needed.

---

## v1.2.0 highlights — your microphone joins the party

- **Mic Bridge: the voice chain now works unsigned.** The RNNoise machine-
  learning denoiser, leveling compressor, de-esser, AGC and presence EQ used to
  live only inside the mic APO, which needs a signed build. Now the service can
  capture your microphone, clean it, and deliver it into a second virtual
  cable — Discord, OBS or any game just selects that cable's output as its
  microphone. Devices → MIC BRIDGE: one switch, two pickers, done.
- **Much lower Bridge latency.** The playback bridge streams event-driven on a
  Pro Audio priority thread; when sample rates line up it opens the render
  stream at the Windows audio engine's minimum period (typically ~3 ms buffers
  instead of the old 100 ms polling). Casual gaming on the Bridge is now
  entirely reasonable.
- **Self-healing audio.** The service watches device arrivals/removals and
  power events: unplug your headphones, plug them back, install a cable, or
  wake the laptop from sleep — the bridges reconnect immediately instead of
  waiting out retry timers.
- **Surround sources just work.** 5.1/7.1 (and mono/quad) sources downmix to
  stereo properly instead of the bridge idling.
- **"Get VB-CABLE" button** appears right on the Devices card when routing
  needs a cable that isn't installed yet.

Full details in the [CHANGELOG](CHANGELOG.md).

---

## Download

| File | Description |
|------|-------------|
| `veyra-sounds-setup-1.2.0-x64.exe` | Windows installer (recommended) |
| `veyra-portable-1.2.0-x64.zip` | Portable ZIP (no install required) |
| `SHA256SUMS.txt` | Checksums for the files above |
| Source code | Available on GitHub |

---

## Installation (short version)

1. Download `veyra-sounds-setup-1.2.0-x64.exe` and run it.

> **Windows SmartScreen:** Windows may show a "Windows protected your PC" blue
> screen on first launch. This is expected for a new unsigned open-source
> project. Click **"More info"** → **"Run anyway"**. If the button is missing
> entirely, Smart App Control is enforcing on your machine — see
> [FRESH_INSTALL.md](FRESH_INSTALL.md).

2. Follow the installer, pick your speakers or headphones when asked.
3. Install the free [VB-CABLE](https://vb-audio.com/Cable/) virtual device and
   reboot.
4. Open Veyra → **Devices** → turn **Audio Bridge** on. Play music, toggle an
   effect, hear the difference.
5. Optional, for the clean mic: install a **second** cable (e.g. VB-Audio
   "CABLE A"), turn **Mic Bridge** on, and select that cable's output as the
   microphone in your apps.

Full walkthrough for a freshly reinstalled PC: [FRESH_INSTALL.md](FRESH_INSTALL.md).
Bridge and Mic Bridge details: [docs/AUDIO_BRIDGE.md](docs/AUDIO_BRIDGE.md).

---

## Code signing status (honest version)

The assets on this page are **unsigned**. Our SignPath Foundation application
was declined until the project has more public traction; we will reapply as it
grows. Until a signing route lands:

- SmartScreen will warn on first run (Run anyway works).
- Enforced Smart App Control blocks unsigned installers outright (no override).
- The APO path (in-engine processing at < 5 ms) stays dormant; both bridges are
  unaffected.

Details and roadmap: [installer/SIGNING.md](installer/SIGNING.md).

---

## Known limitations

### APO on unsigned builds

`audiodg.exe` (the Windows audio engine that hosts APOs) runs as a protected
process and loads only **digitally signed** APO DLLs. This build is unsigned,
so the APO does not load on a stock consumer machine. This is a signing
requirement, not a test-signing switch: the `DisableProtectedAudioDG` override
no longer works on current Windows 11 (verified on build 26200.8655). Every
playback feature works through the Audio Bridge, and as of this release the
voice chain works through the Mic Bridge.

### Bridge latency

Event-driven streaming brings the playback bridge to roughly 10–30 ms in the
common case (down from 30–80 ms), and ~3 ms render buffers when sample rates
match. The < 5 ms in-engine APO path still requires a signed build. The Mic
Bridge adds a similar amount on the voice path — fine for calls and streaming.

### Two cables for two bridges

The playback bridge and the mic bridge each need their own virtual cable. With
just VB-CABLE installed you can run either one; add a second cable (e.g.
VB-Audio's CABLE A) to run both at once. The Devices card warns if you point
both at the same cable.

### Hardware not yet fully validated

The `HARDWARE_VALIDATION.md` checklist has not been completed on physical
hardware. IPolicyConfig (default-device keeper), hot-plug and sleep/resume
paths are implemented and code-reviewed; broad hardware validation is ongoing.

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

See [BUILD_GUIDE.md](BUILD_GUIDE.md) for full instructions.

---

## Checksums

SHA-256 hashes for all release artifacts are published in `SHA256SUMS.txt` on
this release page.

---

## License

GNU General Public License v3.0. See [LICENSE](LICENSE).

Third-party licenses: `third_party/*/LICENSE`.
MIT KEMAR HRTF dataset: non-commercial research license (see `third_party/hrtf/`).
