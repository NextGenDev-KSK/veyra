# Audio Bridge — Advanced Bluetooth Compatibility Mode

> **Most users do not need this.** Veyra's primary audio path is the APO, which
> loads directly into the Windows audio engine and processes all audio on your
> chosen output device — no virtual cable, no configuration. The Audio Bridge
> exists only as a fallback for Bluetooth endpoints that reject the APO.

## When to use the Audio Bridge

Bluetooth endpoints (e.g. **OnePlus Nord Buds 3 Pro**, AirPods, most TWS devices)
often reject custom endpoint APOs. If you associate the Veyra APO with a Bluetooth
endpoint and hear no effect (or silence), fall back to the Audio Bridge.

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

1. Install the virtual sink and **set CABLE Input as the Windows default output**
   (Settings → System → Sound → Output). Apps will now play into the cable; you
   won't hear anything yet.
2. Start the service:
   ```
   build\windows-release\bin\veyra-service.exe --console
   ```
3. In the Veyra UI → **Devices**:
   - Source = `CABLE Output`
   - Output = your Bluetooth headphones
   - Enable = ON
4. Play music. You should hear it through the headphones with DSP applied.

> **Tip:** Set both CABLE Input and your headphones to the same sample rate
> (48 kHz, 16-bit) in Windows Sound → Properties → Advanced to avoid silence
> from format mismatch.

## Limitations

- Source and target must share the same sample rate and be stereo 32-bit float
  (shared-mode default). The service logs and idles if they don't match.
- Loopback adds ~30–80 ms latency — acceptable for music, may lag video lip-sync.
- Audio Bridge is not active if Source or Output is not set.

## Switching back to APO

When you switch to a wired endpoint (USB-C DAC, built-in audio, USB headset),
disable the Audio Bridge in Devices and associate the APO with that endpoint via
**Devices → Preferred Output** or the "Setup Audio Driver (Advanced)" Start Menu
shortcut. The APO path delivers < 5 ms latency.

## Why APO is preferred

| | APO (primary) | Audio Bridge (fallback) |
|-|--------------|------------------------|
| Latency | < 5 ms | 30–80 ms (loopback) |
| Setup | Install once, use forever | Requires virtual cable + default device swap |
| Bluetooth | Varies by endpoint | Works on any endpoint |
| CPU | audiodg.exe, RT-safe | Service thread, user-mode |
| Admin | Test-signing (dev), WHQL (release) | None |
