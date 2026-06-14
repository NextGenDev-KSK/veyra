# Veyra Sounds — Build Guide

This covers building locally, plus the (later-phase) audio-driver test-signing
and APO registration workflow.

## 1. Toolchain

### Phase 0 (current) — minimal

The no-op stubs link no third-party dependencies, so you only need:

| Tool | Notes |
|---|---|
| **Visual Studio 2022 Build Tools** | "Desktop development with C++" workload (MSVC v143 + Windows 11 SDK). |
| **CMake ≥ 3.25** | Bundled with VS, or install standalone. |
| **Ninja** | Bundled with VS ("C++ CMake tools"), or install standalone. |
| **Git** | For cloning. |

Open a **"x64 Native Tools Command Prompt for VS 2022"** (so `cl` and the SDK
are on `PATH`), then:

```sh
cmake --preset windows-release
cmake --build --preset windows-release
```

Binaries land in `build/windows-release/bin/`.

> CI does the same on `windows-latest` using `ilammy/msvc-dev-cmd` (MSVC env)
> and `lukka/get-cmake` (CMake + Ninja). See `.github/workflows/build.yml`.

### Later phases — additional

| Phase | Adds |
|---|---|
| 2 (APO) | **Windows Driver Kit (WDK)** matching your SDK — provides the APO base classes and `audioenginebaseapo.h`. |
| 3 (DSP) | `spdlog`, optional `kissfft` submodules. |
| 4 (UI) | `JUCE` submodule. |
| 6 (Voice) | `rnnoise` submodule. |

Enable bundled deps with `-DVEYRA_FETCH_DEPS=ON` after initialising the
relevant submodule (commands are documented in `.gitmodules`).

## 2. APO registration (Phase 2+)

The APO must be registered through a Windows audio driver INF plus a signed
catalog (`.cat`). Three signing paths, in order of preference for an
open-source project:

1. **Microsoft Hardware Dev Center (WHQL)** — free for verified accounts;
   produces a production-trusted signature.
2. **Free EV / signing program** — e.g. SignPath.io's open-source program or
   Azure Trusted Signing free tier.
3. **Developer test-signing** — for local development only:

```powershell
# Run as Administrator, then reboot.
bcdedit /set testsigning on
```

With test-signing on, a self-signed test certificate can install the driver
package for local APO testing. Turn it off (`bcdedit /set testsigning off`)
when done.

> The portable build works **without** the driver by falling back to WASAPI
> loopback (user-mode, no admin, but higher latency ~15 ms). The APO path is
> what delivers the <5 ms target.

## 3. Packaging (Phase 14)

- **MSIX** via `MakeAppx.exe` (Windows SDK) — see `installer/msix/`.
- **Portable ZIP** — see `installer/portable/`.

These are stubs in Phase 0.
