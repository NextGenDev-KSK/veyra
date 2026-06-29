# Changelog

All notable changes to Veyra Sounds are documented here.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Versioning: [Semantic Versioning](https://semver.org/).

---

## [Unreleased]

### Added (commercial installer — no PowerShell)
- **`apps/veyra-setup-helper/` — VeyraSetupHelper.exe** — native C++ installer
  helper (static CRT, no VC++ dependency) compiled alongside the main binaries.
  Replaces `apo-helper.ps1` entirely. Five operations: `--list-devices <ini>`
  (IMMDeviceEnumerator → INI file for NSIS to read), `--associate <guid>`
  (writes `PKEY_FX_PostMixEffectClsid` + restarts AudioSrv via SCM API),
  `--unassociate-all` (removes Veyra CLSIDs from every render + capture endpoint),
  `--verify-com` (checks HKLM CLSID + DLL on disk), `--restart-audio` (stops +
  restarts AudioSrv + AudioEndpointBuilder via SCM). No PowerShell. No scripts.
  No visible windows.
- **`installer/setup/veyra-setup.nsi` complete rewrite** — zero PowerShell calls.
  All audio operations delegated to `VeyraSetupHelper.exe`. COM registration via
  `regsvr32 /s` (system binary, no window). APO unassociation in uninstaller via
  both `VeyraSetupHelper.exe --unassociate-all` and a native NSIS
  `EnumRegKey` loop (belt-and-suspenders). Device picker populated from INI
  written by the helper — user sees friendly device names, never GUIDs.
- **`CMakeLists.txt`** — `apps/veyra-setup-helper` added as a new target with
  static CRT linkage (`/MT`) so the helper binary has no VC++ runtime dependency.

### Changed
- **`installer/setup/build-installer.ps1` — PS 5.1 compatibility** — removed all
  PowerShell 7-only syntax: `?.` null-conditional operator replaced with explicit
  `if ($cmd) { $makensis = $cmd.Source }` guard. All operators now compatible with
  Windows PowerShell 5.1. NSIS auto-detection searches common install paths, then
  registry (`HKLM\SOFTWARE\NSIS`), then PATH. Friendly error message if not found.
  `apo-helper.ps1` removed from staging (replaced by `VeyraSetupHelper.exe`).
- **`docs/USER_GUIDE.md`** — "Getting sound through Veyra" no longer shows
  `bcdedit` or PowerShell commands. Directs users to the installer.
- **`VERIFY.md`** — Stage B now offers Option A (installer, recommended) and
  Option B (manual developer setup). PS developer scripts explicitly labelled as
  developer tools, not user instructions.

### Changed (APO-first architecture)
- **Primary audio path is now the APO** — `OnboardingOverlay` step 3 updated from
  "Open Devices → Audio Bridge" to the APO-first message ("Veyra loads into the
  Windows audio engine on your current output device — no virtual cable needed").
- **Audio Bridge demoted to advanced fallback** — all user-facing documentation
  and the onboarding flow now position the Audio Bridge as an optional,
  Bluetooth-specific compatibility mode, not the default experience.
- **VERIFY.md** — Stage B is now the APO path (⭐ primary); Audio Bridge moves
  to Stage C (advanced / Bluetooth fallback). Stage C includes updated setup
  steps with sample-rate matching note.
- **docs/AUDIO_BRIDGE.md** — rewritten as an advanced reference with an
  explicit "most users do not need this" callout, an APO vs Bridge comparison
  table, and corrected `%ProgramData%` paths.
- **docs/USER_GUIDE.md** — "Getting sound through Veyra" now leads with the APO
  setup path; Audio Bridge listed as "advanced / Bluetooth fallback". Devices
  screen description updated to reflect the Preferred Output / APO workflow.
  `%APPDATA%` path corrected to `%ProgramData%`.
- **installer/SIGNING.md** — APO driver package section now states APO is the
  primary path; Audio Bridge described as a fallback for Bluetooth endpoints.
- **BUILD_GUIDE.md** — portable fallback note updated: APO is primary (< 5 ms),
  Audio Bridge is the loopback fallback (~30–80 ms, needs virtual cable).
- **HARDWARE_VALIDATION.md** — VB-CABLE moved from required prerequisite to
  optional ("only needed for Section 5 Audio Bridge tests").
- **TROUBLESHOOTING.md** — Problem 3 (Audio Bridge) header updated to
  "Bluetooth fallback" and a note added directing users to set up the APO first.
- **AudioBridge.h** — header comment updated to describe the bridge as the
  advanced compatibility path, not the primary workflow.
- **DevicesScreen.h** — class comment updated to reflect APO preferred output as
  the primary control; Audio Bridge as secondary advanced option.

### Fixed
- **App rules path** — `AppsScreen.cpp` `rulesFile()` now resolves to
  `%ProgramData%\Veyra\app_rules.json` (via `veyra::paths::appDataDir()`) instead
  of `%AppData%\Veyra\app_rules.json`. The old path caused rules saved from the UI
  to be invisible to the service on restart, as both use `%ProgramData%\Veyra` since v0.9.0.
- **Crash folder path** — `crashBanner_.onOpenFolder` now calls
  `juce::File(veyra::paths::crashesDir().string()).revealToUser()` instead of the
  stale `userApplicationDataDirectory / Veyra / crashes` path. The "Open Folder"
  button in the crash-recovery banner now opens the correct `%ProgramData%\Veyra\crashes` directory.

### Added
- **Professional NSIS installer** — `installer/setup/veyra-setup.nsi` produces a
  commercial-quality `veyra-sounds-setup-{version}-x64.exe`. Install flow:
  Welcome → License → Directory → Install Files → Device Setup → Finish. The
  installer: checks Windows build 19041+; verifies/installs VC++ 2015-2022 x64
  runtime; extracts all binaries; registers the APO COM server; installs and
  starts `VeyraAudioService`; shows a **device picker** with friendly endpoint
  names (no GUIDs exposed); associates the APO; falls back to Audio Bridge mode
  with a clear explanation on failure; sets Start with Windows; creates Start Menu
  and optional Desktop shortcuts; writes a full Programs & Features entry; and
  logs to `%ProgramData%\Veyra\logs\install.log`. The uninstaller asks whether to
  preserve user presets and settings.
- **Upgrade detection** — `.onInit` reads the existing `InstallLocation` from the
  registry and, when an upgrade is detected, stops the old service and replaces
  binaries in-place without touching user data.
- **`installer/setup/apo-helper.ps1`** — installer-internal PowerShell helper
  with five actions: `list` (enumerates active render endpoints, writes an INI
  for NSIS to read), `associate` (writes `PKEY_FX_PostMixEffectClsid` and
  restarts AudioSrv), `verify-com` (checks the CLSID is registered and the DLL
  present), `check-testsign` (bcdedit query), and `unassociate-all` (removes
  Veyra CLSIDs from every render + capture endpoint — used by the uninstaller).
- **APO setup helper** — `installer/setup/setup-audio-driver.cmd` is installed to
  `%ProgramFiles%\Veyra\` and linked from the Start Menu as "Setup Audio Driver
  (Advanced)". It re-launches elevated and runs `associate-apo.ps1` interactively,
  so users who need to re-associate after hardware changes never need to open a
  PowerShell window manually.
- **INSTALLATION.md** — complete end-user installation and quick-start guide.
  Covers download, setup, Bluetooth fallback, per-device APO association, startup
  configuration, uninstallation, and data paths.

### Changed
- **README.md** — "Building" section split into "Installing" (end users, links to
  INSTALLATION.md) and "Building from source" (developers). Project structure
  entry for `installer/` updated to reflect the production installer.
- **BUILD_GUIDE.md** — new §3 "Building the end-user installer" documents the
  `build-installer.ps1` workflow; PowerShell driver scripts explicitly labelled
  as developer tools not intended for end-user documentation.
- **`installer/setup/build-installer.ps1`** — staging now includes
  `apo-helper.ps1`, `resources/themes/`, `resources/autoeq/`, and an APO-first
  `INSTALLATION.txt` quick-start file (replaces the old Bridge-first README).
- **TROUBLESHOOTING.md** — covers the 9 most common issues (LED amber, no APO
  effect, Audio Bridge, app rules, overlay, crash banner, launch/single-instance,
  high CPU, startup registration) with root causes, diagnostic commands, and fixes.
- **HARDWARE_VALIDATION.md** — 54-item hardware validation checklist covering
  install, UI/UX, APO render, capture APO, Audio Bridge, overlay, visualizer,
  hot-plug, sleep/resume, Sound Lab, uninstall, and 8-hour soak, with expected
  results, failure modes, and a rollback guide for each section.

### Changed
- **`make-portable.ps1`** — the driver/ folder in the portable ZIP now includes
  `associate-apo.ps1` and `uninstall-apo.ps1` in addition to `register-apo.ps1`,
  so portable users have the complete APO management toolkit.
- **`SIGNING.md`** — version examples updated from `0.3.0` to `0.9.0`.
- **`CMakeLists.txt`** — `VEYRA_FETCH_DEPS` option comment updated to remove stale
  Phase 0 language.

---

## [0.9.0] — 2026-06-28

### Security
- **Pipe client session authentication** — After `ConnectNamedPipe`, the server
  validates the client's Windows session via `GetNamedPipeClientSessionId` against
  `WTSGetActiveConsoleSessionId`. Connections from non-console sessions (e.g. an RDP
  user in a parallel session on a multi-user machine) are rejected and logged with the
  client PID and session ID. Session 0 (LocalService) and the active console session
  are the only accepted origins.
- **IPC payload size cap** — `Protocol.h` now defines `kMaxPayloadBytes` (4 MiB).
  `parse()` rejects oversized payloads before the bounds check, preventing integer
  wraparound on `sizeof(header) + payloadSize`. `readMessage()` also stops accumulating
  early if the in-flight size exceeds the cap, preventing OOM from multi-chunk messages.
- **Named pipe DACL** — `CreateNamedPipeW` now passes an explicit SDDL
  `D:(A;;FA;;;SY)(A;;FA;;;BA)(A;;GRGW;;;IU)` instead of `nullptr`. Restricts pipe
  connections to LocalSystem, Administrators, and the current interactive user. Prevents
  any unprivileged background process from connecting and issuing control commands.
- **Shared memory DACL** — Replaced the null DACL (world-writable) on all named shared
  memory sections with an explicit DACL granting full access only to LocalSystem and
  Administrators, and read-only to Everyone. The APO/overlay/UI only need to read.
- **Service account** — Installed service now runs as `NT AUTHORITY\LocalService` instead
  of LocalSystem. LocalService has `SeCreateGlobalPrivilege` (needed for `Global\` objects)
  while carrying a significantly smaller privilege set than LocalSystem.
- **Cross-session shared memory** — Tracker and analyzer shared blocks renamed to
  `Global\VeyraTracker_v1` / `Global\VeyraAnalyzer_v1` so the Session-0 service and
  Session-1 UI/overlay can both see them when the service is installed. APO parameter
  blocks stay `Local\` (both service and audiodg.exe are in Session 0). A `Local\`
  fallback is applied automatically in console mode where `SeCreateGlobalPrivilege`
  is absent.

### Fixed
- **Data path** — Service and UI now store config, presets, logs, and crash reports in
  `%ProgramData%\Veyra` (`C:\ProgramData\Veyra`) instead of `%AppData%\Veyra`. This
  ensures both the installed LocalService account and the interactive user process resolve
  the same directory. Existing configs in `%AppData%\Veyra` should be migrated manually.
- **JUCE deprecation** — Replaced `juce::Displays::Display::userArea` with `userBounds`
  in `RootComponent.cpp`, eliminating the C4996 compiler warning.
- **Parametric EQ APO path** — `parametricMode` and editor band data now flow from the
  service through shared memory (`VeyraParamsPayload`) to the render APO. Previously the APO
  always ran graphic EQ regardless of the UI's parametric mode setting. Added `PayloadBand`
  struct and `parametricMode` / `parametricCount` / `parametricBands[16]` fields to the
  shared-memory contract; `ApoPublisher` publishes them; `VeyraApo` applies them via
  `DspChain::setParametricBands()` before `setParameters()` so the explicit bands take
  precedence over the 10-band fallback.
- **Mic noise gate honoured** — `MicPublisher` now sets `noiseSuppression = 0` when
  `VoiceConfig::noiseGate` is `false`, so the noise suppressor actually disables when the
  user turns the gate off. Previously the suppressor ran regardless of the gate toggle.

---

## [0.4.0]

### Fixed
- **Denormal protection** — Added `_mm_setcsr` (FTZ+DAZ) on the first `APOProcess` call in
  both render and capture APOs via a `denormalsSet_` flag. Without this, decayed filter states
  on x86 CPUs without hardware DAZ/FTZ cause 10–100× CPU overhead during silence. Biquad also
  adds a `1.0e-25f` anti-denormal guard to the y1 feedback state as secondary defence for the
  AudioBridge path (which runs without APO context).
- **Freeverb quality** — Upgraded reverb from 4 comb + 2 allpass per channel to the full
  standard Freeverb complement of 8 comb + 4 allpass with the complete Schroeder tuning set
  (1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 / 556, 441, 341, 225). `kCombMax` raised
  to 3700 to fit the largest tuning at 96 kHz. Reverb tail is now denser and more musical.
- **FeatureExtractor mid-band energy** — Fixed physically incorrect RMS subtraction for the
  Sound Tracker's mid-band energy. Mid-band power is now computed as
  `max(0, totalPow − lowPow − highPow)` in the power domain before `sqrt`. Voice
  classification accuracy improves as a result.
- **INF ExtensionId** — Replaced all-zeros placeholder GUID in `veyra_apo.inf` with
  `{E7C3A8F2-4B6D-4E1F-9D0A-B3C5F7E2A841}`. Windows rejects all-zeros ExtensionIds;
  this was a hard blocker for production APO deployment.
- **Overlay config polling** — Reduced config.json disk poll in the overlay render loop from
  every 15 frames (~0.5 s) to every 300 frames (~10 s), eliminating render stalls on HDDs
  and network-mapped `%APPDATA%` paths.
- **GamerMode toggle layout** — Replaced hardcoded `+50 px` horizontal toggle offset with
  `cellW − toggleWidth − 4` (right-aligned within cell). Label draw width is now computed
  from available space rather than a fixed 48 px. Fixes layout for longer locale strings.

---

## [0.3.0] — DSP + Spatial Complete

### Added
- Full 30-effect DSP chain: EQ, compressor, transient shaper, bass enhancer, harmonic exciter,
  saturation (3 modes, 2× oversampled), stereo widener, multiband width, reverb, room simulator,
  crossfeed, HRTF, field compensation, night mode, equal loudness, loudness normaliser,
  true-peak limiter, spectrum analyser.
- MIT KEMAR HRTF database (full + diffuse sets); ITD-aware interpolation.
- RNNoise denoiser (v0.1.1, MSVC VLA→alloca patch) — PIMPL wrapper for use in capture APO.
- NLMS Acoustic Echo Canceller (256 taps, allocation-free after `prepare()`).
- 16-band Parametric EQ with node editor.
- AutoEQ — 16 oratory1990 headphone profiles included.
- Hearing Test → personalised EQ shaping.
- EBU R128 Loudness Normaliser and ISO-226 Equal Loudness curve.
- True-Peak Limiter: 64-sample lookahead, 4-phase 8-tap windowed-sinc ISP detection.
- Sound Tracker: WASAPI loopback → heuristic classifier → radar HUD overlay.
- VoiceChain: HPF → RNNoise → compressor → de-esser → AGC → presence → output gain.

---

## [0.2.0] — UI + Presets + Themes

### Added
- Full JUCE 8.0.13 UI: Home, Presets, Apps, Devices, Sound Lab, Gamer Mode, Settings.
- 11 built-in themes with live switching; custom accent colour.
- 27 built-in presets with categories, favourites, and curated popular list.
- Mini Mode, global hotkeys, system tray, DWM Acrylic backdrop.
- Onboarding wizard, per-app rule engine, crash capture + recovery banner.
- 8 visualiser modes, Sound Lab (7 diagnostic tools).

---

## [0.1.0] — Service + IPC Backbone

### Added
- Four-binary project: APO DLL, service, UI, overlay.
- Windows service with SCM registration; `--console` for development.
- Named-pipe binary protocol (`VYRA` magic, version field, 12-byte framing).
- Seqlock shared memory for APO parameters.
- Config round-trip (JSON, atomic save), Preset schema, AppRules.
- CI: GitHub Actions, MSVC x64, CMake preset, CTest, portable ZIP artifact.
