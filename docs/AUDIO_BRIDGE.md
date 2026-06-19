# Hearing Veyra on Bluetooth (the AudioBridge path)

Bluetooth outputs (e.g. **OnePlus Nord Buds 3 Pro**) reject endpoint APOs, so the
Tier-B "register the APO on your headphones" route usually won't work for them.
The **AudioBridge** is the alternative: a user-mode loop in `veyra-service` that

```
captures a source endpoint (loopback) -> runs the DspChain -> renders to your headphones
```

So apps must play into a **source** that isn't the Buds, and Veyra renders the
processed result **to** the Buds. That source is a *virtual* playback device.

## What you need

1. **A virtual playback device** (a "sink" apps can target). Two options:
   - **Now:** install a free virtual audio cable (any that creates a render
     endpoint). It shows up as a playback device.
   - **Later:** Veyra ships its own signed virtual device (see
     `drivers/veyra-vad/` — WDK + code-signing, not yet built).
2. The Veyra service running.

## Setup

1. Install the virtual sink and set it as the **Windows default playback device**
   (so Spotify/YouTube play into it). You won't hear anything yet — it has no
   real speakers.
2. Start the service so it logs the endpoint ids:
   ```
   build\windows-release\bin\veyra-service.exe --console
   ```
   It prints lines like:
   ```
   AudioBridge: render endpoint "CABLE Input (VB-Audio...)" = {0.0.0.00000000}.{...}
   AudioBridge: render endpoint "OnePlus Nord Buds 3 Pro Stereo" = {0.0.0.00000000}.{...}
   ```
3. Put those two ids into `%APPDATA%\Veyra\config.json` under `bridge`:
   ```json
   "bridge": {
     "enabled": true,
     "source_device_id": "<the virtual sink id>",
     "target_device_id": "<the Nord Buds id>"
   }
   ```
   (Save — the service applies it live; the bridge thread restarts the session.)
4. Play music. You should hear it on the Buds **with the EQ/knobs/Night
   Mode/crossfeed applied**, and the controls change it in real time.

## Limits (v1)

- Source and target must share the **same sample rate** and be **stereo 32-bit
  float** (shared-mode default); no resampling yet — the service logs and idles
  if they don't match.
- Loopback adds ~10–30 ms latency — fine for music, may lag video lip-sync.
- This is a user-mode bridge; the in-app device picker UI is a follow-up (for
  now the ids are set in `config.json`).

## Why not just the APO?

The APO path (Tier B) is lower latency and the "proper" Windows mechanism, but
it depends on the endpoint accepting a custom post-mix effect — which generic
**Bluetooth** endpoints generally don't. The bridge trades a little latency for
working on *any* output device.
