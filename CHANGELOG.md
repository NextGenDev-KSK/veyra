# Changelog

All notable changes to Veyra Sounds are documented here.
Format: [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).
Versioning: [Semantic Versioning](https://semver.org/).

---

## [Unreleased]

---

## [1.1.0] — 2026-07-09

Bridge-first release: on unsigned builds the Audio Bridge is the official audio
path, and it is now fully configurable from the app.

### Added
- **Audio Bridge controls in the Devices screen** — enable switch, Capture
  picker, Play to picker, and a live status line that validates the routing in
  plain language. Turning the Bridge on auto-detects a virtual cable (VB-CABLE,
  Voicemeeter, and similar) for Capture and picks the current listening device
  for Play to. The Preferred Output picker now applies only while the Bridge is
  off (it belongs to the signed-APO path) and is disabled otherwise.
- **`FRESH_INSTALL.md`** — step-by-step checklist from a freshly installed
  Windows 11 machine to processed audio, including the Smart App Control
  decision, SmartScreen, VB-CABLE setup, and a what-works table.

### Changed
- **Default-device keeper is Bridge-aware** — while the Bridge is enabled, the
  UI keeps the *capture* device (the virtual sink apps play into) set as the
  Windows default output instead of the APO-path Preferred Output, so the
  routing survives Windows or apps changing the default.
- **Docs rewritten Bridge-first** — README, `docs/AUDIO_BRIDGE.md` (UI workflow
  replaces the config.json walkthrough, which moves to an advanced section),
  and `RELEASE_NOTES.md`.
- **`installer/SIGNING.md`** — honest signing status: SignPath Foundation
  application declined pending public traction (2026-07); roadmap reordered to
  Certum Open Source / Azure Trusted Signing / SignPath reapply.

### Fixed
- **`AudioBridge` same-endpoint guard** — if capture and playback resolve to
  the same endpoint (e.g. both empty, or both pointing at the default device),
  the service now logs a warning and idles instead of starting a session that
  loops its own output back into its capture (doubled audio / echo build-up).
- **UI text mojibake** — em dashes, ellipses and arrows in `const char*` string
  literals rendered as `â€"`-style garbage through `juce::String(const char*)`
  (visible in the onboarding overlay, knob tooltips, Apps placeholders,
  Settings and Sound Lab labels). All user-facing literals are ASCII now.
- **Onboarding step 3 described the APO path** ("no virtual cable, no
  rerouting"), which is wrong on unsigned builds; it now walks the user to
  Devices → Audio Bridge instead.
- **Sidebar showed a stale hardcoded version ("v0.3.0")** — it now renders
  `veyra::kVersionString`, the same single source of truth as the About and
  Updates cards.

---

## [1.0.0] — 2026-07-01

**Audit Pass 5 — release-candidate pass (2026-07-02)**

### Fixed
- **`installer/driver/associate-apo.ps1`, `uninstall-apo.ps1`, `veyra_apo.inf` —
  wrong FxProperties FMTID** — the developer scripts and INF still used the
  legacy `{D04E05A6-…-EC11D9}` GUID (audiodg silently ignores keys under a
  wrong FMTID) and mislabeled PIDs (PreMix is PID 1, not 4; PID 6 is
  ModeEffect, not PostMix). All three now write/remove
  `{D04E05A6-594B-4FB6-A80D-01AF5EED7D1D}` PID 6 (render) / PID 1 (capture),
  matching `VeyraSetupHelper.exe`. `uninstall-apo.ps1` additionally scrubs the
  legacy wrong-GUID keys, and the INF registry line now uses the standard
  value-name form (`HKR,"FX\0",%PKEY%,,%CLSID%`).
- **`apps/veyra-service/UpdaterClient.cpp` — service stop latency** — the daily
  update-check loop slept in 1-minute slices, so stopping the service could
  block up to 60 s (far past SCM's 3 s wait hint, and past the uninstaller's
  8 s stop wait). Now sleeps in 250 ms slices.
- **`apps/veyra-overlay/main.cpp` — tracker attach retry** — if the overlay
  launched before the service, the shared tracker block was opened once and
  never retried, leaving the radar dead until relaunch. It now retries on the
  existing ~10 s config-poll cadence.

### Changed
- **`.github/workflows/build.yml`** — the artifact gate now also verifies
  `VeyraSetupHelper.exe` (all five shipped binaries, previously four).
- **`ARCHITECTURE.md`** — documented that Sound Tracker game auto-detection
  (`GetForegroundWindow`) is inert in session-0 service mode; the manual Gamer
  Mode toggle is unaffected.
- **`RELEASE_AUDIT.md`** — removed a personal filesystem path ahead of the
  repository going public.

**Audit Pass 4 — hardware verification + release audit (2026-07-01)**

### Fixed (audio path, verified on hardware)
- **`apps/veyra-setup-helper/main.cpp` + `installer/setup/veyra-setup.nsi` —
  Bluetooth A2DP endpoints use `PKEY_FX_ModeEffectClsid` (PID 6), not
  `EndpointEffectClsid` (PID 7)** — the association now writes the render CLSID
  to `{D04E05A6-…-ED7D1D},6`. Verified against `audioenginebaseapo.h`
  (SDK 10.0.26100.0): the audio engine reads PID 6 for every endpoint type,
  while PID 7 is silently ignored on A2DP because the Bluetooth stack only
  declares Stream (5) + Mode (6) effect support.
- **`apps/veyra-setup-helper/main.cpp` — original ModeEffect CLSID backup and
  restore** — `--associate` now saves the displaced APO CLSID (e.g. the inbox
  `WMALFXGFXDSP` mode effect) under `HKLM\SOFTWARE\Veyra\Devices\<endpoint>`
  *before* the IPropertyStore write (for an active endpoint the Commit hits the
  physical registry immediately, so reading after would capture our own CLSID).
  `--unassociate-all` restores it instead of deleting the value, leaving the
  endpoint exactly as found.
- **`modules/veyra-common/src/ipc/SharedMemory.cpp` — consumers now map
  shared-memory regions read-only** — `open()` requested `FILE_MAP_ALL_ACCESS`,
  which the hardened DACL (System/Admins full, Everyone read) denies to every
  actual consumer: the APO inside `audiodg.exe` (LocalService), the UI, and the
  overlay (interactive user). The APO would have silently run on defaults and
  the UI visualizer/overlay radar silently fell back to idle animation. All
  consumers are pure readers, so `open()` now requests `FILE_MAP_READ`.
- **`installer/setup/build-installer.ps1` + NSIS — VC++ runtime now actually
  bundled** — `installer/redist/vc_redist.x64.exe` is packed and installed
  silently when the runtime is missing. `veyra-apo.dll` links the dynamic CRT;
  `audiodg.exe` resolves its imports from `System32` only, so a missing
  `MSVCP140.dll` fails the APO load with `ERROR_MOD_NOT_FOUND` (126).

### Fixed (installer)
- **`installer/setup/veyra-setup.nsi` — uninstaller ran without the installer's
  process-wide fixes** — added `un.onInit` with `${DisableX64FSRedirection}`,
  `SetRegView 64`, and `$DataDir` resolution. Without it: the 32-bit `SysWow64`
  regsvr32 silently failed to unregister the 64-bit DLL (the same WOW64 bug
  Audit Pass 3 fixed for install), registry cleanup targeted the WOW6432Node
  view so the Programs & Features entry survived, and "delete my data" removed
  nothing because `$DataDir` was empty.
- **`installer/setup/veyra-setup.nsi` — device-enumeration scratch file
  cleanup** — `$TEMP\veyra-devices.ini` (endpoint names + GUIDs) is now deleted
  when the installer finishes.

### Fixed (service reliability)
- **`apps/veyra-service/ServiceMain.cpp` — startup failure no longer reports a
  clean stop** — a failed `ServiceRuntime::start()` reported `SERVICE_STOPPED`
  with `NO_ERROR`, so SCM recovery never fired and Event Viewer showed nothing.
  It now reports `ERROR_SERVICE_SPECIFIC_ERROR` (code 1; details in the service
  log), and a failed stop-event creation reports its real Win32 error.
- **`apps/veyra-service/ServiceInstaller.cpp` — failure actions on non-crash
  failures** — SCM only runs recovery actions for processes that die without
  reporting `SERVICE_STOPPED`; `SERVICE_CONFIG_FAILURE_ACTIONS_FLAG` is now set
  so self-reported failures (like the above) also trigger the restart ladder.
- **`modules/veyra-common/src/ipc/PipeServer.cpp` — shutdown race** —
  `CancelSynchronousIo` only cancels an operation that is already pending; a
  one-shot cancel could miss the listener between blocking calls and leave
  `stop()` joined forever (hanging `net stop` and the uninstaller). The cancel
  is now re-issued until the listener thread exits.

### Changed (documentation honesty)
- **README, RELEASE_NOTES, INSTALLATION, TROUBLESHOOTING, FEATURE_COVERAGE** —
  corrected the claim that the APO processes audio on unsigned builds. Verified
  on Windows 11 26200.8655: `audiodg.exe` loads only signed APO DLLs, enabling
  test-signing does not help an unsigned DLL, and the `DisableProtectedAudioDG`
  developer override no longer un-protects `audiodg` (confirmed by reboot test).
  The Audio Bridge is documented as the working audio path for this release and
  the only option for Bluetooth A2DP. The INSTALLATION "green LED" row now
  matches the code: it indicates the app→service connection, not APO activity.

**Audit Pass 3 — root-cause audio fix (2026-06-30)**

### Fixed (Audit Pass 3 — root causes for audio not active after install)
- **`installer/setup/veyra-setup.nsi` — WOW64 FS redirection** — added
  `${DisableX64FSRedirection}` in `.onInit`. NSIS is a 32-bit process; without this,
  `$SYSDIR\regsvr32.exe` resolves to the 32-bit copy in `SysWow64`, which cannot
  load a 64-bit DLL (`ERROR_BAD_EXE_FORMAT`). COM registration was silently failing
  on every install. The fix causes `$SYSDIR` to resolve to the real 64-bit
  `C:\Windows\System32`, so regsvr32 correctly registers `veyra-apo.dll`.
- **`apps/veyra-service/ServiceInstaller.cpp` — service reinstall on upgrade** —
  `CreateServiceW` returns `ERROR_SERVICE_EXISTS` when reinstalling over an existing
  copy. Added handler: `OpenServiceW` + `ChangeServiceConfigW` to update the binary
  path, start type, account, and error control on the existing entry. Previously the
  error was swallowed and the service kept the old binary path.
- **`apps/veyra-setup-helper/main.cpp` — `RestartAudioService` race condition** —
  replaced fixed `Sleep(1000)` with `WaitForServiceState` polling
  (`QueryServiceStatus` every 200 ms, up to 8 s). `AudioSrv` and
  `AudioEndpointBuilder` are now stopped in order (each polled to STOPPED) before
  starting in order (each polled to RUNNING). The old fixed sleep caused
  `StartService` to be called before the dependency reached STOPPED state.
- **`apps/veyra-setup-helper/main.cpp` — APO association readback verify** —
  `CmdAssociate` now uses `IMMDevice::OpenPropertyStore(STGM_READWRITE)` +
  `IPropertyStore::SetValue` as the primary write path (correct API for
  FxProperties), with a direct `RegSetValueExW` fallback. After both paths, reads
  back `PKEY_FX_PostMixEffectClsid` and compares to `kRenderClsid`. Returns 1 on
  mismatch. Handles both `REG_SZ` and `REG_BINARY` (PROPVARIANT) value types.
- **`apps/veyra-setup-helper/main.cpp` — new `--verify-service` command** — queries
  SCM with `QueryServiceStatusEx`; returns 0 iff `VeyraAudioService` state is
  `SERVICE_RUNNING`. NSIS calls this after `sc.exe start`.
- **`apps/veyra-setup-helper/main.cpp` — new `--verify-association <guid>` command**
  — reads FxProperties for the given endpoint GUID and confirms the CLSID value
  matches the render APO. NSIS calls this after `--associate`.
- **`installer/setup/veyra-setup.nsi` — `!ifdef HAVE_VCREDIST` compile guard** —
  the `File` instruction packs files at compile time; wrapping the optional vcredist
  block in `!ifdef` prevents a build-time "file not found" error when
  `vc_redist.x64.exe` is not bundled. Fixed `/oname=` syntax (removed extra quotes).
- **`installer/setup/build-installer.ps1` — pass `/DHAVE_VCREDIST` flag** — the
  build script now passes this define to makensis only when `installer/redist/
  vc_redist.x64.exe` is present.

### Fixed (service reliability)
- **`apps/veyra-service/ServiceInstaller.cpp` — SCM crash recovery** — added
  `ChangeServiceConfig2W(SERVICE_CONFIG_FAILURE_ACTIONS)` immediately after
  service creation. `VeyraAudioService` now auto-restarts on crash: 5 s after the
  1st failure, 10 s after the 2nd, 60 s after any subsequent failure. Failure
  count resets after 1 hour of clean uptime. Previously the SCM left the service
  stopped permanently on any unexpected exit.

### Fixed (installer)
- **`installer/setup/veyra-setup.nsi` — desktop shortcut always-selected bug** —
  Removed the `Function .onSelChange` that called `SectionSetFlags ${SecDesktop} 1`
  on every selection change, which made the "optional" desktop shortcut mandatory.
  Moved the initial pre-check to `.onInit` where it fires once at installer start.
  The desktop shortcut is now truly optional — users can uncheck it.
- **`CMakeLists.txt` — version bump 0.9.0 → 1.0.0** — `VEYRA_VERSION` now reflects
  the release version. All compiled binaries, version resources, and the About page
  will report `1.0.0`.

### Fixed (documentation)
- **`TROUBLESHOOTING.md` — removed developer commands from end-user content** —
  "Quick Diagnosis" section rewritten for end users (services.msc, tray icon, LED
  state) instead of PowerShell commands. Problem 2 Fix A removed `bcdedit /set
  testsigning on` from the user-visible troubleshooting steps; replaced with
  user-appropriate steps (re-run Setup Audio Driver shortcut, reinstall). A note
  for open-source builds explains the signing requirement without exposing the
  bcdedit workaround to end users.
- **`TROUBLESHOOTING.md` — Problem 3 / Problem 4 version references** — Problem 3
  now directs Bluetooth users to INSTALLATION.md rather than BUILD_GUIDE.md §2.
  Problem 4 version reference updated from v0.9.0 to v1.0.0.

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
