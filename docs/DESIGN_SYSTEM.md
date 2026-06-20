# Veyra Sounds — Design System & UI Specification

**Source of truth:** the screenshots in [`Reference theme/`](../Reference%20theme/)
(`home`, `presets`, `apps`, `devices`, `sounds`, `gamermode`, `settings`,
`minimode`). `Settings1.png` is an older alternate mock — only its *content*
(Launch on Startup, Minimize to Tray, Check for Updates, Buffer Size, Sample
Rate, Hardware Acceleration) is adopted; its flatter styling is **not**.

This document recreates that design language as an implementable system and maps
every requested feature onto it without inventing a new style. Where a feature
isn't already in a screenshot, it is placed as the smallest natural extension of
an existing card/table/tab pattern.

> Implementation note: the repo already ships most of this
> (`apps/veyra-ui/Source/Theme`, `…/Components`, `…/Screens`). Token and
> component names below reference those files. Code-vs-reference gaps are listed
> in [§12](#12-reconciliation--gaps-vs-current-build).

---

## 1. Design principles (read from the screenshots)

1. **One dark indigo room, glass furniture.** A single deep blue-violet gradient
   canvas; every surface is a frosted-glass card floating on it with a soft
   shadow and a faint top light-leak. Depth comes from blur + elevation, never
   from hard borders.
2. **Violet is the only loud color.** A single vivid violet accent marks the one
   primary action / active state per view. Everything else is greyscale-on-navy.
3. **Lime-green = signal/health only.** Meters, "Active" dots, "ENGINE ACTIVE",
   mic input level. Never decorative.
4. **Three type voices.** Tracked uppercase Orbitron for identity/section
   titles; Inter for everything you read; JetBrains Mono for every number.
5. **Calm motion.** Sliding segmented selectors, sliding toggles, arc fills,
   gentle hover lift, long theme cross-fades. Nothing bounces.

---

## 2. Color palette (extracted)

Hex values read from the screenshots, mapped to the existing `Palette` tokens in
[`Theme/DesignTokens.h`](../apps/veyra-ui/Source/Theme/DesignTokens.h). These are
the **Midnight** (default) values; the 10 alternate themes re-tint `accent*` and
the `bg*` ramp only.

| Role | Hex (extracted) | `Palette` token |
| --- | --- | --- |
| App backdrop (top-left of gradient) | `#15172A` | `bgApp` |
| App backdrop (bottom-right) | `#0A0B12` | `bgCanvas` |
| Glass card fill | `#191B2B` @ ~82% | `bgGlass` |
| Glass card (hover) | `#1F2236` | `bgGlassHover` |
| Glass card (elevated: knob/preset cards) | `#212438` | `bgGlassElevated` |
| Input / track trough | `#0E0F1A` | `bgInput` |
| Modal scrim | `#05060B` @ 72% | `bgModalScrim` |
| Hairline stroke | `#FFFFFF` @ 6% | `strokeDefault` |
| Hover stroke | `#FFFFFF` @ 12% | `strokeHover` |
| Active/selected stroke (violet) | `#7C5CFF` @ 70% | `strokeActive` |
| Top light-leak | `#FFFFFF` @ 10% | `strokeLightLeak` |
| Text primary | `#F2F3F7` | `textPrimary` |
| Text secondary | `#A0A3B8` | `textSecondary` |
| Text tertiary / micro-labels | `#5C6075` | `textTertiary` |
| **Accent primary (violet)** | `#7C5CFF` | `accentPrimary` |
| Accent hover / active | `#8E72FF` / `#6A4AF0` | `accentPrimaryHover/Active` |
| Accent glow (behind active) | `#7C5CFF` @ 22% | `accentGlow` |
| Accent secondary (micro-caps) | `#9B8Cff`/`#784BA0` | `accentSecondary` |
| Success / meters / ACTIVE | `#B6E84F` | `success` |
| Warning / active-input border | `#E8D44F` | `warning` |
| Danger / clip / Anti-cheat alert | `#FF5C6A` | `danger` |
| Boosted value (e.g. "110%") | `#8E72FF` | `accentPrimary` |

**Gradient:** the canvas is a 135° linear blend `bgApp → bgCanvas` with a faint
radial violet bloom (`accentGlow`, ~6%) behind the top-left wordmark. Match
[`Graphics/GlassBackground`](../apps/veyra-ui/Source/Graphics/).

---

## 3. Typography

Three embedded families (already in the build): **Orbitron**, **Inter**,
**JetBrains Mono** — see [`Theme/Fonts.h`](../apps/veyra-ui/Source/Theme/Fonts.h)
(`fonts::display / body / mono`).

| Token | Family | Usage in screenshots | Size / tracking |
| --- | --- | --- | --- |
| `display/wordmark` | Orbitron 700 | "VEYRA / VEYRA SOUNDS", "PRESETS", "PER-APP RULES", "GAMER MODE", "SPEAKER TEST" | 18–26px, +6% tracking, UPPERCASE |
| `heading` | Inter 600 | "Settings", "Device Routing", "Music", card titles ("Sennheiser HD 600") | 18–34px |
| `body` | Inter 500 | descriptions, nav labels, table cells | 13–15px |
| `micro-label` | Inter 600 / JetBrains Mono | tracked caps: `PARAMETERS`, `PRESET`, `VOL`, `DETECTION`, `MASTER VOL`, `STEP 1/4` | 10–11px, +8% tracking, UPPERCASE, `textTertiary` |
| `numeric` | JetBrains Mono 500 | every readout: `75%`, `-18 dB`, `+4 dB`, `256 smp`, `72`, freq labels `32…16k` | 11–15px (mini-mode master = 40px) |

RTL (Arabic): mirror the sidebar to the right, flip table column order and slider
fill direction; numerics stay LTR.

---

## 4. Spacing, radius, elevation

- **Grid:** 8px base. Card padding **24px**; section gaps **20px**; intra-card
  row gaps 12–16px.
- **Sidebar width:** 200px (min layout 180px). **Content max-width** ~ 1180px,
  centered, with the sidebar fixed.
- **Radius:** cards **16px**; elevated knob/preset cards **18px**; pills/buttons
  **10px**; segmented control **12px** outer / 9px thumb; full-round knobs &
  toggles.
- **Elevation/shadow:** one soft ambient shadow per card —
  `y+8, blur 24, #000 @ 35%`; elevated cards add a second `y+2, blur 6 @ 25%`.
- **Glass recipe** (see `Components/GlassPanel.h` / `Graphics/Glass.h`):
  1. blurred backdrop slice, 2. `bgGlass` tint, 3. 1px inner stroke
  (`strokeDefault`, brighter on the top edge = `strokeLightLeak`), 4. ambient
  shadow, 5. optional `accentGlow` bloom when active/selected.

---

## 5. Iconography

Thin (1.5px) line icons, rounded caps, `textSecondary` default / `textPrimary`
or `accentPrimary` when active — matches the repo's vector
[`Graphics/Icons`](../apps/veyra-ui/Source/Graphics/). Set used: home, presets
(sliders), apps (grid), devices (chip), sound-lab (flask), gamer (gamepad), gear,
search, bell, close, expand (mini), headphone/speaker/tv/mic/usb/hdmi badges,
three-dot menu, plus, check, play. Device-type badges (`USB`, `BUILT-IN`, `HDMI`)
are tiny tracked-caps pills, not glyphs.

---

## 6. Component library

Each maps to an existing JUCE class; states are read from the screenshots.

1. **Sidebar nav item** — icon + label; states: default / hover (`bgGlassHover`)
   / **active** (filled `bgGlassActive` pill + `accentPrimary` text + left
   accent rail + soft glow). `Home/Sidebar`.
2. **Top bar** — drag region; right cluster: search, bell (with dot badge), gear,
   close. Left: wordmark + version + connection LED. `Home/TopBar`.
3. **Glass card / elevated card** — §4 recipe. `Components/GlassPanel`.
4. **Segmented control** — pill track with a sliding violet-tinted thumb;
   2–3 segments (Graphic/Parametric, Competitive/Rich, Cinematic/Competitive).
   `Components/SegmentedControl`.
5. **Toggle switch** — track + sliding knob; off = grey, on = violet (some on =
   green for "ON" status). `Components/ToggleSwitch`.
6. **Knob** — circular arc-fill, center label + mono value under; "danger zone"
   past threshold (Volume Gain >200%). `Components/Knob`.
7. **EQ band slider** — vertical track, circular violet handle; handles joined by
   the **response curve** overlay; freq label under.
   `Components/EqBandSlider` + `EqualizerCard`.
8. **Primary button** — violet fill, white label ("Apply Preset", "Start Test",
   "+ Add App", "Apply Changes"). **Secondary button** — glass fill + hairline
   ("Duplicate", "Export", "Reset to Defaults").
9. **Horizontal meter** — segmented lime bar on dark trough (IN/OUT, mic input
   level, device VOL). Clip segment turns `danger`.
10. **Preset card** — title + category badge + optional ✓, mini-waveform line,
    three-dot menu; selected = `accentGlow` border. `Screens/PresetsScreen`.
11. **Device card** — header (type icon + badge), name, status dot + label,
    divider, PRESET/PROFILE row, VOL/level meter; **active = colored border**
    (violet output / yellow input). `Screens/DevicesScreen`.
12. **Data table row** — Apps page: icon+name (+badge), columns, status dot.
13. **Detail drawer** — right-side panel (Presets) with params list, info, tags,
    actions.
14. **Tab bar** — horizontal pill tabs (Sound Lab tools). New small component.
15. **Mini window** — frameless: dot-titlebar + expand, big master knob, IN/OUT
    meters, mini visualizer, 3 preset pills, status footer. `MiniWindow`.

---

## 7. Navigation structure

```
Left sidebar (persistent)
  VEYRA · vX.Y.Z + connection LED
  ├ Home
  ├ Presets
  ├ Apps
  ├ Devices
  ├ Sound Lab
  ├ Gamer Mode
  ├ Settings            (bottom group)
  └ [ MINI MODE ]       (bottom button)
Top bar (persistent): search · bell · gear · close

Secondary navigation patterns (per screen, reused — not new pages):
  Presets    → category tree column + right detail drawer
  Sound Lab  → top tab bar (7 tools)
  Settings   → vertical sub-nav (General · Audio Engine · Hotkeys · Updates · About)
  Gamer Mode → 2×2 card dashboard + a section row beneath
  Home       → stacked cards (visualizer · EQ · effects)
  Devices    → "Output Devices" / "Input Devices" sections of cards
  Apps       → single table
```

---

## 8. Screen-by-screen specification

For each: **layout hierarchy**, **feature placement**, and **why it fits**.

### 8.1 Home  (`home.png`)
```
[sidebar] [ content
  Card: Visualizer            (top-left mode chip "Bars ▾"; bottom-left "FPS")
  Card: Equalizer             (title left; right: Graphic|Parametric seg + "Show curve" toggle)
        10 vertical band sliders joined by response curve; freq labels 32…16k
  Row of 6 elevated knob cards: Bass Boost · Treble · Volume Gain · Stereo Width · Reverb · Compression
]
```
- **Master volume / enable / device selector / active-preset / output meter /
  audio statistics** → the **top bar** holds master enable + master volume +
  active-preset chip + device selector; the visualizer card carries the live
  spectrum, and a thin **output meter** sits under the mode chip. *Why:* the
  screenshots already keep global controls in the top bar and metering inside the
  visualizer card — no new surface needed.
- **Graphic + Parametric EQ (up to 16 bands; bell/shelf/HPF/LPF)** → the
  Graphic|Parametric segmented control already present; Parametric mode swaps the
  10 fixed bands for up to 16 draggable nodes on the same curve, with a small
  per-node type selector (bell/HS/LS/HPF/LPF) in a popover. *Why:* reuses the
  exact EQ card + curve.
- **Effects (Bass, Treble, Clarity, Compression, Stereo Width, Surround, Reverb,
  Delay, Loudness, Limiter)** → the knob row. Six are shown; the rest live behind
  the **"More Effects"** tile (already in `HomeScreen`) which opens an effects
  rack using the same knob cards. *Why:* a knob card is the established atom.

### 8.2 Presets  (`presets.png`)
```
[sidebar][ category tree | card grid | detail drawer ]
  tree: All Presets · Built-in(Music/Movie/Gaming/Voice/Bass Heavy) · Custom · Recently Used · Favorites
  grid header: filter "All ▾" · search · grid/list toggle
  cards: Music✓(BUILT-IN) · Cinema Night · FPS Footstep Boost(GAMING) · Podcast Studio(VOICE) · My Lo-fi(CUSTOM·YOU) · + Create New
  drawer: name + source ▾ + close · LIVE mini-viz · PARAMETERS · INFO · tags · [Apply Preset] [Duplicate][Export]
```
- **Built-in set (Music/Gaming/Movie/FPS/Bass/Vocal/Streaming/Night Mode)** →
  the grid + tree categories. **Create/Edit/Save/Import/Export** → "Create New
  Preset" card + drawer actions (Import lives next to the grid/list toggle as an
  icon, matching the existing toolbar). *Why:* drawer + grid already cover it.

