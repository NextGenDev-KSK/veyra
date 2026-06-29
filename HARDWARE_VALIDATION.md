# Veyra Sounds — Hardware Validation Guide

This document is the definitive hardware validation checklist for Veyra Sounds v1.0.
Every item must be executed on a **physical Windows 10/11 x64 machine** — CI cannot substitute for audio hardware.

**Legend:**
- `[ ]` — must be verified by a human tester
- **PASS** / **FAIL** — record the result against each item
- Failures block the v1.0 final tag; record the device model and Windows build for all items

---

## Pre-conditions

Complete these before any test:

```
[ ] Windows 10 v2004 (build 19041+) or Windows 11 — confirm: winver
[ ] Test machine has a wired output endpoint (built-in or USB audio)
[ ] Test-signing enabled: bcdedit /set testsigning on + reboot
[ ] Sysinternals Process Explorer installed (for DLL verification)
[ ] VB-CABLE virtual audio cable installed — optional, only required for Section 5 (Audio Bridge / Bluetooth fallback tests)
[ ] Veyra built from source: cmake --preset windows-release && cmake --build --preset windows-release
[ ] Service NOT yet installed (clean state for install tests)
```

---

## Section 1 — Installer / Service Installation

```
[ ] 1.1  Run the NSIS installer (veyra-sounds-setup-x64.exe) as a normal user
         EXPECTED: UAC prompt appears; proceeds with admin rights
         FAILURE:  No UAC prompt, or prompt denied — installer requires admin (correct behaviour)

[ ] 1.2  After installer completes, verify service is installed and running:
           sc query VeyraAudioService
         EXPECTED: STATE: 4  RUNNING
         FAILURE:  Missing or STOPPED — check Event Viewer → System → Service Control Manager

[ ] 1.3  Verify COM server is registered:
           reg query "HKCR\CLSID\{7E9C2B14-3F6A-4D8E-9B21-5C0A1F2E3D44}"
         EXPECTED: InprocServer32 key present pointing to veyra-apo.dll
         FAILURE:  Key missing — re-run regsvr32 /s veyra-apo.dll elevated

[ ] 1.4  Verify install directory:
           dir "C:\Program Files\Veyra Sounds\"
         EXPECTED: veyra.exe, veyra-service.exe, veyra-apo.dll, veyra-overlay.exe, driver\, hrtf\, LICENSE
         FAILURE:  Any binary missing — re-run installer

[ ] 1.5  Verify Programs & Features / Settings → Apps entry exists for "Veyra Sounds"
         EXPECTED: Listed with correct version and publisher
         FAILURE:  Missing — registry key HKLM\...\Uninstall\VeyraSounds was not written

[ ] 1.6  Verify Start Menu shortcuts:
         EXPECTED: Veyra Sounds, Veyra Overlay, Setup Audio Driver (Advanced), Uninstall
         FAILURE:  Any shortcut missing — installer CreateShortcut step failed
```

---

## Section 2 — UI Launch and Connectivity

```
[ ] 2.1  Launch veyra.exe from Start Menu (as normal user)
         EXPECTED: Window appears; brand LED turns green within 3 seconds
         FAILURE:  LED stays amber → service not running or pipe session rejected
         ROLLBACK: sc start VeyraAudioService

[ ] 2.2  Close and re-open veyra.exe — second-instance check
         EXPECTED: Tray icon flashes (existing instance raised); no second window
         FAILURE:  Two windows appear — moreThanOneInstanceAllowed() is returning true

[ ] 2.3  Right-click tray icon while app is open
         EXPECTED: Menu: Open, Mini Mode, Master Enabled (checked), Quit
         FAILURE:  No tray icon — TrayIcon constructor or setIconImage failed

[ ] 2.4  Mini Mode — click the compact icon in the top bar
         EXPECTED: Compact widget appears; drag to reposition; visualizer animates
         FAILURE:  Mini window blank or frozen — check GDI compositor

[ ] 2.5  Theme switching — Settings → Appearance → cycle through all 11 themes
         EXPECTED: UI colour changes immediately for all 11 themes; no repaint glitches
         FAILURE:  Colour change partial or frozen — palette_ not propagated to all components

[ ] 2.6  DPI scaling — if possible, set display to 125% or 150% DPI
         EXPECTED: All text and controls scale correctly; no clipping or overflow
         FAILURE:  Text clips or controls overlap — getLocalBounds() math is DPI-unaware

[ ] 2.7  Dark mode consistency — all 11 themes should feel consistent with
         Windows system dark mode; no bright-white flash on transition
         EXPECTED: All transitions smooth; acrylic backdrop visible through glass panels
         FAILURE:  White flash on theme change — setOpaque(true) somewhere in the tree
```

