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
│  └─ Windows Service, NT AUTHORITY\LocalService, autostarts   │
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
- **Service** is the orchestrator. It runs as `NT AUTHORITY\LocalService` and
  performs filesystem I/O, network calls, and process enumeration. The UI never
  touches the APO directly — the service mediates.
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

## IPC security

- **Named pipe DACL** — `veyra-control` is restricted to LocalSystem, Administrators,
  and Interactive Users (`D:(A;;FA;;;SY)(A;;FA;;;BA)(A;;GRGW;;;IU)`).
- **Session validation** — after `ConnectNamedPipe`, the server checks
  `GetNamedPipeClientSessionId` against `WTSGetActiveConsoleSessionId`. Connections
  from non-console sessions (e.g. RDP users in other sessions) are rejected and logged.
- **Shared memory DACL** — `Global\VeyraTracker_v1` / `Global\VeyraAnalyzer_v1` grant
  full access only to System and Administrators; read-only to Everyone.
- **Data directory** — `C:\ProgramData\Veyra` (accessible by both LocalService and
  the interactive user). Not `%AppData%` which resolves to the service account's profile.

## Service reliability

- **SCM crash recovery** — `installService()` calls `ChangeServiceConfig2W(SERVICE_CONFIG_FAILURE_ACTIONS)` with three `SC_ACTION_RESTART` entries (5 s / 10 s / 60 s) and a 1-hour reset period. If the service exits unexpectedly, the SCM restarts it automatically without user intervention.
- **IPC reconnect** — `ServiceClient` runs a background thread with exponential back-off (500 ms → 30 s cap). A crashed-and-restarted service is reconnected within the back-off window. The UI shows the amber LED during the gap; no user action needed.
- **APO fault isolation** — The APO runs inside `audiodg.exe`. A UI or service crash does not affect the APO. The APO continues processing audio with the last parameters written to shared memory until the service restarts and republishes.

## Device change behaviour

- **APO path** — The APO is endpoint-specific. When the Windows default playback device changes, `audiodg.exe` naturally unloads and reloads APO instances per-endpoint. If the new endpoint has the APO associated, processing is seamless. If not, the user can re-associate via **Start → Veyra Sounds → Setup Audio Driver (Advanced)**.
- **Audio Bridge path** — `AudioBridge::run()` loops with a 750 ms back-off. On device disconnect, `session()` returns false and the loop retries until the device reappears or the config changes. No user action required.
- **Known limitation** — The service does not implement `IMMNotificationClient`. There is no proactive notification to the UI when a new endpoint is plugged in. The Devices screen reflects the current enumeration on demand. This matches the behaviour of FxSound and Equalizer APO; proactive hot-plug following is a post-1.0 item.
- **Known limitation** — The Sound Tracker's auto-activation on a foreground game (`GetForegroundWindow` in `TrackerService`) is inert when running as an installed service: session-0 services cannot see the interactive desktop's foreground window. The manual Gamer Mode toggle works in both modes; auto-detection works only in `--console` developer runs. Moving detection to the UI process is a post-1.0 item.
