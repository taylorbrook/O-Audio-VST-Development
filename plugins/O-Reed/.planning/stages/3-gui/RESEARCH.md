# Stage 3: GUI - Research

**Date:** 2026-04-06
**Plugin:** O-Reed
**Phase:** 4.1 (Layout, tabs, collapsible sections, knobs, XY pad, bore viz placeholder)

---

## 1. Current C++ State (No Changes Needed)

The PluginEditor is fully wired from Stage 1:

- **28 WebSliderRelays** (all float/int params)
- **6 WebComboBoxRelays** (instrumentPreset, boreProfile, vibratoSource, tuningSystem, polyMode, oversampling)
- **1 WebToggleButtonRelay** (dualBore)
- **35 WebParameterAttachments** (matching relays to APVTS)
- **Resource provider** serves from BinaryData: `index.html`, `index.js`, `check_native_interop.js`, `tuning-panel.js`, `tuning-panel.css`
- **Window size:** 900x600

All relay names exactly match APVTS parameter IDs. No C++ changes needed for Phase 4.1 — only HTML/CSS/JS work.

---

## 2. JUCE WebView Relay API (JavaScript Side)

### SliderRelay (28 params)

```javascript
import { getSliderState } from './juce/index.js';

const state = getSliderState('breathPressure');  // Must match relay name exactly

// Read
state.getNormalisedValue();  // 0.0–1.0
state.getScaledValue();      // In actual parameter range

// Write (with drag lifecycle)
state.sliderDragStarted();
state.setNormalisedValue(0.5);
state.sliderDragEnded();

// Listen for host/automation changes
state.valueChangedEvent.addListener(() => {
    const v = state.getNormalisedValue();
    updateKnob(v);
});

// Properties available
state.properties.start;   // range min
state.properties.end;     // range max
state.properties.skew;    // log-like skew factor
state.properties.label;   // unit label ("Hz", "dB", "cents")
state.properties.name;    // display name
```

### ComboBoxRelay (6 params)

```javascript
import { getComboBoxState } from './juce/index.js';

const state = getComboBoxState('instrumentPreset');

state.getChoiceIndex();                     // 0-based index
state.properties.choices;                   // String array of labels
state.setChoiceIndex(3);                    // Set by index
state.valueChangedEvent.addListener(() => { /* update UI */ });
```

### ToggleButtonRelay (1 param: dualBore)

```javascript
import { getToggleState } from './juce/index.js';

const state = getToggleState('dualBore');

state.getValue();                           // boolean
state.setValue(!state.getValue());           // toggle
state.valueChangedEvent.addListener(() => { /* update UI */ });
```

**Critical notes:**
- `valueChangedEvent` callbacks receive NO parameters — always call getter inside
- Always call `sliderDragStarted()`/`sliderDragEnded()` for proper DAW automation recording
- Use `pointerdown`/`pointermove`/`pointerup` (not mouse events)

---

## 3. Parameter Reference (All 35)

### Primary Controls (Instrument Panel)
| ID | Type | Range | Default | Unit | Control |
|----|------|-------|---------|------|---------|
| breathPressure | Float | 0–1 | 0.5 | — | Knob |
| embouchure | Float | 0–1 | 0.4 | — | Knob |
| reedHardness | Float | 0–1 | 0.5 | — | Knob |
| boreCharacter | Float | 0–1 | 0.0 | — | Knob |
| instrumentPreset | Choice | 21 items | 0 | — | XY Pad (macro) |

### Secondary Controls (Bore & Resonance + Reed)
| ID | Type | Range | Default | Unit | Control |
|----|------|-------|---------|------|---------|
| reedOpening | Float | 0–1 | 0.4 | — | Knob |
| bellSize | Float | 0–1 | 0.5 | — | Knob |
| airNoise | Float | 0–1 | 0.15 | — | Knob |
| doubleReed | Float | 0–1 | 0.0 | — | Knob |
| boreDiameter | Float | 0–1 | 0.5 | — | Knob |