---

## Section 3 — APO Render Path

```
[ ] 3.1  Associate APO with the wired/built-in output endpoint:
           Run "Setup Audio Driver (Advanced)" from Start Menu
           — OR —
           pwsh installer/driver/associate-apo.ps1 (pick your endpoint)
         EXPECTED: "Written." and AudioSrv restarts
         FAILURE:  "Endpoint GUID not found" → run associate-apo.ps1 -ListEndpoints

[ ] 3.2  Verify APO loaded into audiodg.exe:
           Process Explorer → audiodg.exe → lower pane → filter "veyra-apo"
         EXPECTED: veyra-apo.dll in the DLL list
         FAILURE:  DLL absent → check Event Viewer → System → Source=AudioDg
                   Common causes: test-signing off, COM not registered, FxProperties wrong

[ ] 3.3  Play audio through the associated endpoint (any app: Spotify, YouTube, browser)
         EXPECTED: Audio plays normally; no glitches, clicks, or silence
         FAILURE:  Silence → APO is crashing; check audiodg EventLog
                   Glitches → buffer size mismatch; check APOProcess latency

[ ] 3.4  Master EQ — drag the 10 EQ sliders on the Home screen
         EXPECTED: Frequency response changes audible in real time (< 200 ms)
         FAILURE:  No effect → seqlock or shared memory not connected

[ ] 3.5  Parametric EQ — switch to Parametric mode; drag a node to +6 dB at 2 kHz
         EXPECTED: Audible 2 kHz boost; curve displayed on EQ card
         FAILURE:  No effect → VeyraParamsPayload.parametricMode not propagating

[ ] 3.6  Spatial — Gamer Mode → Spatial Audio → Cinematic (HRTF)
         EXPECTED: Out-of-head widening; binaural effect audible on headphones
         FAILURE:  No change → VirtualSurround/HrtfDatabase not loading KEMAR set
                   Verify: hrtf/diffuse/ directory present in install dir

[ ] 3.7  Reverb — home screen Reverb knob to 50%
         EXPECTED: Clear reverb tail; decay audible on percussion
         FAILURE:  No change → reverbAmount not in VeyraParamsPayload

[ ] 3.8  Limiter ceiling — set Volume to 250%, enable True-Peak Limiter
         EXPECTED: Output peaks stay below −0.3 dBFS; no distortion on transients
         FAILURE:  Audible clipping → limiter ceiling not holding; check Limiter.h

[ ] 3.9  APO CPU usage — play music for 60 seconds, observe Task Manager
         EXPECTED: audiodg.exe < 3% CPU on Ryzen 5 5600 or equivalent
         FAILURE:  > 10% CPU → denormal flood (check FTZ/DAZ), infinite loop, or
                   excessive block processing in DspChain

[ ] 3.10 Bass boost — Home → Bass slider to +6 dB
         EXPECTED: Audible bass increase without distortion (limiter prevents clipping)
         FAILURE:  No change → ToneControls not receiving the param update
```

---

## Section 4 — Capture APO / Microphone

```
[ ] 4.1  Associate capture APO:
           pwsh installer/driver/associate-apo.ps1 -Capture
         EXPECTED: "Written." for the default microphone

[ ] 4.2  Open a recording app (Voice Recorder, Audacity, Discord); speak normally
         EXPECTED: Microphone works; no silence or constant noise

[ ] 4.3  RNNoise — Settings → Microphone → Noise Suppression ON
         EXPECTED: Background noise (keyboard, HVAC) visibly reduced in the recording
         FAILURE:  No change → RnnoiseDenoiser not applied; check MicPublisher log

[ ] 4.4  Noise Gate — toggle Noise Gate off → on while speaking
         EXPECTED: Gate active: silence between words cut; gate off: noise floor heard
         FAILURE:  No change → noiseGate flag not propagating from Settings to APO

[ ] 4.5  AGC — speak quietly then loudly; watch recording level meter
         EXPECTED: Output level stays approximately consistent (−16 LUFS target)
         FAILURE:  Large swings → AGC loop not running; check MicPublisher VoiceChain
```

---

## Section 5 — Audio Bridge (Advanced Bluetooth Fallback)

