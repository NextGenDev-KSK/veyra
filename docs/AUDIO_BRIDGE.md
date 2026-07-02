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

## Setup

> **Note (v1.0.0):** the Devices screen currently exposes only the Preferred
> Output picker — the Bridge source/target/enable controls are not in the UI
> yet (planned post-1.0). For now the Bridge is configured in
> `%ProgramData%\Veyra\config.json`, which the service picks up live.

1. Install the virtual sink and **set CABLE Input as the Windows default output**
   (Settings → System → Sound → Output). Apps will now play into the cable; you
   won't hear anything yet.
2. Find your device IDs. Open `%ProgramData%\Veyra\config.json` and set
   `"bridge": { "enabled": true }` (leave the IDs empty for now), then restart
   the service (`services.msc` → Veyra Audio Service → Restart). The service
   log (`%ProgramData%\Veyra\logs\veyra-service.log`) now lists every render
   endpoint with its ID:
   ```
   AudioBridge: render endpoint "CABLE Input (VB-Audio Virtual Cable)" = {0.0.0.00000000}.{...}
   AudioBridge: render endpoint "OnePlus Nord Buds 3 Pro" = {0.0.0.00000000}.{...}
   ```
3. Put the IDs into the `bridge` block — **source** is the virtual cable,
   **target** is your headphones:
   ```json
   "bridge": {
     "enabled": true,
     "source_device_id": "{0.0.0.00000000}.{...cable...}",
     "target_device_id": "{0.0.0.00000000}.{...headphones...}",
     "preferred_output_id": ""
   }
   ```
   Save the file — the service applies it live, no restart needed.
4. Play music. You should hear it through the headphones with DSP applied.

> **Do not leave both IDs empty with `enabled: true`** — both then resolve to
> the same default endpoint and you will hear the original and the processed
> audio doubled. Source and target must be different devices.

> **Tip:** Set both CABLE Input and your headphones to the same sample rate
> (48 kHz, 16-bit) in Windows Sound → Properties → Advanced to avoid silence
> from format mismatch.

## Limitations

- Source and target must share the same sample rate and be stereo 32-bit float
  (shared-mode default). The service logs and idles if they don't match.
- Loopback adds ~30–80 ms latency — acceptable for music, may lag video lip-sync.
- Audio Bridge is not active if Source or Output is not set.

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
