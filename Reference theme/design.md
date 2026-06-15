---
name: Veyra Sounds
version: 1.0.0
target: Windows 10/11 64-bit Desktop Application
framework: JUCE 7 (C++20)
license: GPLv3
tagline: Every Note Elevated

# ─── SEMANTIC COLOR TOKENS (USE THESE IN CODE) ───
colors:
  # Base layers
  bg-app: '#0A0B10'                       # deepest layer, behind everything
  bg-canvas: '#0F1117'                    # main canvas behind glass cards
  bg-glass: rgba(20, 22, 32, 0.55)        # default glass surface
  bg-glass-hover: rgba(28, 30, 42, 0.65)
  bg-glass-active: rgba(36, 38, 52, 0.75)
  bg-glass-elevated: rgba(32, 34, 48, 0.82)
  bg-modal-scrim: rgba(0, 0, 0, 0.55)
  bg-input: '#07080C'                     # darker than canvas, for input fields

  # Strokes (1px inner borders on glass surfaces)
  stroke-default: rgba(255, 255, 255, 0.08)
  stroke-hover: rgba(255, 255, 255, 0.14)
  stroke-active: rgba(124, 92, 255, 0.45)
  stroke-focus: rgba(124, 92, 255, 0.80)
  stroke-light-leak: rgba(255, 255, 255, 0.06)  # inset top edge "light hitting glass"

  # Text
  text-primary: '#F2F3F8'
  text-secondary: '#A8ABBD'
  text-tertiary: '#6B6F84'
  text-disabled: '#4A4D5E'
  text-on-accent: '#FFFFFF'

  # Accent (Brand Electric Violet)
  accent-primary: '#7C5CFF'               # PRIMARY BRAND COLOR
  accent-primary-hover: '#8E72FF'
  accent-primary-active: '#6A4AE8'
  accent-glow: rgba(124, 92, 255, 0.40)
  accent-glow-strong: rgba(124, 92, 255, 0.65)

  # Secondary Accent (Status Green — peak meters, "safe" levels)
  accent-secondary: '#A8C64E'
  accent-secondary-dim: '#86A03E'

  # Semantic
  success: '#4ADE80'
  success-bg: rgba(74, 222, 128, 0.15)
  warning: '#FBBF24'
  warning-bg: rgba(251, 191, 36, 0.15)
  danger: '#F87171'
  danger-bg: rgba(248, 113, 113, 0.15)
  info: '#60A5FA'
  info-bg: rgba(96, 165, 250, 0.15)

# ─── TYPOGRAPHY ───
typography:
  # Display face — uppercase only, used sparingly for brand/screen titles
  display:
    fontFamily: Nasalization
    fallback: [Eurostile, Michroma, Orbitron, sans-serif]
    weight: 400
    case: uppercase
    letterSpacing: 0.06em

  display-large:    { fontFamily: Nasalization, fontSize: 48px, weight: 400, lineHeight: 48px, letterSpacing: 0.06em }
  display-medium:   { fontFamily: Nasalization, fontSize: 32px, weight: 400, lineHeight: 32px, letterSpacing: 0.06em }
  display-small:    { fontFamily: Nasalization, fontSize: 24px, weight: 400, lineHeight: 24px, letterSpacing: 0.06em }
  display-xsmall:   { fontFamily: Nasalization, fontSize: 20px, weight: 400, lineHeight: 20px, letterSpacing: 0.06em }
  display-mini:     { fontFamily: Nasalization, fontSize: 16px, weight: 400, lineHeight: 16px, letterSpacing: 0.06em }

  # Body face — used for everything readable
  heading-h3:       { fontFamily: Inter, fontSize: 20px, weight: 600, lineHeight: 24px, letterSpacing: -0.02em }
  heading-h4:       { fontFamily: Inter, fontSize: 18px, weight: 600, lineHeight: 22px, letterSpacing: -0.02em }
  heading-h5:       { fontFamily: Inter, fontSize: 16px, weight: 600, lineHeight: 20px, letterSpacing: -0.01em }
  body-large:       { fontFamily: Inter, fontSize: 16px, weight: 400, lineHeight: 24px, letterSpacing: -0.01em }
  body:             { fontFamily: Inter, fontSize: 14px, weight: 400, lineHeight: 20px, letterSpacing: -0.01em }
  body-small:       { fontFamily: Inter, fontSize: 13px, weight: 400, lineHeight: 18px, letterSpacing: -0.01em }
  label:            { fontFamily: Inter, fontSize: 13px, weight: 500, lineHeight: 16px, letterSpacing: -0.01em }
  label-small:      { fontFamily: Inter, fontSize: 12px, weight: 500, lineHeight: 16px, letterSpacing: 0 }
  tooltip:          { fontFamily: Inter, fontSize: 12px, weight: 400, lineHeight: 16px, letterSpacing: 0 }
  button-large:     { fontFamily: Inter, fontSize: 14px, weight: 600, lineHeight: 20px, letterSpacing: 0 }
  button:           { fontFamily: Inter, fontSize: 13px, weight: 500, lineHeight: 16px, letterSpacing: 0 }
  nav-label:        { fontFamily: Inter, fontSize: 14px, weight: 500, lineHeight: 16px, letterSpacing: 0 }

  # Numerals — for all live numeric readouts (dB, Hz, %, ms)
  numeric-large:    { fontFamily: JetBrainsMono, fontSize: 14px, weight: 500, lineHeight: 18px, tabularNumbers: true }
  numeric:          { fontFamily: JetBrainsMono, fontSize: 13px, weight: 400, lineHeight: 16px, tabularNumbers: true }
  numeric-small:    { fontFamily: JetBrainsMono, fontSize: 11px, weight: 400, lineHeight: 14px, tabularNumbers: true }
  numeric-caps:     { fontFamily: JetBrainsMono, fontSize: 11px, weight: 700, lineHeight: 16px, letterSpacing: 0.1em, case: uppercase }

