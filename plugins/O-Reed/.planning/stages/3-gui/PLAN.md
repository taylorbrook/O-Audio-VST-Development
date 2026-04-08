# Stage 3: GUI - Phase 4.1 Execution Plan

**Date:** 2026-04-06
**Plugin:** O-Reed
**Phase:** 4.1 — Layout, Tabs, Collapsible Sections, Knobs, XY Pad, Bore Viz Placeholder

---

## Goal

Build the complete WebView UI for O-Reed in a single `index.html` with inlined CSS/JS: three-tab navigation (Instrument / Tuning / FX), Ouaricon Naturalist aesthetic, seed cross-section knobs for all 28 slider params, 6 dropdowns, 1 toggle, XY pad for instrument morph, bore visualization placeholder, and full two-way parameter binding for all 35 parameters via JUCE WebView relay API.

**Note:** CONTEXT.md originally split binding into Phase 4.2, but since all relays are already wired in C++, we bind everything in Phase 4.1 for a fully functional UI from the start.

---

## Tasks

### 1. [ ] Replace index.html with full Naturalist UI

**Files:** `plugins/O-Reed/Resources/ui/index.html`
**Depends on:** None

Build the complete single-file UI (~600-800 lines) with inlined `<style>` and `<script type="module">`:

**HTML Structure:**
```
<div class="container">
  <header> — O-REED title + tab buttons (Instrument / Tuning / FX) </header>
  <div id="panel-instrument"> — scrollable, collapsible sections </div>
  <div id="panel-tuning"> — tuning panel mount point </div>
  <div id="panel-fx"> — "Coming soon" placeholder </div>
</div>
```

**CSS (inlined `<style>`):**
- Ouaricon Naturalist palette: `--bg-paper: #F5E6D3`, brown borders, green accents
- Garamond typography: title 16px uppercase, section labels 11px, param labels 9px
- Container: 900x600, `overflow: hidden`, `user-select: none`
- Tab navigation: brown inactive, green active state, right-aligned in header
- Scrollable panel: `overflow-y: auto`, `overscroll-behavior: contain`, custom scrollbar
- Collapsible sections: `max-height` transition, chevron rotation
- Seed cross-section knob: 50px, `conic-gradient` 10-segment pattern, `#C9A27B` ring, `#8B7355` border
- Knob indicator: 3px bar from center, rotates 0–270deg arc
- Dropdown: `#F5E6D3` bg, `#8B7355` border, Garamond 10px
- Toggle: 42x22px pill, `#D4BFA0` off → `#8BA870` on
- XY pad canvas wrapper: inset shadow, `#EDE0CF` bg, crosshair cursor
- Bore viz placeholder: `#EDE0CF` bg, 1px brown border, centered text

**Sections in Instrument Panel (7 collapsible):**
1. XY Pad (always visible, not collapsible) — ~180px
2. PRIMARY CONTROLS (expanded) — breathPressure, embouchure, reedHardness, outputGain
3. BORE & RESONANCE (collapsed) — boreCharacter, boreDiameter, bellSize, boreLength, boreProfile
4. BORE VISUALIZATION (collapsed) — placeholder canvas ~300x120
5. REED (collapsed) — reedOpening, reedMass, reedDamping, doubleReed, mouthpieceVol
6. EXPRESSION (collapsed) — vibratoDepth, vibratoRate, vibratoSource, growlAmount, flutterTongue, subtone, attackChiff, airNoise
7. SOUND DESIGN (collapsed) — infiniteSustain, reverseBore, feedbackPath, dualBore, dronePitch
8. VOICE (collapsed) — polyMode, maxVoices, oversampling

**JavaScript (inlined `<script type="module">`):**

a) **Knob engine** — Pointer-based drag (pointerdown/pointermove/pointerup with `setPointerCapture`), vertical drag maps to 0–270deg rotation, calls `sliderDragStarted()`/`setNormalisedValue()`/`sliderDragEnded()`. Each knob element has `data-param` attribute matching relay name.

