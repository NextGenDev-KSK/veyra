# Veyra Sounds — Roadmap

What's planned beyond v1.2.0, in rough priority order. Grounded in what the
codebase can actually support; no promises without a path.

## Near term

### Per-app audio (process loopback)
Windows 10 2004+ can capture a single process's audio
(`ActivateAudioInterfaceAsync` with `AUDIOCLIENT_ACTIVATION_TYPE_PROCESS_LOOPBACK`).
That enables per-app EQ profiles, per-app volume, and "process the game, leave
the music player alone" — building on the existing per-app rule engine. This is
the headline feature of the next release.

### winget distribution
`winget install VeyraSounds.Veyra` — manifests live under `packaging/winget/`;
submission to microsoft/winget-pkgs happens per release. winget installs skip
most SmartScreen friction and widen reach, which also strengthens the SignPath
Foundation reapplication.

### Accessibility + localization
- The sidebar and custom controls need JUCE accessibility handlers so screen
  readers (and UI automation) can navigate the app.
- The localization scaffolding (`resources/lang/`, `Localization.cpp`) exists
  but ships English-only; wiring real translations is mostly template work.

## Medium term

### Convolution engine
An impulse-response loader (headphone correction beyond the 16 AutoEQ profiles,
room correction) as a DSP chain stage. Partitioned convolution to keep the
allocation-free real-time guarantee.

### Microsoft Store (MSIX)
Store packages are signed by Microsoft, which would end the SmartScreen story
for the app itself. Whether the service + APO architecture fits Store policy
(full-trust packaged services) needs investigation before this is promised.

### Code signing
- Reapply to the SignPath Foundation once the project has visible traction
  (stars, articles, independent discussion) — their stated criteria.
- Alternatively Certum Open Source (~€69/year) or Azure Trusted Signing
  ($9.99/month) whenever the budget exists. Signing activates the < 5 ms APO
  path and the in-engine mic APO on stock Windows.

## Long term

### Own virtual audio device
An FxSound-style bundled virtual device would remove the VB-CABLE dependency —
but kernel drivers require EV certificate + Microsoft attestation signing, so
this only makes sense after the project has a signing budget.

### Post-signing consolidation
Once the APO path is live for everyone: bridge auto-fallback only for
Bluetooth, APO-first onboarding, and hardware-validation completion
(`HARDWARE_VALIDATION.md`).
