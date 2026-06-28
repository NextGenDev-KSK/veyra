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

### Veyra APO developer registration (Phase 2)

The Veyra APO (`veyra-apo.dll`) implements the APO COM interfaces directly
against the base Windows SDK — no WDK needed to build. Registering it for real
runtime testing on your own machine:

1. **Build** the binaries (`cmake --build --preset windows-release`).
2. **Enable test-signing** and reboot (so an unsigned/test-signed DLL can load
   into `audiodg.exe`):
   ```powershell
   bcdedit /set testsigning on   # then REBOOT
   ```
3. **Register the COM server** from an elevated prompt:
   ```powershell
   cd installer\driver
   .\register-apo.ps1 -DllPath ..\..\build\windows-release\bin\veyra-apo.dll
   ```
4. **Associate the APO with an output endpoint** (writes
   `PKEY_FX_PostMixEffectClsid` to the endpoint's FxProperties PropertyStore
   and restarts Windows Audio so `audiodg.exe` reloads the APO chain):
   ```powershell
   cd installer\driver
   .\associate-apo.ps1          # lists endpoints, pick one interactively
   # or, non-interactively:
   .\associate-apo.ps1 -EndpointGuid "{xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx}"
   # to remove:
   .\associate-apo.ps1 -EndpointGuid "{...}" -Unassociate
   # for the microphone (VeyraMicApo):
   .\associate-apo.ps1 -Capture
   ```
   The script enumerates endpoints from
   `HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\MMDevices\Audio\Render`,
   writes `{D04E05A6-594B-4FB6-A80D-01AF5EEC11D9},6` =
   `{7E9C2B14-3F6A-4D8E-9B21-5C0A1F2E3D44}` under the chosen endpoint's
   `FxProperties` subkey, then restarts `AudioSrv` to reload the chain.
5. **Start the service** (`veyra-service.exe --console` or installed) so the
   shared parameter block exists; the APO reads it live.

6. **To fully remove**: run `installer\driver\uninstall-apo.ps1` (elevated).
   This removes all FxProperties entries from every endpoint, restarts AudioSrv,
   and unregisters the COM server in one step.

> **Shared-memory names:**
> - `Local\VeyraAPOParameters_v1` (service ↔ APO, both in Session 0)
> - `Local\VeyraMicParameters_v1` (service ↔ capture APO, both in Session 0)
> - `Global\VeyraAnalyzer_v1` (service → UI visualizer, cross-session)
> - `Global\VeyraTracker_v1` (service → overlay radar, cross-session)
>
> `Global\` objects are created by the service (LocalService has `SeCreateGlobalPrivilege`)
> and can be opened by any interactive user process. In console mode, a `Local\` fallback
> is used automatically if `Global\` creation fails.

> ⚠️ CI verifies the APO **compiles and links** only. Registration, endpoint
> association, and audio passthrough are inherently runtime/admin/test-signing
> steps that must be done on a real Windows machine.

> The portable build works **without** the driver by falling back to WASAPI
> loopback (user-mode, no admin, but higher latency ~15 ms). The APO path is
> what delivers the <5 ms target.

## 3. Packaging (Phase 14)

- **MSIX** via `MakeAppx.exe` (Windows SDK) — see `installer/msix/`.
- **Portable ZIP** — see `installer/portable/`.

These are stubs in Phase 0.