# ─── CORNER RADII (FIXED PER ELEMENT TYPE) ───
radii:
  none: 0
  xs: 6px            # slider tracks
  sm: 8px            # tooltips, small badges
  md: 10px           # buttons, inputs, dropdowns
  lg: 16px           # cards, panels (DEFAULT for major surfaces)
  xl: 20px           # modals
  '2xl': 24px        # splash screen content container
  pill: 999px        # toggles, chips, segmented controls

# ─── SPACING (4PX BASE UNIT) ───
spacing:
  base: 4px
  '0': 0
  xs: 4px
  sm: 8px
  md: 12px
  lg: 16px
  xl: 20px
  '2xl': 24px        # default panel inner padding
  '3xl': 32px
  '4xl': 40px
  '5xl': 48px
  '6xl': 64px

# ─── DIMENSIONS ───
dimensions:
  window-default: { width: 1180, height: 760 }
  window-min: { width: 960, height: 620 }
  window-mini: { width: 320, height: 80 }
  window-onboarding: { width: 720, height: 540 }
  window-gamer-hud: { width: 180, height: 220 }
  topbar-height: 56
  sidebar-expanded-width: 220
  sidebar-collapsed-width: 64
  modal-small: 480
  modal-medium: 640
  modal-large: 800

# ─── ELEVATION (SHADOWS) ───
elevation:
  level-0: none
  level-1: '0 8px 32px rgba(0, 0, 0, 0.35), inset 0 1px 0 rgba(255, 255, 255, 0.06)'
  level-2: '0 12px 40px rgba(0, 0, 0, 0.45)'
  level-3: '0 16px 48px rgba(0, 0, 0, 0.55)'
  level-4: '0 24px 64px rgba(0, 0, 0, 0.65)'
  glow-accent: '0 0 24px rgba(124, 92, 255, 0.40)'
  glow-accent-strong: '0 0 32px rgba(124, 92, 255, 0.65)'
  glow-danger: '0 0 24px rgba(248, 113, 113, 0.40)'
  glow-success: '0 0 16px rgba(74, 222, 128, 0.30)'

# ─── ANIMATION ───
animation:
  easing-standard: cubic-bezier(0.32, 0.72, 0, 1)
  easing-decelerate: cubic-bezier(0, 0, 0.2, 1)
  easing-accelerate: cubic-bezier(0.4, 0, 1, 1)
  duration-instant: 80ms
  duration-fast: 180ms        # default for most transitions
  duration-medium: 280ms      # modal mounts, panel slides
  duration-slow: 400ms        # theme cross-fades
  visualizer-fps: display-refresh  # 60/120/144 dynamic
---

# Veyra Sounds — Design System

> Reference document for all UI implementation in Phase 4+. Match every measurement, color, and component spec exactly. JUCE-specific implementation hints embedded inline.

---

## 1. Brand Identity

### 1.1 Personality
**Advanced Harmonic Engineering** — the cold precision of studio hardware fused with the fluid, ethereal nature of digital soundscapes. Calm, technical, futuristic, quietly powerful. Never gamer-loud. Never enterprise-sterile.

Reference points: Linear (clarity), Arc Browser (motion), Native Instruments Komplete Kontrol (premium hardware feel), Stripe Dashboard (density without clutter).

### 1.2 Voice (UI Copy)
- Short sentences, 1–8 words for labels.
- No exclamation marks. No emoji in production UI.
- Technical terms used precisely: "Equalizer" not "Sound Tuner".
- Confidence without arrogance. State what is.
- Good: *"Preset saved."* / *"Anti-cheat protected game detected."*
- Bad: *"Boost your sound experience!"* / *"AMAZING preset created!"*

### 1.3 Logo
Continuous ribbon-like monogram fusing V and A into a single flowing geometric form, with a small floating circle beneath the apex. Brand violet gradient: `#7C5CFF → #5B3FE4 → #4A2FCF`.

**Placement sizes:**
- Top utility bar: 28×28 with wordmark beside
- System tray: 16×16 monochrome
- Splash: 128×128
- About modal: 64×64
- Fullscreen visualizer watermark: 32×32 at 30% opacity