```
[ ] 5.1  Connect Bluetooth headphones
         EXPECTED: Device appears in Devices → Audio Bridge Output dropdown

[ ] 5.2  Configure Audio Bridge:
           Source = CABLE Output (VB-CABLE)
           Output = Bluetooth headphones
           Enable = on
         EXPECTED: Veyra App shows "Bridge Active"

[ ] 5.3  Set Windows default output to CABLE Input (VB-CABLE)
         Play audio in any app
         EXPECTED: Audio plays through Bluetooth headphones with DSP applied

[ ] 5.4  Apply EQ change with Bridge active
         EXPECTED: Effect audible within 200 ms (same as APO path)
         FAILURE:  No effect → Bridge DSP chain not receiving param updates

[ ] 5.5  Bridge hot-plug — disconnect Bluetooth headphones while Bridge is active
         EXPECTED: Veyra shows an error state; recovers automatically when headphones reconnect
         FAILURE:  Service crash → AudioBridge stop/restart logic broken
```

---

## Section 6 — Overlay and Gamer Mode

```
[ ] 6.1  Launch veyra-overlay.exe while service is running
         EXPECTED: Transparent radar HUD appears; always-on-top; click-through

[ ] 6.2  Enable Gamer Mode in the main UI (toggle in Gamer Mode screen)
         EXPECTED: Radar activates; sweep arm visible

[ ] 6.3  Play audio with distinct directional cues (binaural game audio or test tones
         panned hard left/right) for 10+ seconds
         EXPECTED: Blips appear on radar at correct angular positions
         FAILURE:  No blips → TrackerService not writing to VeyraTracker_v1;
                   check service log for "TrackerService: started"

[ ] 6.4  Switch radar style (Competitive / Rich / Compass) in Gamer Mode settings
         EXPECTED: Overlay redraws in the selected style within one frame

[ ] 6.5  Overlay on second monitor — drag it to a second display
         EXPECTED: Overlay stays always-on-top; renders correctly on second DPI
         FAILURE:  Disappears behind other windows → WS_EX_TOPMOST not set

[ ] 6.6  Verify overlay is click-through:
         EXPECTED: Mouse clicks pass through the overlay window to apps beneath it
         FAILURE:  Clicks captured → WS_EX_TRANSPARENT not set on the layered window
```

---

## Section 7 — Visualizer and Analyzer

```
[ ] 7.1  Play music; observe the analyzer bars on the Home screen
         EXPECTED: Bars animate in sync with the music
         FAILURE:  Bars static → VeyraAnalyzerData shared memory not connected;
                   check that the APO (or AudioBridge) is writing to the analyzer ring

[ ] 7.2  Home screen VU meters — play a loud track
         EXPECTED: L/R VU meters and peak indicators animate; clip LED lights on very loud content

[ ] 7.3  Mini Mode visualizer — enter Mini Mode while music plays
         EXPECTED: Spectrum visible behind the volume slider in the mini bar
         FAILURE:  Mini bars all at zero → setVisualizerBars() not being called from timerCallback
```

---

## Section 8 — Device Hot-plug

```
[ ] 8.1  USB headphones — unplug and re-plug while audio plays (APO path)
         EXPECTED: Audio drops briefly; recovers on re-plug without service crash
         FAILURE:  Service crashes → AudioBridge device-lost error handling

[ ] 8.2  Bluetooth — connect while Bridge is active with a different output
         EXPECTED: New device appears in Devices dropdown; can switch to it
         FAILURE:  Dropdown doesn't update → DevicesScreen not polling device list

[ ] 8.3  Default device change — open Windows Sound Settings; change default output
         while Veyra is running
         EXPECTED: Veyra's preferred output check (~0.5 Hz poll) re-asserts it
         NOTE:     Only applies if "Preferred Output Device" is set in Veyra Devices screen
```

---

## Section 9 — Sleep and Resume

```
[ ] 9.1  Put the machine to sleep (Start → Power → Sleep) while Veyra is running
         EXPECTED: Machine sleeps; on resume, service restarts audio within 5 seconds

[ ] 9.2  Resume + play audio
         EXPECTED: Audio plays with effects; LED still green (reconnected)
         FAILURE:  LED amber after resume → pipe server did not restart after sleep;
                   consider adding a power-notification handler to the service

[ ] 9.3  Sleep Timer (if configured) — set Sleep Timer to 1 minute in Settings → Loudness
         EXPECTED: Volume fades to zero after 1 minute; system does not sleep (timer is volume-only)
         FAILURE:  Volume unchanged → SleepTimerService not receiving LoudnessConfig
```

---

## Section 10 — Sound Lab

