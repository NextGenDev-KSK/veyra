# Fresh PC Install Checklist

Step-by-step from a brand-new (or freshly reinstalled) Windows 11 machine to
working, processed audio. Follow it in order — step 1 is the one people miss,
and it is the only one that can permanently lock you out of installing Veyra.

---

## 0. Before wiping your old machine

- Copy `%ProgramData%\Veyra\` somewhere safe (USB stick, cloud). It contains
  your `config.json`, custom presets, and app rules. Restore it to the same
  location after step 3 and everything comes back.

---

## 1. First boot: decide about Smart App Control (do this FIRST)

Windows 11 ships with **Smart App Control (SAC)** in *evaluation mode* on fresh
installs, and after a while it silently switches itself **on**. When SAC is
enforcing, unsigned programs are **blocked with no "Run anyway" button** — that
includes the Veyra installer and every other unsigned open-source app.

Right after Windows setup finishes:

1. Open **Windows Security** → **App & browser control** → **Smart App Control**.
2. If it shows *Evaluation* or *On* and you intend to use unsigned open-source
   software, set it to **Off**.

> **This is a one-way switch.** Once SAC is off it cannot be turned back on
> without reinstalling Windows — that is a Windows rule, not ours. If you would
> rather keep SAC on, you cannot run the current unsigned Veyra build; wait for
> a signed release instead.

---

## 2. Download Veyra and get past SmartScreen

1. Download `veyra-sounds-setup-1.1.0-x64.exe` from the
   [Releases page](https://github.com/NextGenDev-KSK/veyra/releases/latest).
2. Optional but recommended — verify the checksum matches `SHA256SUMS.txt`
   from the same release:
   ```
   certutil -hashfile veyra-sounds-setup-1.1.0-x64.exe SHA256
   ```
3. Run the installer. When SmartScreen shows "Windows protected your PC",
   click **More info** → **Run anyway**. This warning is normal for a new
   unsigned open-source project.

   *If there is no "More info" link and Windows says an App Control policy
   blocked the file, SAC is still enforcing — go back to step 1.*

---

## 3. Install Veyra

Follow the installer (Next → I Accept → Install). It automatically:

- installs and starts the **Veyra Audio Service**,
- installs the VC++ runtime,
- registers the APO COM server (dormant until a signed build exists — expected),
- asks you to pick your playback device by name,
- creates Start Menu / optional desktop shortcuts.

If you backed up `%ProgramData%\Veyra\` in step 0, restore it now and restart
the service (`services.msc` → Veyra Audio Service → Restart) or reboot.

---

## 4. Install VB-CABLE (the virtual device Veyra listens to)

Veyra's Audio Bridge needs a virtual output device that apps can play into.
VB-CABLE is free, its driver is properly signed (installs clean on any
Windows 11), and we cannot legally bundle it — so it is one extra download:

1. Get it from [vb-audio.com/Cable](https://vb-audio.com/Cable/).
2. Extract the ZIP, right-click `VBCABLE_Setup_x64.exe` → **Run as
   administrator** → **Install Driver**.
3. Reboot when it asks.

---

## 5. Turn the Audio Bridge on

1. Open Veyra → **Devices**.
2. Switch **Audio Bridge** on.
3. Veyra auto-detects the cable: **Capture** = `CABLE Input`, **Play to** = the
   device you were just listening on. Adjust if needed.
4. The status line goes green: *"Live: CABLE Input to \<your device\>. Veyra
   keeps the capture device set as the Windows default."* From now on Veyra
   holds that routing for you — you never have to touch the Windows default
   output yourself.

---

## 6. Verify you're done (two minutes)

- Play any music.
- In Veyra, drag a couple of EQ bands or toggle **Bass Boost** — you should
  hear the change immediately.
- The Home screen visualizer should be moving.
- Tray icon present; `services.msc` shows **Veyra Audio Service: Running**.

If any of that fails, check the status line on **Devices** first (it names the
problem), then `%ProgramData%\Veyra\logs\veyra-service.log`, then
[TROUBLESHOOTING.md](TROUBLESHOOTING.md).

---

## What works on this unsigned build (so there are no surprises)

| Feature | Status |
|---|---|
| All playback DSP (EQ, spatial, reverb, limiter, presets, AutoEQ) | ✅ via Audio Bridge |
| Bluetooth headphones/earbuds | ✅ via Audio Bridge (the only path that ever works for A2DP) |
| Microphone chain (RNNoise denoiser, compressor, de-esser, AGC) | ✅ via Mic Bridge (v1.2.0; needs a second virtual cable) |
| Sound Tracker HUD, Gamer Mode, visualizers, hotkeys, themes | ✅ |
| Latency | ~10–30 ms (event-driven Bridge). The < 5 ms APO path needs a signed build |

### Optional step 7: clean microphone (Mic Bridge)

1. Install a **second** virtual cable — VB-Audio's
   [CABLE A](https://vb-audio.com/Cable/) (the base VB-CABLE already carries
   your playback).
2. Veyra → **Devices** → switch **MIC BRIDGE** on. Veyra picks your default
   microphone and the free cable automatically.
3. In Discord/OBS/your game, select that cable's output (e.g. `CABLE-A Output`)
   as the microphone. Your voice arrives denoised and levelled.

Signing status and roadmap: [installer/SIGNING.md](installer/SIGNING.md).