### 1.4 Wordmark
"VEYRA SOUNDS" in **Nasalization** Regular, **uppercase**, letter-spacing `+0.06em`. Always uppercase. Never bold or italic.

---

## 2. Visual Direction — Refined Glassmorphism

The foundation is **deep, multi-layered dark surfaces** with high-index refraction and saturation. Glass surfaces are not a theme variant — they are the structural language of the entire app.

### 2.1 Glass Surface Recipe (MANDATORY for all panels)

```css
background: rgba(20, 22, 32, 0.55);
backdrop-filter: blur(24px) saturate(140%);
-webkit-backdrop-filter: blur(24px) saturate(140%);
border: 1px solid rgba(255, 255, 255, 0.08);
border-radius: 16px;
box-shadow:
  0 8px 32px rgba(0, 0, 0, 0.35),
  inset 0 1px 0 rgba(255, 255, 255, 0.06);
```

### 2.2 JUCE Implementation Note for Glass

JUCE has no native `backdrop-filter`. Implement glass rendering as:

1. Capture the area behind each glass card as a `juce::Image` snapshot of the background composition (desktop wallpaper + Veyra ambient layer).
2. Apply gaussian blur via `juce::ImageEffects::applyStackBlur(image, 24)`.
3. Apply 140% saturation through HSL pixel-mapping (custom — JUCE doesn't have built-in saturation filter).
4. Composite the result with `bg-glass` tint color over the top.
5. Draw 1px `stroke-default` inner border.
6. Draw 8px outer shadow with `inset 0 1px 0 stroke-light-leak` for the top "light-leak" edge.

**Cache aggressively.** Only re-blur when the background image changes (desktop wallpaper change, or every 30 seconds for refresh). Re-blurring every frame will tank performance.

### 2.3 Background Treatment

Behind all glass panels:
1. Base layer `bg-app` (`#0A0B10`)
2. Blurred desktop wallpaper (captured at app launch, refreshed every 30s)
3. 1% opacity noise grain overlay across the entire app for filmic texture (use a tileable 256×256 noise PNG)

User can override the background in Settings → Appearance:
- Default: blurred desktop
- Built-in ambient (8 slow-moving gradient blobs)
- Custom uploaded image
- Solid color from theme

### 2.4 Elevation Levels (Refractive Layering)

Depth is communicated through layered glass, not heavy drop-shadows.

| Level | Usage | Treatment |
|-------|-------|-----------|
| 0 | App base | `bg-app` solid `#0A0B10` |
| 1 | Canvas | `bg-canvas` solid `#0F1117` |
| 2 | Panels / cards | Glass recipe + `elevation.level-1` |
| 3 | Dropdowns, popovers | Glass elevated + `elevation.level-3` |
| 4 | Modals | Glass elevated + `elevation.level-4` + scrim |
| 5 | Tooltips | Solid dark + `elevation.level-1` |
| 6 | Toasts | Glass elevated + `elevation.level-2` |

**Rule:** never apply both heavy drop-shadow AND glass on the same element. Glass surfaces use the `inset 0 1px 0 stroke-light-leak` light-leak instead of outer shadows for definition.

---

## 3. Color System

### 3.1 Default Theme — Midnight

Already defined in the frontmatter. This is the canonical dark theme and must be implemented first. All other themes are token swaps applied to the same component library.

### 3.2 Theme Variants (11 total)

| # | Name | Accent | bg-app | Notes |
|---|------|--------|--------|-------|
| 1 | Midnight (default) | `#7C5CFF` | `#0A0B10` | Brand default |
| 2 | Pure Black | `#7C5CFF` | `#000000` | AMOLED — glass darker |
| 3 | Daylight | `#5B3FE4` | `#F4F5F8` | Light mode — text inverted, glass whitish |
| 4 | Synthwave | `#FF3CAC` → `#784BA0` gradient accent | `#1A0B2E` | Purple/pink neon |
| 5 | Cyberpunk | `#F0FF26` / `#00E5FF` dual accent | `#000000` | Yellow/cyan on black, glow trails |
| 6 | Forest | `#86E08A` | `#0B1A0F` | Earthy green, low saturation |
| 7 | Sunset | `#FF6B4A` → `#FFA94D` | `#1F0E08` | Warm orange/red |
| 8 | Ocean | `#3FD7E4` | `#06181D` | Teal calm |
| 9 | Mono | `#FFFFFF` | `#0A0A0A` | Pure grayscale brutalist |
| 10 | Glassmorphism Max | `#7C5CFF` | `#0A0B10` | Heavier blur (40px), animated gradient behind glass |
| 11 | Custom | User-defined | User-defined | Built via Theme Builder |

### 3.3 Theme File Format

Themes live as JSON files in `resources/themes/`. User-built themes live in `%APPDATA%\Veyra\themes\custom_*.json`. Theme swap is a **400ms cross-fade** between two overlaid rendered views.

Example theme file structure:
```json
{
  "name": "Midnight",
  "id": "midnight",
  "built_in": true,
  "tokens": {
    "bg-app": "#0A0B10",
    "accent-primary": "#7C5CFF",
    "...": "..."
  },
  "radius_scale": 1.0,
  "animation_intensity": 1.0
}
```

### 3.4 Color Usage Rules

- **Accent primary** appears on: active sidebar rail, active toggle states, selected slider thumbs, primary button fills, focus outlines, accent glow shadows, brand logo gradient, default visualizer color.
- **Accent secondary (green)** appears on: peak meters "safe" zones (0–60% fill), success states, "Active" status pills.
- **Danger red** appears on: clipping indicators (never accent violet for negative states), error banners, delete buttons.
- **Warning yellow** appears on: caution states (e.g., "Volume Gain exceeds 200%"), VU meter 60–85% zone.

---

## 4. Typography Rules

### 4.1 The Three-Font System

| Font | Use Case | Strict Rules |
|------|----------|--------------|
| **Nasalization** | Brand wordmark, screen titles, hero text, theme tile names | UPPERCASE ONLY · letter-spacing +0.06em · never below 16px · Regular weight only |
| **Inter** | Body, buttons, labels, nav, tooltips, all readable text | Weights 400/500/600 only · never 700+ |
| **JetBrains Mono** | All live numeric readouts (dB, Hz, ms, %, FPS) | Tabular figures enabled |

### 4.2 Where Nasalization Appears (Exhaustive List)

- "VEYRA SOUNDS" wordmark in top utility bar (16px)
- Splash screen logo wordmark (48px)
- Onboarding step 1 hero: "WELCOME TO VEYRA SOUNDS" (32px)
- Tagline "EVERY NOTE ELEVATED" on About / splash (20px)
- Main screen titles: "GAMER MODE", "SOUND LAB", "PRESETS", "DEVICES", "APPS", "SETTINGS" (28px on screen, never as sidebar label)
- Section card titles in Gamer Mode: "SOUND TRACKER", "SPATIAL AUDIO", "VOICE & MICROPHONE", "NIGHT MODE" (20px)
- Theme tile names in theme picker (12px is the absolute minimum — bump to 16px if too small)
- Visualizer mode labels in fullscreen mode

### 4.3 Where Nasalization MUST NOT Appear

- Sidebar nav labels (use Inter — they're small and frequently scanned)
- Body copy
- Button labels
- Settings descriptions
- Numeric values (use JetBrains Mono)
- Tooltips
- Any paragraph of 2+ lines

### 4.4 JUCE Font Loading

```cpp
// In MainComponent constructor or fonts initializer
juce::Typeface::Ptr nasalization = juce::Typeface::createSystemTypefaceFor(
    BinaryData::Nasalization_otf, BinaryData::Nasalization_otfSize);
juce::Typeface::Ptr inter = juce::Typeface::createSystemTypefaceFor(
    BinaryData::Inter_Variable_ttf, BinaryData::Inter_Variable_ttfSize);
juce::Typeface::Ptr jetbrainsMono = juce::Typeface::createSystemTypefaceFor(
    BinaryData::JetBrainsMono_Regular_ttf, BinaryData::JetBrainsMono_Regular_ttfSize);
```

Embed fonts via JUCE's `BinaryData` (Projucer or the `juce_add_binary_data` CMake function). Do NOT rely on system-installed fonts.

**Font licensing note:** Verify Nasalization's license permits redistribution in GPLv3 software. If restrictive, fall back to **Orbitron** (SIL OFL, free for any use) — it has similar geometric futurist character. Update all typography references if swapped.

---

## 5. Layout System

### 5.1 The 4px Base Grid

Every spacing value is a multiple of 4. Period. No 5px, no 7px, no 13px gaps anywhere.

### 5.2 Application Window Structure

```
┌────────────────────────────────────────────────────────────┐
│ Top Utility Bar (56px tall, full width)                    │
├──────┬─────────────────────────────────────────────────────┤
│      │                                                     │
│ Side │            Main Canvas                              │
│ bar  │            (24px padding all sides)                 │
│ 220  │                                                     │
│ px   │                                                     │
│      │                                                     │
└──────┴─────────────────────────────────────────────────────┘
```

- **Default window:** 1180 × 760
- **Min window:** 960 × 620
- **Topbar height:** 56px (fixed)
- **Sidebar expanded:** 220px wide; collapsed: 64px (icon-only)
- **Main canvas inner padding:** 24px all sides
- **Section gaps (vertical inside main canvas):** 16–24px

### 5.3 Sidebar Composition

Top to bottom:
1. 24px top padding
2. Nav items (Home, Presets, Apps, Devices, Sound Lab, Gamer Mode) — 40px tall each, 4px vertical gap
3. Flexible spacer
4. 1px divider stroke `stroke-default`, 16px margin top and bottom
5. Settings nav item (40px tall)
6. Mini-mode button (32px, compact)
7. Version label `v1.0.0` — JetBrains Mono 11px `text-tertiary`, centered, 8px bottom padding

### 5.4 Top Utility Bar Composition

Left to right:
- 24px left padding
- Logo icon (28×28) + wordmark "VEYRA SOUNDS" (Nasalization 16px) — 8px gap between
- 16px gap, then master toggle pill
- 16px gap, then master volume slider (220px wide) + "Master" label above + dB value to right
- Flex spacer (center pushes outward)
- Current preset chip (clickable)
- 16px gap, A/B compare button
- 8px gap, search icon button
- 8px gap, notifications bell
- 8px gap, settings gear
- 16px gap, window controls (min/max/close, 14×14 each, 8px gaps)
- 16px right padding

---

## 6. Shape Language

**Precision Softness** — refined corners that suggest physical hardware, not iOS app blobs.

| Element Type | Radius | Token |
|--------------|--------|-------|
| Cards, panels | 16px | `lg` |
| Buttons, inputs, dropdowns | 10px | `md` |
| Slider tracks | 6px | `xs` |
| Modals | 20px | `xl` |
| Tooltips | 8px | `sm` |
| Toggles, pills, chips, segmented controls | 999px | `pill` |
| Fullscreen visualizer | 0 | `none` |

**Never deviate.** A button with 12px radius is wrong. Use 10px.

---

## 7. Component Specifications

For each component below, all 5 states must be implemented: default, hover, active/pressed, focus (keyboard), disabled.

### 7.1 Button — Primary

| Property | Value |
|----------|-------|
| Background | `accent-primary` (`#7C5CFF`) |
| Text color | `text-on-accent` (`#FFFFFF`) |
| Height | 36px |
| Horizontal padding | 16px |
| Border-radius | `md` (10px) |
| Font | `button` (Inter 13px / 500) |
| Border | none |
| Shadow | `glow-accent` at 30% opacity |

**States:**
- **Hover:** bg `accent-primary-hover` (`#8E72FF`), shadow at 50%
- **Active (pressed):** bg `accent-primary-active` (`#6A4AE8`), transform `scale(0.97)` over 80ms
- **Focus (keyboard):** 2px outline `stroke-focus` with 4px offset
- **Disabled:** bg `rgba(124, 92, 255, 0.30)`, text `text-disabled`, cursor not-allowed, no shadow

### 7.2 Button — Ghost

| Property | Value |
|----------|-------|
| Background | transparent |
| Text color | `text-primary` |
| Height | 36px |
| Horizontal padding | 16px |
| Border-radius | `md` (10px) |
| Border | 1px solid `stroke-default` |

**Hover:** bg `bg-glass-hover`, border `stroke-hover`
**Active:** bg `bg-glass-active`

### 7.3 Button — Danger

| Property | Value |
|----------|-------|
| Background | transparent |
| Text color | `danger` |
| Border | 1px solid `rgba(248, 113, 113, 0.40)` |
| Other | matches Ghost button |

**Special:** Delete buttons require hold-to-confirm. Render a progress bar inside the button that fills over 800ms while held. Release before completion cancels.

### 7.4 Button — Icon-Only

| Property | Value |
|----------|-------|
| Size | 32×32 |
| Icon size | 18×18 (Phosphor regular) |
| Icon color | `text-secondary` default, `text-primary` hover, `accent-primary` active |
| Border-radius | `sm` (8px) |

### 7.5 Toggle Switch (Pill)

| Property | Value |
|----------|-------|
| Track size | 36 × 20px |
| Track radius | `pill` (999px) |
| Thumb | 16×16 circle, white, 2px inset from track |
| OFF track | `rgba(60, 62, 80, 0.6)` |
| ON track | `accent-primary` + subtle glow |
| Transition | 180ms `easing-standard` for thumb position + track color |

Label position: right of toggle, 8px gap, Inter 13px.

### 7.6 Slider — Horizontal

| Property | Value |
|----------|-------|
| Track | 4px tall, radius `xs`, bg `rgba(60, 62, 80, 0.6)` |
| Track-fill | left of thumb, bg `accent-primary` |
| Thumb | 18×18 circle, white center, 2px `accent-primary` border |
| Thumb shadow | `0 2px 8px rgba(0,0,0,0.4)` |
| On drag | thumb scales to 22×22, accent glow halo |
| Value tooltip | floats 8px above thumb during drag, glass bg, JetBrains Mono 12px |

### 7.7 Slider — Vertical (EQ Bands)

| Property | Value |
|----------|-------|
| Track | 6px wide, 200px tall, radius `xs` |
| Track-fill | between 0dB midpoint and thumb (rises both directions) |
| Thumb | 24×24 circle, white center, 2px accent border |
| Midpoint detent | subtle horizontal tick at center, snap-to-zero within ±0.3 dB |
| Floating value | above thumb, JetBrains Mono 12px, glass bg, e.g. "+3.5 dB" |
| Frequency label | below track, JetBrains Mono 11px `text-tertiary` |

### 7.8 Knob (Effect Knob)

| Property | Value |
|----------|-------|
| Card container | 64×64 micro-card, glass, 12px padding, radius `md` |
| Knob visual | 56×56 circle |
| Outer arc | 8px stroke, sweep from 225° to 495° (270° total) |
| Arc background | `rgba(60, 62, 80, 0.4)` |
| Arc fill | `accent-primary` with subtle glow, painted up to current value |
| Inner circle | 38×38 glass with 1px stroke (simulates physical knob) |
| Indicator | 14×2px white line from center to top-of-inner, rotates with value |
| LED dot | 4×4 circle at top of card (12 o'clock), dim when value=0, accent glow when >0 |
| Label above | Inter 12px / 500 centered |
| Value below | JetBrains Mono 12px |

### 7.9 Dropdown

**Closed:** glass bg, 1px stroke, radius `md`, height 36px, padding 12px horizontal. Selected text left, chevron-down right.
**Open:** chevron rotates 180°, popover panel appears 4px below with glass elevated bg, max-height 280px, scrollable. Items 32px tall, 12px padding. Hover state `bg-glass-hover`. Selected item shows checkmark right + `accent-primary` text.

### 7.10 Glass Card

| Property | Value |
|----------|-------|
| Recipe | Section 2.1 mandatory |
| Inner padding | 24px |
| Border-radius | `lg` (16px) |
| Header row | title left, action area right, 16px bottom margin |
| Body | main content |
| Footer (optional) | 1px top border `stroke-default`, 16px top padding |

### 7.11 Pill / Chip / Badge

| Property | Value |
|----------|-------|
| Height | 24px |
| Horizontal padding | 10px |
| Radius | `pill` |
| Font | Inter 12px / 500 |
| Default bg | `bg-glass-hover` |
| Default text | `text-secondary` |
| Active variant | bg `accent-primary`, text white |

Status variants: success/warning/danger/info use the matching `*-bg` and `*` colors.

### 7.12 Segmented Control

Pill-shaped container, glass bg, 1px stroke, 4px padding. Items 32px tall, 16px horizontal padding, Inter 13px / 500. Inactive items transparent. Active item: `bg-glass-elevated`, `text-primary`, 1px `stroke-active`. Active background slides between positions over 180ms.

### 7.13 Modal

- **Scrim:** full-window `bg-modal-scrim` with 20px backdrop-blur of underlying app
- **Container:** centered, glass elevated (`bg-glass-elevated`), border-radius `xl` (20px), 1px `stroke-hover`
- **Widths:** small 480 / medium 640 / large 800
- **Header:** 24px padding, title (Nasalization or Inter heading depending), close icon top-right
- **Body:** 24px padding, scrollable if content exceeds 70vh
- **Footer:** 16px vertical, 24px horizontal, 1px top border, right-aligned button group
- **Entrance:** scrim fades 200ms, container fades + slides up from 16px below over 280ms

### 7.14 Tooltip

Background `rgba(0, 0, 0, 0.85)` with 8px backdrop blur, 1px `stroke-default`, padding 4–6px, radius `sm`. Inter 12px / 400. Triangle arrow 4px pointing at trigger. 6px gap from trigger. Appears after 500ms hover; disappears on mouse leave or after 5s.

### 7.15 Toast Notification

Top-center, 24px from top of canvas. Glass elevated. 12×10px padding. Radius `md`. Border-left 3px solid semantic color. Icon (18px semantic) + message (Inter 13px) + close X. Slides in from top 280ms, auto-dismisses after 4s, fades out 200ms.

### 7.16 Input Field

Container 36px tall, 12px horizontal padding, glass `bg-input` (darker than canvas — `#07080C`), 1px `stroke-default`, radius `md`. Label above in Inter 12px / 500 `text-secondary`, 4px margin. Input text Inter 14px `text-primary`. Placeholder Inter 14px `text-tertiary`. Focus: stroke `accent-primary` with subtle outer glow `glow-accent`, no separate outline. Error: stroke `danger`, error message below in Inter 12px `danger`.

### 7.17 VU Meter

- **Vertical:** 8×64 bars, 2 of them side-by-side (L/R) with 4px gap
- **Horizontal:** 80×4 bar
- **Background:** `rgba(60, 62, 80, 0.4)` radius 4px
- **Fill gradient:**
  - 0–60%: `accent-secondary` (status green `#A8C64E`)
  - 60–85%: `warning` (`#FBBF24`)
  - 85–100%: `danger` (`#F87171`)
- **Peak hold:** 2px line at peak position, decays linearly over 1.5s
- **Clipping:** peak segment pulses red, "CLIP" text appears in JetBrains Mono 10px above

### 7.18 Spectrum Bars (Live)

N bars, log-frequency mapped. 4px wide, 2px gaps, radius 2px at top. Color `accent-primary` with vertical brightness gradient. Soft accent glow under tallest bars. **Smoothing:** Monstercat-style gravity decay — `current = max(input, current * 0.95)` per frame.

### 7.19 Sidebar Nav Item

| State | Treatment |
|-------|-----------|
| Default | transparent bg, `text-secondary` text + icon |
| Hover | `bg-glass-hover` bg, `text-primary` |
| Active | `bg-glass-active` bg, `text-primary` text, `accent-primary` icon (Phosphor Fill variant) + **3px accent rail on left edge** |
| Focus | 2px outline `stroke-focus` |

Container: 220 × 40px expanded, 12px horizontal padding. Icon 18×18, label `nav-label` (Inter 14 / 500), 12px gap between.

### 7.20 Preset Card

260 × 180. Glass card. Top row: category badge left, favorite star right. Middle: preset name (Inter 18 / 500), truncate ellipsis. Below name: 240×40 EQ sparkline. Bottom: creator chip ("Veyra" / "You") left, "Active" indicator right.

**Hover:** card lifts (`translateY(-2px)`), accent glow, action buttons appear: Preview (▶), Favorite (★), More (⋯).
**Active preset:** 2px accent border, checkmark badge top-right.
**Empty/new:** dashed border `rgba(255,255,255,0.20)`, centered "+" icon and subtitle.

---

## 8. Iconography

**Library:** Phosphor Icons (https://phosphoricons.com). Use the **Regular** variant by default, **Fill** variant for active/selected states.

**Sizes:**
- Sidebar nav: 18×18
- Inline (next to text): 16×16
- Primary action: 20×20
- Feature card headers: 24×24
- Onboarding step icons: 32×32

**Colors:**
- Default: `text-secondary`
- Hover: `text-primary`
- Active: `accent-primary`
- Disabled: `text-disabled`

Embed Phosphor SVGs in `resources/icons/` and load via JUCE's `Drawable::createFromSVG()`.

---

## 9. Animation System

### 9.1 Timing Tokens

| Token | Duration | Use |
|-------|----------|-----|
| `duration-instant` | 80ms | Button press scale |
| `duration-fast` | 180ms | Default transitions (hover, toggle, slider) |
| `duration-medium` | 280ms | Modal mounts, panel slides |
| `duration-slow` | 400ms | Theme cross-fades |

### 9.2 Standard Easing

`cubic-bezier(0.32, 0.72, 0, 1)` — used for almost everything. It's a snappy ease-out curve that feels responsive.

### 9.3 Animation Inventory

- **Sidebar item hover:** bg fades 180ms
- **Button hover:** bg lightens 180ms (no scale)
- **Button press:** scale 0.97 over 80ms, return on release
- **Toggle switch:** thumb slides + track color cross-fades 180ms
- **Slider drag:** real-time, zero delay
- **Dropdown open:** chevron rotates 180° + popover fades + slides down 8px over 220ms
- **Modal mount:** scrim fades 200ms, content fades + slides up from 16px over 280ms
- **Toast in:** slides down from top 280ms
- **Toast out:** fades over 200ms
- **Theme switch:** 400ms cross-fade between two overlaid theme renderings
- **Visualizer:** matches display refresh rate (60/120/144 FPS) via JUCE OpenGL renderer
- **Spectrum bars:** Monstercat decay (see Section 7.18)
- **VU peak hold:** instant rise, 1.5s linear decay
- **Clipping flash:** danger pulse at 2Hz, 50%↔100% opacity while clipping

### 9.4 What NOT to Animate

- Page transitions between screens (instant — desktop apps don't slide between pages)
- Bouncy springs on UI elements
- Parallax on scroll
- Entrance animations on list items

### 9.5 JUCE Implementation

Use `juce::Component::AnimationManager` for property-based animations. For OpenGL-accelerated visualizers, use `juce::OpenGLContext::setContinuousRepainting(true)` and disable internal frame limiting to match display refresh.

---

## 10. Accessibility

### 10.1 Color Contrast (WCAG AA Minimum)

- Body text: 4.5:1 against background
- Large text (18px+): 3:1
- Semantic indicators: distinguishable by icon + label, not color alone
- All 11 themes must pass — flag any failures during theme implementation

### 10.2 Keyboard Navigation

- Every interactive element focusable via Tab
- 2px focus outline `stroke-focus` with 4px offset, always visible
- Logical tab order on every screen
- Modals trap focus
- Escape closes modals/popovers

### 10.3 Screen Reader Support

- Every icon-only button has `aria-label` equivalent (JUCE: `setAccessible(true)` + `setTitle()` + `setDescription()`)
- Sliders announce label + current value + range
- Visualizer marked decorative (excluded from accessibility tree)

### 10.4 Motion Preferences

Setting → Appearance → "Reduce motion" toggle. When enabled:
- Disable visualizer continuous animation (show static curve instead)
- Reduce all transition durations to 0ms except essential state changes
- Disable parallax/glow pulses

### 10.5 High-Contrast Mode

Separate from the 11 styled themes. Settings → Appearance → "Accessibility → High contrast" toggle overrides active theme: pure black bg, pure white text, bright cyan accent, no glassmorphism, hard 2px borders on every interactive element.

---

## 11. Localization

### 11.1 Supported Languages (v1)

- English (en) — default
- 简体中文 (zh-CN)
- Español (es)
- العربية (ar) — **RTL**
- हिन्दी (hi)
- தமிழ் (ta)

### 11.2 RTL Mirroring (Arabic)

When language is Arabic:
- Sidebar moves to right side
- Window controls (min/max/close) move to top-left
- Text aligns right
- Sliders flip direction (low value right, high left)
- Knob arc reverses sweep direction
- Direction-implying icons flip (chevrons, arrows)

### 11.3 JUCE Implementation

Use `TRANS()` macro for all user-facing strings. Translation files in `resources/lang/<locale>.lang`. For RTL, set `juce::TextEditor::setJustification(juce::Justification::right)` and flip layout with `juce::ComponentBuilder` or custom layout managers.

---

## 12. Phosphor Icon Inventory (Phase 4 Subset)

Icons needed for Phase 4 screens (Home + Mini-mode + Settings → Appearance + Tray menu):

| Icon (Phosphor name) | Use |
|---------------------|-----|
| `waveform` | Home nav item |
| `bookmark` | Presets nav item |
| `squares-four` | Apps nav item |
| `speaker-high` | Devices nav item |
| `flask` | Sound Lab nav item |
| `game-controller` | Gamer Mode nav item |
| `gear` | Settings nav item |
| `compact-disc` | Mini-mode button |
| `power` | Master ON/OFF |
| `magnifying-glass` | Search |
| `bell` | Notifications |
| `corners-out` | Fullscreen visualizer |
| `caret-down` | Dropdown chevron |
| `caret-up`, `caret-left`, `caret-right` | Various |
| `x` | Close modals/toasts |
| `check` | Selected state |
| `dots-three-vertical` | Context menu |
| `star`, `star-fill` | Favorite |
| `play` | Preview |
| `download-simple` | Export |
| `upload-simple` | Import |
| `palette` | Theme builder |
| `info` | Info notices |
| `warning` | Warnings |
| `shield-check` | Anti-cheat detected |
| `microphone` | Mic features |
| `headphones`, `desktop`, `monitor`, `bluetooth` | Device types |

Bundle the full Phosphor SVG set in `resources/icons/phosphor/` — only ~2 MB total.

---

## 13. Phase 4 Implementation Scope

**This document covers all screens.** Phase 4 builds only the foundation + first screens.

### 13.1 Phase 4 Targets

1. **Theme system** — load `design-tokens.json` at runtime, support 11 themes (implement Midnight first, scaffold the rest)
2. **Glass rendering pipeline** — Section 2.2 recipe
3. **Component library** — Sections 7.1 through 7.19 (all components)
4. **Home screen** — full implementation
5. **Mini-mode bar** — full implementation
6. **Settings → Appearance** sub-page only (other Settings sub-pages come in Phase 11)
7. **System tray icon + custom glass popup menu**

### 13.2 Out of Scope for Phase 4

- Presets, Apps, Devices, Sound Lab, Gamer Mode screens (later phases)
- Onboarding flow (Phase 11)
- Theme Builder modal (Phase 11)
- Hotkey configuration UI (Phase 11)
- Other Settings sub-pages: General, Audio Engine, Hotkeys, Updates, Crash Reports, About (Phase 11)

### 13.3 Acceptance Criteria

Phase 4 is complete when:
- [ ] Theme system loads `design-tokens.json` and applies all values
- [ ] Switching between Midnight and 1 other theme (e.g., Synthwave) works with 400ms cross-fade
- [ ] Glass rendering produces visually correct blurred surfaces (verify against `01_home_default.png`)
- [ ] All Phase 4 components render with all 5 states (default/hover/active/focus/disabled)
- [ ] Home screen pixel-matches `01_home_default.png` ±2px tolerance
- [ ] Mini-mode renders correctly at 320×80
- [ ] Settings → Appearance shows theme grid and switches themes live
- [ ] Tray icon shows custom glass popup menu (not native Windows menu)
- [ ] All EQ controls on Home update DSP through APO with <50ms perceived latency

---

## 14. Reference Files Expected in `reference/design/`

```
reference/design/
├── design.md                          (this file)
├── design-tokens.json                 (machine-readable token export)
├── screenshots/
│   ├── 01_home_default.png            (Home screen — Midnight theme)
│   ├── 04_mini_mode.png               (Mini-mode 320×80)
│   ├── 05_settings_appearance.png     (Settings → Appearance)
│   ├── 07_tray_menu.png               (System tray popup)
│   ├── 13_components_library.png      (Component gallery)
│   └── 14_foundations_tokens.png      (Token reference)
└── icons/
    └── phosphor/                       (full Phosphor icon set as SVG)
```

---

## 15. Reference: Frontmatter as design-tokens.json

The YAML frontmatter at the top of this file should also exist as a separate `design-tokens.json` so it's machine-loadable by JUCE at runtime. Generate it by converting this file's frontmatter from YAML to JSON. Token names and values must match exactly.

---

— End of design system specification —