### Advanced / Sound Design
| ID | Type | Range | Default | Unit | Control |
|----|------|-------|---------|------|---------|
| reedMass | Float | 0–1 | 0.3 | — | Knob |
| reedDamping | Float | 0–1 | 0.5 | — | Knob |
| mouthpieceVol | Float | 0–1 | 0.3 | — | Knob |
| toneHoleCutoff | Float | 200–8000 | 1500 | Hz | Knob (skew 0.3) |
| registerHole | Float | 0–1 | 0.0 | — | Knob |
| boreLength | Float | 0–1 | 0.5 | — | Knob |
| boreProfile | Choice | Simple, Multi-segment | 0 | — | Dropdown |

### Expressive Controls
| ID | Type | Range | Default | Unit | Control |
|----|------|-------|---------|------|---------|
| vibratoDepth | Float | 0–1 | 0.0 | — | Knob |
| vibratoRate | Float | 1–10 | 5.0 | Hz | Knob |
| vibratoSource | Choice | Lip, Breath, Throat | 0 | — | Dropdown |
| growlAmount | Float | 0–1 | 0.0 | — | Knob |
| flutterTongue | Float | 0–1 | 0.0 | — | Knob |
| subtone | Float | 0–1 | 0.0 | — | Knob |
| attackChiff | Float | 0–1 | 0.3 | — | Knob |

### Impossible Physics (Sound Design)
| ID | Type | Range | Default | Unit | Control |
|----|------|-------|---------|------|---------|
| infiniteSustain | Float | 0–1 | 0.0 | — | Knob |
| reverseBore | Float | 0–1 | 0.0 | — | Knob |
| dualBore | Bool | — | false | — | Toggle |
| dronePitch | Float | -2400–2400 | 0 | cents | Knob |
| feedbackPath | Float | 0–1 | 0.0 | — | Knob |

### Tuning (Tuning Panel)
| ID | Type | Range | Default | Unit | Control |
|----|------|-------|---------|------|---------|
| referencePitch | Float | 220–880 | 440 | Hz | Knob |
| tuningSystem | Choice | Scala/TUN, MTS-ESP, 12-TET | 2 | — | Dropdown |

### Voice Configuration
| ID | Type | Range | Default | Unit | Control |
|----|------|-------|---------|------|---------|
| polyMode | Choice | Monophonic, Polyphonic | 0 | — | Dropdown |
| maxVoices | Int | 1–16 | 8 | — | Knob |
| oversampling | Choice | 2x, 4x | 0 | — | Dropdown |

### Output
| ID | Type | Range | Default | Unit | Control |
|----|------|-------|---------|------|---------|
| outputGain | Float | -60–12 | 0 | dB | Knob |

---

## 4. Aesthetic: Ouaricon Naturalist (ouaricon-naturalist-001)

### Color Palette
```css
:root {
    --bg-paper: #F5E6D3;
    --bg-paper-mid: #EBD9C7;
    --bg-accent: #D4C4B0;
    --brown-border: #8B7355;
    --brown-frame: #5C4033;
    --brown-text: #3C2F2F;
    --green-light: #8BA870;
    --green-mid: #6B8E4E;
    --green-dark: #3C5C1A;
    --green-darkest: #2C3E10;
    --knob-ring: #C9A27B;
    --knob-segment-1: #F5DEB3;
    --knob-segment-2: #E8D5B7;
    --knob-core: #FFF8DC;
}
```

### Typography
- Font: `'Garamond', 'Times New Roman', serif`
- Plugin title: 16px, normal weight, uppercase, letter-spacing: 3px
- Section labels: 11–12px, uppercase, letter-spacing: 1px
- Parameter labels: 9–10px, uppercase, letter-spacing: 0.5px
- Value displays: 10px, regular case