b) **Value display** — `getScaledValue()` formatted appropriately:
   - Most params: 2 decimal places, no unit
   - toneHoleCutoff: `Math.round()` + "Hz"
   - outputGain: 1 decimal + "dB"
   - vibratoRate: 1 decimal + "Hz"
   - dronePitch: `Math.round()` + "ct"
   - maxVoices: integer

c) **Dropdown binding** — `getComboBoxState()`, populate `<select>` from `state.properties.choices`, bind change event → `setChoiceIndex()`, listen for `valueChangedEvent`.

d) **Toggle binding** — `getToggleState('dualBore')`, click → `setValue(!getValue())`, listen for `valueChangedEvent`.

e) **Tab switching** — Click tab button → show/hide panels via `display:none`/`display:block`.

f) **Collapsible sections** — Click header → toggle `data-expanded`, CSS handles `max-height` animation.

g) **Host automation sync** — All 35 params register `valueChangedEvent` listeners on `DOMContentLoaded` to update knob rotation, dropdown selection, toggle state from host.

h) **XY pad** — Canvas element, DPI-aware (`devicePixelRatio`), explicit `width`/`height` sizing (not left+right — canvas replaced element gotcha). Draws:
   - Grid lines at 25% intervals, brown at 10% opacity
   - Preset markers (15 instruments) with Garamond 8px labels
   - Current position: green circle + crosshair
   - Axis labels: "Cylindrical → Conical" (X), "Single → Double Reed" (Y)
   - Binds to `boreCharacter` (X) and `doubleReed` (Y) sliders
   - Pointer drag updates both params simultaneously with drag lifecycle

i) **Bore visualization placeholder** — Static canvas with "Bore profile visualization" text, brown border, paper background.

### 2. [ ] Integrate tuning panel module

**Files:** `plugins/O-Reed/Resources/ui/index.html` (Tuning tab content)
**Depends on:** Task 1

In the Tuning tab panel:
```html
<link rel="stylesheet" href="/css/tuning-panel.css">
<div id="tuning-panel" class="tuning-panel light compact"></div>
<script type="module" src="/js/tuning-panel.js"></script>
```

The tuning-panel.js auto-initializes and binds to `referencePitch` and `tuningSystem` relays. Resource routes already exist in PluginEditor.cpp.

### 3. [ ] Build and verify

**Files:** None new — build existing
**Depends on:** Tasks 1, 2

```bash
cd build && ninja O-Reed_VST3 O-Reed_AU
```

Verify:
- WebView opens at 900x600
- Tab navigation works (3 tabs)
- Primary Controls section expanded with 4 knobs
- Other sections collapse/expand
- XY pad renders with preset markers
- Bore viz placeholder visible in section
- Tuning tab shows tuning panel
- FX tab shows placeholder
- All knobs drag and update values
- Dropdowns populate with correct choices
- DualBore toggle works
- No console errors, no blank panel

---

## Files Modified

| File | Action | Description |
|------|--------|-------------|
| `Resources/ui/index.html` | **Replace** | Full Naturalist UI (~700 lines, inlined CSS+JS) |

**No C++ changes needed** — all relays, attachments, resource routes, and CMake binary data already configured from Stage 1.

---

## Success Criteria

- [ ] Three-tab navigation functional (Instrument / Tuning / FX)
- [ ] All 28 slider knobs render with seed cross-section style and respond to drag
- [ ] All 6 dropdowns populate from relay choices and are selectable
- [ ] DualBore toggle switches on/off
- [ ] All 35 parameters have two-way binding (UI ↔ host automation)
- [ ] XY pad renders preset markers, current position drags to update boreCharacter + doubleReed
- [ ] Collapsible sections animate open/closed, Primary Controls expanded by default
- [ ] Instrument panel scrolls natively
- [ ] Bore visualization placeholder renders in collapsible section
- [ ] Tuning panel loads and binds referencePitch + tuningSystem
- [ ] Ouaricon Naturalist aesthetic: aged paper bg, brown borders, green accents, Garamond type, seed knobs
- [ ] VST3 + AU build with zero errors
- [ ] No JS console errors on load
