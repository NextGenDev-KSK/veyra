# Veyra Sounds — Architecture

Veyra runs as **three cooperating processes** plus an **APO COM DLL** loaded
into the Windows audio stack.

```
┌─────────────────────────────────────────────────────────────┐
│  Windows Audio Engine (audiodg.exe)                         │
│  └─ Veyra APO (veyra_apo.dll)        ←─ MFX / EFX APO       │
│       │  Real-time DSP. Lives inside audiodg.                │
│       │  Reads parameters via shared memory from service.    │
│       │  Never blocks. Never allocates.                      │
└────────────────┬────────────────────────────────────────────┘
                 │ shared memory (parameters) + ring buffer (analyzer/tracker)
                 ▼
┌─────────────────────────────────────────────────────────────┐
│  Veyra Audio Service (veyra-service.exe)                    │
│  └─ Windows Service, runs as SYSTEM, autostarts              │
│       - Mediates between UI and APO                          │
│       - Holds canonical config + preset state                │
│       - Performs Sound Tracker DSP analysis on capture       │
│         from a loopback session of the system mixer          │
│       - Game detection (process enumeration polling)         │
│       - Updater                                              │
│       - Crash log collector                                  │
└────────┬───────────────────────────────────┬────────────────┘
         │ named pipe \\.\pipe\veyra-control  │ named pipe \\.\pipe\veyra-tracker
         ▼                                   ▼
┌────────────────────────┐         ┌────────────────────────┐
│  Veyra UI App          │         │  Veyra Overlay         │
│  (veyra.exe — JUCE)    │         │  (veyra-overlay.exe)   │
│  - Main UI, presets,   │         │  - Window-overlay-only │
│    settings, vis,      │         │    Sound Tracker HUD   │
│    mini-mode           │         │  - WS_EX_TRANSPARENT + │
│  - Sends config to     │         │    WS_EX_LAYERED +     │
│    service             │         │    WS_EX_TOPMOST       │
│  - Visualizer reads    │         │  - Draws with Direct2D │
│    ring buffer from    │         │    on a per-pixel-alpha│
│    APO                 │         │    layered window      │
└────────────────────────┘         └────────────────────────┘
```

## Why this split

- **APO** lives inside `audiodg.exe`, the Windows audio engine — the *only* way
  to do true system-wide low-latency processing. It MUST be lean, non-blocking,
  and allocation-free in the real-time thread.
- **Service** is the orchestrator. It can run as SYSTEM and do filesystem I/O,
  network calls, and process enumeration. The UI never touches the APO
  directly — the service mediates.
- **UI** is a normal user-mode JUCE app. If it crashes, audio keeps running.
- **Overlay** is its own process so a game/GPU-driver crash can't take down the
  UI or audio. It uses a **layered window only** — never D3D hooks — so it is
  anti-cheat safe.

## Anti-cheat strategy

Default to a **layered-window overlay** (`WS_EX_LAYERED | WS_EX_TRANSPARENT |
WS_EX_TOPMOST`): just a transparent topmost window that hooks no graphics API
and is not flagged by Vanguard, EAC, BattlEye, or Faceit AC.

When the service detects a competitive title (via an auto-updating blocklist),
it warns the user and lets *them* decide whether to enable the optional D3D-hook
mode (for non-anti-cheat games where a layered window doesn't render in
exclusive fullscreen). The blocklist is fetched from a signed source at launch
and cached.

## Phase 0 mapping

In Phase 0 each box above is a no-op stub binary; the diagram is the target
architecture that the numbered build phases fill in.