### Seed Cross-Section Knob (CSS)
10-segment radial pattern via `conic-gradient`:
- Alternating cream tones (#F5DEB3, #E8D5B7) with 1deg brown dividers
- Outer ring (#C9A27B), inner core (#FFF8DC)
- Border: 2px solid #8B7355
- Size: 55px for compact layouts (35 params), 50px for dense sections
- Rotation indicator: triangle at top, rotates with value (0–270deg arc)

### Toggle Button
- Default: `rgba(139,168,112,0.3)` bg, `#3C5C1A` border
- Active: `rgba(107,142,35,0.6)` bg, `#2C3E10` border
- Fleuron decoration (❦) at low opacity

### Dropdown (Select)
- Background: #F5E6D3, border: 1px solid #8B7355
- Font: Garamond, 10px
- Border-radius: 3px

---

## 5. Layout Architecture

### Three-Tab Navigation (900x600)

```
+------------------------------------------------------------------+
|  O-REED  Physical Modeling Reed Wind    [Instrument][Tuning][FX]  |  36px header
+------------------------------------------------------------------+
|                                                                    |
|  (Active panel content — 564px height)                            |
|                                                                    |
+------------------------------------------------------------------+
```

- Header: fixed 36px, plugin name left, tab buttons right
- Active panel: remaining 564px
- Only one panel visible at a time (CSS `display:none` for inactive)
- Tab styling: Naturalist buttons with brown border, green active state

### Instrument Panel (Scrollable)

```
XY Pad (instrumentPreset macro)              ~180px
─────────────────────────────────────────────
▼ PRIMARY CONTROLS                           ~100px
  breathPressure  embouchure  reedHardness  outputGain
─────────────────────────────────────────────
▷ BORE & RESONANCE (collapsed)
  boreCharacter  boreDiameter  bellSize  boreLength  boreProfile(dropdown)
─────────────────────────────────────────────
▷ BORE VISUALIZATION (collapsed)
  <canvas/SVG bore profile>
─────────────────────────────────────────────
▷ REED (collapsed)
  reedOpening  reedMass  reedDamping  doubleReed  mouthpieceVol
─────────────────────────────────────────────
▷ EXPRESSION (collapsed)
  vibratoDepth  vibratoRate  vibratoSource(dropdown)
  growlAmount  flutterTongue  subtone  attackChiff  airNoise
─────────────────────────────────────────────
▷ SOUND DESIGN (collapsed)
  infiniteSustain  reverseBore  feedbackPath
  dualBore(toggle)  dronePitch
─────────────────────────────────────────────
▷ VOICE (collapsed)
  polyMode(dropdown)  maxVoices  oversampling(dropdown)
```

- Primary Controls expanded by default
- All others collapsed by default
- CSS `overflow-y: auto` for native scrolling
- Smooth CSS transitions for expand/collapse

### Tuning Panel
- Uses shared `tuning-panel.js` / `tuning-panel.css` module
- Binds to `referencePitch` and `tuningSystem` relays
- Spacious layout with the tuning visualization components

### Effects Panel
- Placeholder: "Coming soon" centered text
- Reserved for future effect chain additions

---

## 6. XY Pad Implementation

### Approach
The XY pad controls `boreCharacter` (X axis) and `doubleReed` (Y axis) simultaneously, providing a 2D instrument morphing space.

### Binding
- X axis: `getSliderState('boreCharacter')` — 0 (cylindrical) to 1 (conical)
- Y axis: `getSliderState('doubleReed')` — 0 (single reed) to 1 (double reed)
- `instrumentPreset` ComboBox is separate — preset markers are informational overlays

### Preset Markers (Informational, Not Interactive)
Hardcoded positions mapping instrument presets to approximate (boreCharacter, doubleReed) coordinates:

| Preset | X (bore) | Y (reed) | Label |
|--------|----------|----------|-------|
| Bb Clarinet | 0.0 | 0.0 | Clar |
| Bass Clarinet | 0.05 | 0.0 | B.Clar |
| Alto Sax | 0.65 | 0.0 | A.Sax |
| Tenor Sax | 0.7 | 0.0 | T.Sax |
| Soprano Sax | 0.6 | 0.0 | S.Sax |
| Bari Sax | 0.75 | 0.0 | B.Sax |
| Oboe | 0.5 | 0.5 | Oboe |
| English Horn | 0.55 | 0.45 | E.Hrn |
| Bassoon | 0.45 | 0.55 | Bsn |
| Duduk | 0.2 | 0.65 | Ddk |
| Shehnai | 0.7 | 0.7 | Shn |
| Suona | 0.75 | 0.8 | Suona |
| Hichiriki | 0.15 | 0.75 | Hch |
| Zurna | 0.8 | 0.85 | Zrn |
| Piri | 0.3 | 0.6 | Piri |

### Canvas Implementation
- DPI-aware: `canvas.width = w * dpr; ctx.setTransform(dpr, 0, 0, dpr, 0, 0)`
- Grid lines at 25% intervals, brown (#8B7355) at 10% opacity
- Preset labels in Garamond 8px, brown text
- Current position: green (#6B8E4E) filled circle, 8px radius
- Crosshair lines through current position at 20% opacity
- Axis labels: "Cylindrical → Conical" (X), "Single Reed → Double Reed" (Y)
- Aged paper background (#EDE0CF) with inset shadow

---

## 7. Bore Visualization (Phase 4.1 = Placeholder)

Phase 4.1 creates the collapsible section and canvas element. Phase 4.3 implements the real-time bore profile rendering.

### Placeholder for 4.1
- Canvas element inside collapsible section (~300x120px)
- Static text: "Bore profile visualization" centered
- Background: #EDE0CF with 1px brown border

### Full Implementation (Phase 4.3)
- SVG or Canvas cross-section showing bore taper profile
- Left = mouthpiece, Right = bell
- Upper/lower curves = bore radius along length
- Responds to: boreCharacter, boreDiameter, bellSize, boreLength, reverseBore, boreProfile
- Dual bore overlay when dualBore is active
- Colors: brown (#8B7355) outline, green (#6B8E4E) fill
- Update on parameter change only (not every frame)

---

## 8. Collapsible Section Pattern

```html
<div class="section" data-expanded="true">
  <div class="section-header">
    <span class="section-chevron">▼</span>
    <span class="section-title">PRIMARY CONTROLS</span>
  </div>
  <div class="section-content">
    <!-- knobs here -->
  </div>
</div>
```

```css
.section-content {
    max-height: 0;
    overflow: hidden;
    transition: max-height 0.3s ease;
}
.section[data-expanded="true"] .section-content {
    max-height: 500px;  /* Generous max for any section */
}
.section-chevron {
    display: inline-block;
    transition: transform 0.3s ease;
}
.section[data-expanded="false"] .section-chevron {
    transform: rotate(-90deg);
}
```

```javascript
document.querySelectorAll('.section-header').forEach(h => {
    h.addEventListener('click', () => {
        const section = h.closest('.section');
        const expanded = section.dataset.expanded === 'true';
        section.dataset.expanded = expanded ? 'false' : 'true';
    });
});
```

---

## 9. Resource Provider Updates Needed

Phase 4.1 will use a single `index.html` with inlined CSS/JS (matching O-Formant pattern, keeping it in one file served from BinaryData). If the JS grows beyond ~500 lines, split into `main.js` and add a resource route.

Current resource routes in PluginEditor.cpp already serve:
- `/` and `/index.html` → BinaryData::index_html
- `/js/juce/index.js` → BinaryData::index_js
- `/js/juce/check_native_interop.js` → BinaryData::check_native_interop_js
- `/js/tuning-panel.js` → BinaryData::tuningpanel_js
- `/css/tuning-panel.css` → BinaryData::tuningpanel_css

May need to add routes if splitting JS:
- `/js/main.js` → BinaryData for main app logic
- `/css/style.css` → BinaryData for styles
- Image files for botanical illustration

CMakeLists.txt `juce_add_binary_data` must include any new files.

---

## 10. Botanical Illustration

### Theme
Reed/cane botanical illustration — direct thematic connection (reed instruments use Arundo donax cane).

### Placement
- Right side of instrument panel
- ~70% of panel height
- Opacity: 0.3
- `pointer-events: none` (non-interactive overlay)
- `position: absolute`, right-aligned, vertically centered

### Sourcing
Image TBD at implementation time. Check existing library in `plugins/*/Source/ui/public/img/` or `Resources/img/` for available botanical PNGs. If none suitable, implement without image and add in later phase.

---

## 11. Tuning Panel Integration

The scala-tuning-engine module provides complete tuning UI:
- `tuning-panel.js` (882 lines) — intervals table, 5 viz modes, scale generator, file I/O
- `tuning-panel.css` (613 lines) — full theming with light/dark mode

### Integration Pattern
```html
<link rel="stylesheet" href="/css/tuning-panel.css">
<script type="module" src="/js/tuning-panel.js"></script>

<!-- In tuning panel tab -->
<div id="tuning-panel" class="tuning-panel light compact"></div>
```

The tuning panel JS auto-initializes when loaded and binds to `referencePitch` and `tuningSystem` relays internally. Need to verify it uses the same JUCE bridge import path.

---

## 12. Pitfalls and Known Issues

1. **Canvas replaced element sizing** — Use explicit `width`/`height` on canvas, not `left+right` positioning (see MEMORY.md: Canvas Replaced Element Gotcha)
2. **DPI scaling** — Always use `window.devicePixelRatio` for canvas backing store
3. **Scroll containment** — Panel scroll must not propagate to WebView parent; use `overscroll-behavior: contain`
4. **Section max-height** — CSS `max-height: auto` doesn't animate; use a large fixed value or JS-measured heights
5. **XY pad pointer capture** — Must use `setPointerCapture()` for drag continuity outside canvas bounds
6. **Resource paths** — Resource provider receives bare paths (`/`, `/index.html`), not full URLs
7. **WebView scrollbar styling** — Use `-webkit-scrollbar` for consistent styling on macOS/Windows
8. **35 relay initialization** — All 35 `getSliderState`/`getComboBoxState`/`getToggleState` calls should happen on `DOMContentLoaded` to avoid race with JUCE bridge
9. **Value formatting** — Skewed params (toneHoleCutoff with skew 0.3, outputGain -60 to 12 dB) need `getScaledValue()` for display, `getNormalisedValue()` for knob position

---

## 13. Phase 4.1 Scope Summary

### Must Build
- [x] Tab navigation (Instrument / Tuning / FX)
- [x] Instrument panel with scrollable collapsible sections (7 sections)
- [x] Seed cross-section knobs for all 28 slider params
- [x] Dropdowns for all 6 choice params
- [x] Toggle button for dualBore
- [x] XY pad with preset markers (boreCharacter x doubleReed)
- [x] Bore visualization placeholder (canvas in collapsible section)
- [x] Ouaricon Naturalist aesthetic (colors, typography, knob style)
- [x] Tuning panel integration (tuning-panel.js/css)
- [x] Effects placeholder tab

### Deferred to Phase 4.2
- Full two-way parameter binding (all 35 params active in JS)
- Host automation sync verification
- XY pad ↔ parameter interaction (updates boreCharacter + doubleReed)

### Deferred to Phase 4.3
- Real-time bore visualization rendering
- Preset browser
- Scala file browser
- Botanical illustration
- Advanced polish

---

## References

- Aesthetic spec: `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`
- O-Formant UI (complex reference): `plugins/O-Formant/Source/ui/public/index.html`
- O-Texture UI (XY pad): `plugins/O-Texture/Source/ui/public/js/main.js`
- JUCE bridge: `plugins/O-Reed/Resources/ui/js/juce/index.js`
- Tuning module: `modules/tuning/scala-tuning-engine/`
- CMakeLists: `plugins/O-Reed/CMakeLists.txt`
- PluginEditor: `plugins/O-Reed/Source/PluginEditor.cpp`
