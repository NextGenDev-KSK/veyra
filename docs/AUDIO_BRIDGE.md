# Audio Bridge — the no-signing audio path

> **On the current unsigned open-source release, the Bridge is how you hear
> Veyra's effects.** The APO is the primary *design* — it loads directly into
> the Windows audio engine with < 5 ms latency — but Windows only loads
> code-signed APOs, so it stays inactive until a signed build ships (see the
> [Release Notes known limitations](../RELEASE_NOTES.md#known-limitations)).
> The Bridge runs the identical DSP chain inside the Veyra service.

## When to use the Audio Bridge

- **Any endpoint, on this unsigned release** — the APO does not load, so the
  Bridge is the working path.
- **Bluetooth endpoints, always** (e.g. **OnePlus Nord Buds 3 Pro**, AirPods,
  most TWS devices) — Bluetooth A2DP never hosts custom endpoint APOs, so the
  Bridge remains the Bluetooth path even on future signed builds.

The bridge works as follows:

```
Apps → virtual sink (loopback source) → DspChain → Bluetooth headphones
```

Apps must play into a virtual source endpoint; Veyra processes and renders to the Buds.

## What you need

1. **A virtual playback device** — a render endpoint apps can target as their
   output while the bridge captures it.
   - Free option: install **VB-CABLE** (vb-audio.com/Cable) — one installer,
     one reboot — adds `CABLE Input` (playback) and `CABLE Output` (capture).
   - Future option: Veyra will ship its own signed virtual device (`drivers/veyra-vad/`).
2. The Veyra service running.

## Setup (v1.1.0 — all in the app)

1. Install **VB-CABLE** from [vb-audio.com/Cable](https://vb-audio.com/Cable/)
   and reboot when its installer asks.
2. Open Veyra → **Devices** → turn **Audio Bridge** on. Veyra auto-detects the
   cable and preselects **Capture** = `CABLE Input` and **Play to** = the device
   you are listening on right now. Adjust the two pickers if you want different
   routing.
3. That's it. While the Bridge is on, Veyra automatically keeps the capture
   device set as the Windows default output, so every app plays into the cable
   and you hear the processed result on your real device. The status line on the
   card tells you exactly what is happening (and turns amber/red if the routing
   can't work).
4. Play music and toggle any effect to confirm the DSP is live.

> **Capture and Play to must be different devices.** If they resolve to the same
> endpoint the bridge would capture its own output (feedback), so the card shows
> a red warning and the service refuses to start the session and idles.

### Manual setup (config.json, headless/advanced)

The same settings live in `%ProgramData%\Veyra\config.json` under the `bridge`
block (`enabled`, `source_device_id`, `target_device_id`); the service applies
changes live. Device IDs are listed in
`%ProgramData%\Veyra\logs\veyra-service.log` when the bridge starts:
```
AudioBridge: render endpoint "CABLE Input (VB-Audio Virtual Cable)" = {0.0.0.00000000}.{...}
```

## Limitations

- The capture source must be stereo 32-bit float (the Windows shared-mode
  default). The service logs a warning and idles if it isn't. Differing sample
  rates between capture and playback are fine — the playback side is opened
  with Windows auto-conversion and resamples as needed.
- Loopback adds ~30–80 ms latency — acceptable for music, may lag video lip-sync.
- Capture and playback must be two different endpoints (enforced by the service).

## Switching back to APO (signed builds)

On a signed build, when you switch to a wired endpoint (USB-C DAC, built-in
audio, USB headset): set `"enabled": false` in the `bridge` block, restore your
headphones/speakers as the Windows default output, and associate the APO with
that endpoint via **Devices → Preferred Output** or the "Setup Audio Driver
(Advanced)" Start Menu shortcut. The APO path delivers < 5 ms latency.

## APO vs Bridge

| | APO (primary design) | Audio Bridge |
|-|--------------|------------------------|
| Latency | < 5 ms | 30–80 ms (loopback) |
| Setup | Install once, use forever | Requires virtual cable + default device swap |
| Works unsigned | No — Windows loads only signed APOs | Yes |
| Bluetooth | Never (A2DP hosts no custom APOs) | Works on any endpoint |
| CPU | audiodg.exe, RT-safe | Service thread, user-mode |