```
[ ] 10.1 Sound Lab → Tone Generator — set frequency to 1000 Hz, press Start
          EXPECTED: Clear 1 kHz sine tone through the selected output

[ ] 10.2 Frequency Sweep — press Start; confirm audible sweep from low to high

[ ] 10.3 Speaker Test (7.1) — channel L, R, C each in sequence
          EXPECTED: Each channel plays from the correct direction

[ ] 10.4 Microphone level — Sound Lab → Microphone; speak into the mic
          EXPECTED: Level meter reacts to voice

[ ] 10.5 Hearing Test — complete the 6-step wizard
          EXPECTED: Personalised EQ applied and visible on Home EQ card as parametric bands
```

---

## Section 11 — Uninstall

```
[ ] 11.1 Run the uninstaller from Programs & Features / Settings → Apps → Veyra Sounds
          EXPECTED: Uninstall wizard runs; completes without errors

[ ] 11.2 Verify APO associations removed:
           pwsh installer/driver/associate-apo.ps1 -ListEndpoints
          EXPECTED: All endpoints show "no Veyra APO"

[ ] 11.3 Verify COM unregistered:
           reg query "HKCR\CLSID\{7E9C2B14-3F6A-4D8E-9B21-5C0A1F2E3D44}"
          EXPECTED: "ERROR: The system was unable to find the specified registry key"

[ ] 11.4 Verify service removed:
           sc query VeyraAudioService
          EXPECTED: "FAILED 1060: The specified service does not exist"

[ ] 11.5 Play audio after uninstall
          EXPECTED: Audio plays normally without any Veyra processing

[ ] 11.6 Verify no orphan entries in Programs & Features
          EXPECTED: "Veyra Sounds" no longer listed
```

---

## Section 12 — 8-hour Soak Test

```
[ ] 12.1 Run veyra-service.exe + veyra.exe for 8 hours with music playing continuously
          Check at the end:
          - Task Manager: veyra-service.exe RAM growth < 5 MB vs baseline
          - Task Manager: audiodg.exe CPU < 3% (average, not peak)
          - No crash reports in %ProgramData%\Veyra\crashes\
          - LED still green
          FAILURE:  RAM growth → memory leak; CPU spike → denormal flood or busy-wait
```

---

## Rollback Guide

### Rollback: APO association only

If audio stops after APO association but before uninstall:

```powershell
# Elevated:
pwsh installer/driver/associate-apo.ps1 -ListEndpoints
# Find the endpoint with Veyra CLSID and note its GUID, then:
pwsh installer/driver/associate-apo.ps1 -EndpointGuid "{YOUR-GUID}" -Unassociate
# Restart audio:
net stop AudioSrv; net start AudioEndpointBuilder; net start AudioSrv
```

### Rollback: full uninstall

If the NSIS uninstaller is unavailable, use the manual steps in [TROUBLESHOOTING.md](TROUBLESHOOTING.md).

### Rollback: service only

```cmd
sc stop VeyraAudioService
sc delete VeyraAudioService
```

### Rollback: Windows Audio to pre-Veyra state

If Windows Audio itself is behaving unexpectedly after removing Veyra:

```cmd
# Reset the Windows Audio service:
net stop AudioSrv
net stop AudioEndpointBuilder
net start AudioEndpointBuilder
net start AudioSrv
```

If still broken, open Device Manager → Sound, video and game controllers → right-click the audio device → Update driver → Browse my computer → Let me pick → choose the same driver → OK. This forces a driver reinitialisation.

---

## Recording results

Use this table when executing the checklist:

| Section | Items | Pass | Fail | Notes |
|---------|-------|------|------|-------|
| 1 — Install | 1.1–1.6 | | | |
| 2 — UI | 2.1–2.7 | | | |
| 3 — APO Render | 3.1–3.10 | | | |
| 4 — Capture APO | 4.1–4.5 | | | |
| 5 — Audio Bridge | 5.1–5.5 | | | |
| 6 — Overlay | 6.1–6.6 | | | |
| 7 — Visualizer | 7.1–7.3 | | | |
| 8 — Hot-plug | 8.1–8.3 | | | |
| 9 — Sleep | 9.1–9.3 | | | |
| 10 — Sound Lab | 10.1–10.5 | | | |
| 11 — Uninstall | 11.1–11.6 | | | |
| 12 — Soak | 12.1 | | | |
| **Total** | **54** | | | |

**Pass rate required for v1.0 tag:** All Sections 1–11 must have 0 failures. Section 12 soak is recommended but not a hard blocker for RC tagging.
