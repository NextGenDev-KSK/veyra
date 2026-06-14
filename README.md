# Veyra Sounds

[![build](https://github.com/NextGenDev-KSK/veyra/actions/workflows/build.yml/badge.svg)](https://github.com/NextGenDev-KSK/veyra/actions/workflows/build.yml)

Maintainer: **Krithik S** ([@NextGenDev-KSK](https://github.com/NextGenDev-KSK))

**Veyra Sounds** is a free, open-source, system-wide audio enhancer for Windows
10/11 (x64) — EQ, dynamics, spatial audio, voice processing, a gamer Sound
Tracker HUD, and more — built to be faster, lighter, and prettier than the
commercial alternatives. Licensed under the **GNU GPLv3**.

> **Project status: Phase 1 (service + UI + IPC backbone).**
> The service runs under the SCM (or `--console`), hosts the
> `\\.\pipe\veyra-control` named-pipe server, and owns `config.json`. The UI is
> a minimal native shell that shows live connection status with auto-reconnect.
> Real DSP and the APO are still no-op stubs; the JUCE UI lands in Phase 4.
> See [the build order](#build-order).

## Architecture

Veyra runs as three cooperating processes plus an APO DLL loaded into the
Windows audio engine. See **[ARCHITECTURE.md](ARCHITECTURE.md)** for the full
diagram and rationale.

| Binary | Role |
|---|---|
| `veyra-apo.dll` | Real-time DSP inside `audiodg.exe` (the only true system-wide, low-latency path). |
| `veyra-service.exe` | Orchestrator service: config/preset state, detection, Sound Tracker, updater. |
| `veyra.exe` | The JUCE UI app. |
| `veyra-overlay.exe` | The anti-cheat-safe layered-window Sound Tracker HUD. |

## Building

> **Heads-up:** Phase 0 stubs link no third-party dependencies, so a clean
> build needs only MSVC + the base Windows SDK + CMake + Ninja. The canonical
> build is the GitHub Actions workflow on `windows-latest`; local builds are
> optional. Full toolchain setup (incl. the WDK needed for the APO later) is in
> **[BUILD_GUIDE.md](BUILD_GUIDE.md)**.

```sh
cmake --preset windows-release
cmake --build --preset windows-release
```

Outputs land in `build/windows-release/bin/`:
`veyra.exe`, `veyra-service.exe`, `veyra-apo.dll`, `veyra-overlay.exe`.

### Trying the Phase 1 backbone

```sh
# Option A — run the service in the foreground (no admin needed):
build/windows-release/bin/veyra-service.exe --console

# Option B — install + start the real Windows service (run as admin):
build/windows-release/bin/veyra-service.exe --install
sc start VeyraAudioService
# ... later: veyra-service.exe --uninstall

# Then launch the UI; it shows "Service connected · v<version>" when the
# pipe handshake succeeds, and an animated reconnect indicator otherwise.
build/windows-release/bin/veyra.exe
```

Config and logs are written under `%APPDATA%\Veyra\` (`config.json`, `logs/`).


## Build order

The product is built in numbered phases, each with acceptance criteria; see the
engineering spec. Phase 0 is complete when CI builds all four binaries green.

## Contributing

See **[CONTRIBUTING.md](CONTRIBUTING.md)**. All contributions are GPLv3, and PRs
must respect the performance budget.

## License

GPLv3 — see [LICENSE](LICENSE).