### 8.3 Apps  (`apps.png`)
```
"PER-APP RULES" + subtitle | right: "Per-App Switching" toggle + [+ Add App]
Table: APP | DETECTION | PRESET | VOLUME | AUTO-MUTE | STATUS
  rows: Spotify · Discord · Chrome · Valorant(⛨Anti-cheat) · OBS Studio
```
- **Auto-switching / detection rules / priority** → the table is the rule list;
  DETECTION cell = Foreground only / Audio session / Both. **Priority** is the
  row order (drag to reorder) — add a faint drag-handle on hover, the smallest
  possible extension. **Add App** opens a sheet (app picker + detection + preset
  + volume + auto-mute). *Why:* one table, no new page.

### 8.4 Devices  (`devices.png`)
```
"Device Routing"
▍OUTPUT DEVICES   → cards: Sennheiser HD 600(USB·Active, FPS Footstep Boost, 75%) · Realtek Speakers · LG OLED TV(HDMI)
▍INPUT DEVICES    → cards: Shure MV7(USB·Active, Streaming, -18dB meter) · Realtek Mic Array
```
- **Device profiles (Headphones/Speakers/USB DAC/Bluetooth), each storing EQ /
  effects / volume / preset** → each device card already shows PRESET + VOL;
  clicking a card opens its stored profile (preset + per-device EQ/effects/volume)
  in the same detail-drawer pattern as Presets. Bluetooth is just another card
  with a BT badge. *Why:* device card + drawer reuse.

