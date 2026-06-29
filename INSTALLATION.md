# Veyra Sounds — Installation Guide

> **This guide is for end users.** If you are building from source, see
> [BUILD_GUIDE.md](BUILD_GUIDE.md) and [VERIFY.md](VERIFY.md).

---

## Requirements

- Windows 10 version 2004 (build 19041) or later, 64-bit
- 200 MB disk space
- No virtual cable, no special drivers needed for basic use

---

## Installing

1. Download **`veyra-sounds-setup-x.y.z-x64.exe`** from the
   [Releases page](https://github.com/NextGenDev-KSK/veyra/releases).
2. Double-click the installer.
3. Click through: **Next → I Accept → Next → Install**.
4. When the **Device Setup** page appears, pick your speakers or headphones
   from the dropdown and click **Next**.
5. Click **Finish** — optionally check **Launch Veyra now**.

That's it. Veyra is running. The brand LED in the top-left corner turns
**green** when the audio engine is live.

---

## First launch

| What you see | What it means |
|---|---|
| Green LED | APO active — audio is being processed |
| Amber LED | Service not connected — try restarting Veyra |

Play any audio. Go to **Home → EQ** and move a band — you should hear the
change immediately with < 5 ms latency.

---

## Bluetooth headphones

Bluetooth endpoints sometimes reject the APO driver. If the LED stays amber
after setup, or if you hear no effect on Bluetooth:

1. Open **Devices** in Veyra.
2. Under **Audio Bridge (Bluetooth compatibility mode)**, enable the bridge
   and select your Bluetooth headphones as the output.
3. Install **VB-CABLE** (free) from <https://vb-audio.com/Cable/> if prompted,
   and set it as your Windows default output device.

This adds ~30–80 ms latency, which is normal for Bluetooth.

---

## Associating the APO with a different device

If you get new headphones or speakers:

1. Open **Start → Veyra Sounds → Setup Audio Driver (Advanced)**.
2. Follow the prompt — it lists your active playback devices by name.
3. Select the new device and confirm. Windows Audio restarts briefly.

---

## Starting with Windows

The installer sets Veyra to start with Windows automatically. To change this:

- **Disable:** `Settings → About → Start with Windows` (toggle off).
- **Re-enable:** toggle back on, or open Task Manager → Startup and enable
  `VeyraSounds`.

---

## Uninstalling

**Settings → Apps → Veyra Sounds → Uninstall**, or
**Control Panel → Programs → Uninstall a program → Veyra Sounds**.

The uninstaller asks whether to keep your presets and settings. Click **Yes**
to preserve them (they stay in `%ProgramData%\Veyra\`).

---

## Where data is stored

| Path | Contents |
|---|---|
| `%ProgramFiles%\Veyra\` | Binaries, read-only resources |
| `%ProgramData%\Veyra\config.json` | Active configuration |
| `%ProgramData%\Veyra\presets\` | User presets |
| `%ProgramData%\Veyra\logs\` | Service and installer logs |
| `%ProgramData%\Veyra\crashes\` | Crash reports (if any) |

---

## Troubleshooting

See [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for common issues.

For APO driver details and signing, see [SIGNING.md](installer/SIGNING.md).
