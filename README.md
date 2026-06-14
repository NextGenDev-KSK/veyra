# Veyra Sounds

[![build](https://github.com/veyra-sounds/veyra/actions/workflows/build.yml/badge.svg)](https://github.com/veyra-sounds/veyra/actions/workflows/build.yml)

**Veyra Sounds** is a free, open-source, system-wide audio enhancer for Windows
10/11 (x64) — EQ, dynamics, spatial audio, voice processing, a gamer Sound
Tracker HUD, and more — built to be faster, lighter, and prettier than the
commercial alternatives. Licensed under the **GNU GPLv3**.

> **Project status: Phase 0 (scaffolding).**
> The repository currently builds four *no-op stub* binaries to prove the
> structure and toolchain end-to-end on CI. Real DSP, the APO, IPC, and the
> JUCE UI land in later phases. See [the build order](#build-order).

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

## Build order

The product is built in numbered phases, each with acceptance criteria; see the
engineering spec. Phase 0 is complete when CI builds all four binaries green.

## Contributing

See **[CONTRIBUTING.md](CONTRIBUTING.md)**. All contributions are GPLv3, and PRs
must respect the performance budget.

## License

GPLv3 — see [LICENSE](LICENSE).