### 8.5 Sound Lab  (`sounds.png`)
```
Top tab bar: Speaker Test · 7.1 Surround Test · Microphone Test · Frequency Sweep · Hearing Range · Polarity/Phase · Noise Generator
Active tab body: big centered test card (e.g. SPEAKER TEST diagram + [Start Test]) + small "Interactive Testing" helper card
```
- **All 7 tools** are the tabs (the screenshot's exact set). Each tab is one
  centered glass card + a helper card. *Why:* the tab bar is the established
  pattern here. **Note:** Night Mode / Loudness do **not** belong here — they
  live in Gamer Mode (§8.6) and Effects (§8.1).

### 8.6 Gamer Mode  (`gamermode.png`)
```
"GAMER MODE" + subtitle | right: MASTER TOGGLE
2×2 cards:
  SOUND TRACKER (●ACTIVE; radar with you-center + blips; Competitive|Rich seg; [Customize])
  SPATIAL AUDIO (HRTF: Cinematic|Competitive seg; Virtual Headset toggle; Intensity slider; "Test in Sound Lab")
  VOICE & MICROPHONE (profile ▾; RNNoise/Noise Gate/Echo Cancel/Voice Enhancer toggles; Side-tone; input meter)
  NIGHT MODE (toggle; Bass Reduction/Dialogue Emphasis/Peak Limit sliders; "Auto-enable 22:00–07:00 · Edit")
Section: PER-GAME PROFILES + "Scan installed games"
```
- **Gamer Mode** (auto game detection, competitive audio, footstep enhancement,
  voice clarity, performance mode) → Sound Tracker + Spatial + the Master Toggle
  + Per-Game Profiles. **Sound Tracker overlay HUD** (Circular/Half/Edge ·
  direction/distance/category/confidence · footsteps/gunshots/voice/vehicles/
  explosions) → the Sound Tracker card's Competitive|Rich selector + **Customize**
  (radar style + categories). **Microphone Studio** (NS/Gate/AEC/Voice EQ/
  De-esser/AGC/Side-tone + live input/waveform/spectrum; profiles Gaming/
  Streaming/Podcast/Meeting) → the Voice & Microphone card (profile dropdown =
  those profiles; meter = live input; waveform/spectrum open via the Mic Test in
  Sound Lab). **Spatial Audio** (Cinematic/Competitive/HRTF/Headphone
  optimization/Crossfeed) → the Spatial Audio card (Crossfeed is an extra row in
  that card for speaker mode). *Why:* this dashboard already hosts these four
  domains — every feature lands in an existing card.

### 8.7 Settings  (`settings.png` + content from `Settings1.png`)
```
"Settings" + subtitle
Sub-nav: General · Audio Engine · Hotkeys · Updates · About
Audio Engine panel: Hardware Acceleration · Low Latency Mode · Sample Rate seg(44.1/48/96/192) · Buffer Size slider(64–2048)
  + [Reset to Defaults] [Apply Changes]
```
- **Appearance · Audio Engine · Devices · Updates · Hotkeys · Language · Privacy
  · Advanced** → sub-nav items. From `Settings1.png`: **General** = Launch on
  Startup, Minimize to Tray, Check for Updates. Appearance (theme grid + opacity
  + background + reduce-motion) and Privacy (telemetry opt-in) are existing
  sub-pages. *Why:* one vertical sub-nav, no new chrome.

### 8.8 Mini Mode  (`minimode.png`)
```
Frameless: (•••) … V … (⤢)
Big circular MASTER knob "72%"
IN ▮▮▮▯  ▮▮▮▯ OUT
[mini visualizer]
[Gaming][Music][Studio] preset pills
● ENGINE ACTIVE                vX.Y.Z
```
Reuse `MiniWindow`; preset pills = quick-switch; status footer LED = engine
state.

---

## 9. Feature → location map (the full request, placed)

| Requested feature | Screen / element |
| --- | --- |
| Master vol/enable, device selector, active preset, output meter, stats | Home **top bar** + visualizer card |
| Live spectrum visualizer (8 modes) + fullscreen | Home visualizer card (mode chip) |
| Graphic EQ 10-band / Parametric ≤16 (bell/HS/LS/HPF/LPF) | Home Equalizer card (segmented) |
| Effects: Bass/Treble/Clarity/Compression/Width/Surround/Reverb/Delay/Loudness/Limiter | Home knob row + "More Effects" rack |
| Spatial: Cinematic/Competitive/HRTF/Headphone-opt/Crossfeed | Gamer Mode → Spatial Audio card |
| Presets built-in + create/edit/save/import/export | Presets (tree + grid + drawer) |
| Per-app profiles, auto-switch, detection, priority | Apps table (+ Add App sheet) |
| Device profiles (HP/Speakers/USB DAC/BT) storing EQ/FX/vol/preset | Devices cards + drawer |
| Microphone Studio (NS/Gate/AEC/Voice EQ/De-esser/AGC/Side-tone; profiles; live meters) | Gamer Mode → Voice & Microphone card + Sound Lab → Mic Test |
| Gamer Mode (auto-detect, competitive audio, footstep, voice clarity, perf) | Gamer Mode dashboard + Per-Game Profiles |
| Sound Tracker overlay (Circular/Half/Edge; dir/dist/cat/conf; categories) | Gamer Mode → Sound Tracker card → Customize; overlay process |
| Visualizers (Bars/Monstercat/Circular/Ferrofluid/Waveform/Particles/Wavy/3D) | Home visualizer mode chip |
| Sound Lab (Speaker/Surround/Sweep/Hearing/Phase/Noise/Mic tests) | Sound Lab tab bar |
| Night Mode (bass reduction/dialogue/peak limit/schedule) | Gamer Mode → Night Mode card |
| Settings (Appearance/Audio Engine/Devices/Updates/Hotkeys/Language/Privacy/Advanced) | Settings sub-nav |

---

## 10. Interaction & animation

| Element | Interaction | Motion |
| --- | --- | --- |
| Sidebar item | click → route | active pill cross-fades + glow ramps (160ms) |
| Segmented control | click segment | thumb slides to segment (180ms ease-out) |
| Toggle | click | knob slides + track color lerps (140ms) |
| Knob | drag vertical / wheel | arc fills continuously; value text live |
| EQ band | drag handle | curve re-splines live; "Show curve" fades overlay |
| Card hover | — | +2px lift + `bgGlassHover` + stroke→`strokeHover` (120ms) |
| Preset select | click card | `accentGlow` border blooms; drawer slides in (220ms) |
| Theme switch | Settings→Appearance | 400ms cross-fade between two themed renders |
| Meters / radar | live | meter ease ~60ms; radar blips fade-in/decay |
| Mini master knob | drag arc | white handle tracks; "%" updates |

All continuous **audio** params are smoothed in the DSP (5ms) so UI motion never
implies zipper noise. Honor **Reduce Motion** (freezes visualizer + disables
cross-fades) — already wired via `config.reduceMotion`.

---

## 11. Responsive behavior

- **≥1180px:** full layout (sidebar 200px + content as specced).
- **960–1180px (min):** content cards reflow — Home knob row wraps 3+3; Gamer
  2×2 stays; Presets drawer overlaps the grid as a slide-over instead of a third
  column; Apps table keeps APP+STATUS, collapses VOLUME/AUTO-MUTE into a row
  expander.
- **< 960px:** not a target (min window 960×620 per spec §7); sidebar may
  collapse to icons-only as a graceful fallback.
- **HiDPI:** all tokens scale by the display scale factor; glass blur radius is
  scaled too.
- **Mini mode:** fixed compact size, always-on-top.

---

## 12. Reconciliation — gaps vs current build

The engine/back-end for everything exists and is CI-green. UI structural deltas
to make the app match the screenshots:

1. **Sound Lab** — current screen shows Night Mode/Loudness/Sleep controls. Per
   reference it must be the **7-tool tab bar** (Speaker/Surround/Mic/Sweep/
   Hearing/Phase/Noise). → rebuild `SoundLabScreen` as a tab bar; move Night
   Mode to Gamer Mode.
2. **Gamer Mode** — current screen is enable + radar + sensitivity + detection.
   Per reference it's the **2×2 dashboard** (Sound Tracker · Spatial Audio ·
   Voice & Microphone · Night Mode) + Per-Game Profiles. → expand
   `GamerModeScreen`; fold the Settings→Spatial and Settings→Microphone cards in
   here (or share components).
3. **Home top bar** — add the device selector + output meter + active-preset chip
   to match `home.png` (master controls already there).
4. **Presets** — add the left **category tree** column and the **list/grid**
   toggle + filter dropdown to the existing grid + drawer.
5. **Apps** — present as the **table** layout (App/Detection/Preset/Volume/
   Auto-mute/Status) rather than stacked rule rows; add the header toggle + Add
   App.
6. **Devices** — promote the per-device **profile drawer** (EQ/FX/vol/preset);
   the Audio Bridge picker stays as an extra card.
7. **Settings** — add the **sub-nav** (General/Audio Engine/Hotkeys/Updates/
   About) and the Audio Engine panel (HW accel, low-latency, sample-rate
   segmented, buffer-size slider) from `settings.png`.
8. **Parametric EQ** (≤16 bands) and the full **8 visualizer** modes are the
   larger net-new UI builds.

These are UI-only; they reuse the components in §6 and break no visual rule.

---

_Authoritative reference: the `Reference theme/` screenshots. When in doubt,
match the pixels, then reuse the nearest component above._
