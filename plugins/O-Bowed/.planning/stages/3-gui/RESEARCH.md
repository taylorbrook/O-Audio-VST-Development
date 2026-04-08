# Stage 3: GUI - Research

**Researched:** 2026-04-05
**Domain:** JUCE 8 WebView UI, Ouaricon Naturalist aesthetic, canvas visualizations, preset module
**Confidence:** HIGH

## Summary

O-Bowed Stage 3 GUI builds a full WebView-based UI for a 900x600 bowed string synthesizer with 23 APVTS parameters, 3 canvas visualizations (bow-string animation, body resonance spectrum, Schelleng diagram), and the Ouaricon Naturalist aesthetic. The existing PluginEditor has 20 WebSliderRelays + 1 WebComboBoxRelay wired, a resource provider serving index.html and tuning panel JS/CSS, and WinWebView2 configured. Two relays are missing: `frictionTier` (WebComboBoxRelay) and `bowNoise` (WebSliderRelay) -- added during DSP phases but not yet reflected in the editor.

**Key findings:**
1. Layout is tight (23 params + 3 visualizations in 900x600) but feasible with tabbed center visualization panel and 55px seed knobs in flanking columns
2. All 3 visualizations are Canvas 2D (no WebGL needed) -- bow-string animation, frequency response curve, and 2D scatter plot are well within Canvas 2D capability
3. DSP state for visualizations requires a native function polling pattern (timer-based, ~15-30Hz) -- NOT relay-backed since viz data isn't APVTS parameters
4. Preset module integration follows established O-Bells/O-Wind pattern via getNativeFunction()
5. O-Orbit provides the definitive seed knob CSS + relative drag JS pattern

**Primary recommendation:** Follow O-Wind structure (single index.html, inline CSS, module-pattern JS) with O-Orbit seed knob pattern. Phase 4.1 = layout + all 23 controls bound. Phase 4.2 = full binding verification + host automation sync. Phase 4.3 = 3 canvas visualizations + preset browser + tuning file browser.

## User Constraints (from CONTEXT.md)

### Locked Decisions
- Visual aesthetic: Ouaricon Naturalist (ouaricon-naturalist-001)
- Window size: 900x600
- Layout: Center viz panel with flanking controls (Bow left, Body/Strings right)
- Visualizations: All 3 (bow-string, body spectrum, Schelleng) in tabbed center panel
- Friction tier: 3-way toggle/selector (Core/Enhanced/Quality)
- Per-string tuning: Conditionally visible based on stringCount
- Botanical illustration: TBD (horsehair/rosin plant or wood grain suggested)

### Phase Structure (from ROADMAP)
| Phase | Goal |
|-------|------|
| 4.1 | Layout + basic knob controls (HTML/CSS structure, seed knobs, section grouping) |
| 4.2 | Full two-way parameter binding (all 23 params, host automation sync) |
| 4.3 | Visualizations (bow-string, body spectrum, Schelleng), preset browser, tuning file browser |

---

## 1. C++ Changes Required

### Missing Relays (added during DSP phases, not in editor)

Two parameters were added during Stage 2 DSP phases that need relay + attachment wiring:

**frictionTier** (choice parameter, 3 options: Core/Enhanced/Quality):
```cpp
// In PluginEditor.h -- add to relays section:
std::unique_ptr<juce::WebComboBoxRelay> frictionTierRelay;
// In PluginEditor.h -- add to attachments section:
std::unique_ptr<juce::WebComboBoxParameterAttachment> frictionTierAttachment;

// In PluginEditor.cpp constructor:
frictionTierRelay = std::make_unique<juce::WebComboBoxRelay>("frictionTier");
// ... in WebView options chain:
.withOptionsFrom(*frictionTierRelay)
// ... in attachments section:
frictionTierAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
    *apvts.getParameter("frictionTier"), *frictionTierRelay, nullptr);
```

**bowNoise** (float parameter, 0.0-1.0):
```cpp
// In PluginEditor.h -- add to relays section:
std::unique_ptr<juce::WebSliderRelay> bowNoiseRelay;
// In PluginEditor.h -- add to attachments section:
std::unique_ptr<juce::WebSliderParameterAttachment> bowNoiseAttachment;

// In PluginEditor.cpp constructor:
bowNoiseRelay = std::make_unique<juce::WebSliderRelay>("bowNoise");
// ... in WebView options chain:
.withOptionsFrom(*bowNoiseRelay)
// ... in attachments section:
bowNoiseAttachment = std::make_unique<juce::WebSliderParameterAttachment>(
    *apvts.getParameter("bowNoise"), *bowNoiseRelay, nullptr);
```

### Preset Module Integration

O-Bowed needs OuariconPresetManager added for preset browse/save/load. Pattern from O-Bells:

```cpp
// PluginProcessor.h:
#include "OuariconPresetManager.h"
OuariconPresetManager presetManager;

// PluginProcessor.cpp constructor:
presetManager.initialize("O-Bowed", parameters);

// PluginEditor.cpp -- register native functions:
webView->getNativeFunction("getPresetListWithCategories", [this]() { ... });
webView->getNativeFunction("loadPresetFromCategory", [this](String cat, String name) { ... });
webView->getNativeFunction("selectPreviousPreset", [this]() { ... });
webView->getNativeFunction("selectNextPreset", [this]() { ... });
webView->getNativeFunction("savePresetWithDialog", [this]() { ... });
webView->getNativeFunction("getCurrentPreset", [this]() { ... });
```

**Module path:** `modules/persistence/preset-manager/cpp/OuariconPresetManager.h`
**Add via:** `/module-add O-Bowed preset-manager`

### Visualization Data Bridge

Visualizations need DSP state that isn't APVTS-backed. Two approaches exist in codebase:

**Option A: Native function polling (recommended for Phase 4.3)**
- JS timer calls `getNativeFunction('getVisualizationState')()` at 15-30Hz
- Returns JSON with bow state, body coefficients, and current playing position
- Used by O-SpectralShaper for spectrogram data
- Thread-safe: processor writes atomic snapshot, editor reads on message thread

**Option B: Timer-based emitEvent push (O-Orbit pattern)**
- C++ timer in editor pushes state via `webView->emitEventIfBrowserIsVisible()`
- Used by O-Orbit for motion state at 30Hz
- More efficient (no JS polling overhead) but requires timer management in editor

**Recommendation:** Option A for simplicity. The viz data is small (bow speed/pressure/position, 8 body resonance peaks, current Schelleng coordinates). One native function returns all viz state as JSON.

```cpp
// In PluginProcessor:
struct VisualizationState {
    float bowSpeed, bowPressure, bowPosition;
    float bodyPeaks[8];      // Current body resonance peak frequencies
    float bodyGains[8];      // Current body resonance peak gains
    float schellengX, schellengY; // Current position in Schelleng space
    bool isPlaying;
};
std::atomic<VisualizationState> vizState; // Updated in processBlock
```

### Resource Provider Updates

New files to serve (add routes to `getResource()`):
- `/css/styles.css` (if external) or inline in index.html
- `/img/botanical.png` (botanical illustration)
- Any additional JS modules

CMakeLists.txt `juce_add_binary_data` needs to include new resources.

---

## 2. Layout Analysis

### Space Budget (900x600)

```
Header bar:          40px  (preset browser + title + tuning dropdown)
Main content area:  520px  (controls + viz)
Output bar:          40px  (Width + Level)
─────────────────────────
Total:              600px
```

Main content split (520px tall × 900px wide):
```
Left column:    180px  (Bow section: 5 knobs + tier selector)
Center panel:   540px  (Tabbed viz + Impossible Physics below)
Right column:   180px  (Body: 3 knobs + Strings: up to 7 knobs)
```

### Knob Count Per Section

| Section | Knobs | Notes |
|---------|-------|-------|
| Bow (left) | 5 | bowSpeed, bowPressure, bowPosition, rosin, bowNoise |
| Bow extras (left) | 1 | frictionTier (3-way selector, not knob) |
| Body (right top) | 3 | bodyMaterial, bodySize, brightness |
| Strings (right bottom) | 2-6 | stringCount, sympatheticAmount, sympatheticCount, + stringTuning1-4 (conditional) |
| Impossible (center bottom) | 3 | infiniteSustain, reversedFriction, subHarmonics |
| Output (footer) | 2 | width, outputLevel |
| Tuning (header) | 2 | referencePitch (knob or number), tuningSystem (dropdown) |
| **Total** | **18-22** | 4 string tuning knobs conditional on stringCount > 1 |

### Vertical Space Analysis

**Left column (180px × 520px):**
- Section label: 20px
- 5 knobs at 55px + 20px label + 8px gap = ~83px each → 415px
- Tier selector: 40px
- Total: ~475px ✓ fits

**Right column (180px × 520px):**
- Body section label: 20px
- 3 body knobs: ~249px
- Strings section label: 20px
- stringCount + sympatheticAmount + sympatheticCount: ~249px (tight but fits)
- Per-string tuning (conditional): Would NOT fit without collapsing
- **Solution:** Per-string tuning in a compact row of 4 mini-knobs (40px) below stringCount, only visible when stringCount > 1

**Center panel (540px × 520px):**
- Tab selector: 30px
- Viz canvas: ~340px
- Impossible Physics row: 100px (3 knobs horizontal)
- Remaining: 50px padding/margins ✓

### Conditional Visibility Strategy

Per-string tuning knobs (stringTuning1-4) appear/disappear based on `stringCount`:
- stringCount = 1: Hide all tuning knobs
- stringCount = 2: Show tuning 1-2
- stringCount = 3: Show tuning 1-3
- stringCount = 4: Show tuning 1-4

Sympathetic controls: Show sympatheticAmount only when sympatheticCount > 0

JS implementation: Listen to `stringCount` relay's `valueChangedEvent`, toggle CSS class on tuning container.

---

## 3. Visualization Research

### 3.1 Bow-String Animation (Default Tab)

**Canvas 2D approach** -- simple, performant, well-suited:

Visual elements:
- Horizontal string line (bezier curve with wave displacement)
- Bow contact point (vertical indicator at beta position)
- Bow arrow/angle showing speed and pressure
- String vibration as sinusoidal displacement scaled by amplitude

```
┌─────────────────────────────────────────┐
│                                         │
│  nut ─────╱╲╱╲───⟨BOW⟩───╱╲╱╲── bridge │
│                    ↕                    │
│              bow pressure               │
│              ←→ bow speed               │
│                                         │
└─────────────────────────────────────────┘
```

**Data needed from DSP (per animation frame):**
- `bowPosition` (0.02-0.30): Where bow contacts string
- `bowSpeed` (0.02-2.0): Arrow length/speed indicator
- `bowPressure` (0.01-5.0): Bow penetration depth
- `isPlaying`: Whether a note is active
- `stringAmplitude`: Current RMS or peak of waveguide output

**Animation approach:**
- `requestAnimationFrame` at 60fps
- Bow position updates from parameter relay (instant)
- String vibration: Smooth sinusoidal wave, amplitude from DSP state
- When not playing: String at rest (straight line), bow lifted

**Complexity:** LOW -- simple geometric shapes, no WebGL needed

### 3.2 Body Resonance Spectrum (Tab 2)

**Canvas 2D frequency response plot:**

Visual elements:
- Log-frequency X-axis (20Hz - 20kHz)
- dB Y-axis (-24dB to +12dB)
- 8 resonance peaks drawn as smooth curve
- Grid lines at octave intervals
- Color-coded by material type (green=wood, blue=metal, amber=membrane, white=glass)

```
┌─────────────────────────────────────────┐
│ +12 ─────────────────────────────────── │
│      ╱╲                                 │
│  0  ╱  ╲   ╱╲                          │
│    ╱    ╲ ╱  ╲    ╱╲                   │
│ -24─────────────────────────────────── │
│   20   100   1k   5k   10k   20k  Hz  │
└─────────────────────────────────────────┘
```

**Data needed from DSP:**
- 8 peak frequencies (Hz)
- 8 peak gains (dB)
- 8 peak Q values (for curve width)
- Current material type (for color coding)

These are derived from BodyResonator's current biquad coefficients. Can be computed on message thread from the current Material and Size parameter values (no need for real-time DSP data -- it's deterministic from parameters).

**Optimization:** Only redraw when Material or Size parameters change (not every frame).

**Complexity:** MEDIUM -- requires biquad frequency response calculation and smooth curve rendering

### 3.3 Schelleng Diagram (Tab 3)

**Canvas 2D scatter plot with region overlay:**

Visual elements:
- X-axis: Bow position (beta, 0.02-0.30)
- Y-axis: Bow pressure (N, 0.01-5.0, log scale)
- Colored region showing Helmholtz motion zone (playable region)
- Current playing point as crosshair/dot
- Region boundaries from Schelleng theory: min/max pressure as function of position

```
┌─────────────────────────────────────────┐
│ 5.0  ╲                                  │
│       ╲ RAUCOUS (scratchy)              │
│  P     ╲▓▓▓▓▓▓▓▓▓▓╱                   │
│  r      ▓HELMHOLTZ▓  ╱                  │
│  e      ▓(good bow)▓╱                   │
│  s     ╱▓▓▓▓▓▓▓▓▓▓╱                    │
│  s   ╱   SURFACE SOUND                  │
│ 0.01╱──────────────────                  │
│   0.02    0.12    0.20    0.30  Position │
└─────────────────────────────────────────┘
```

**Schelleng boundaries (analytical):**
- Min pressure: P_min ∝ v_B / (β² × Z) -- below this, surface sound (no stick-slip)
- Max pressure: P_max ∝ v_B / (β × Z) -- above this, raucous (multiple slips)
- Where β = bow position, v_B = bow speed, Z = string impedance

These boundaries can be computed from current bowSpeed + string properties. The "playable region" is the area between min and max pressure curves.

**Data needed:**
- Current bowPosition (from relay -- already bound)
- Current bowPressure (from relay -- already bound)
- Current bowSpeed (from relay -- for boundary computation)
- String impedance Z (fixed for current pitch -- can approximate)

**Key insight:** This visualization is mostly parameter-derived, not DSP-state. The boundaries shift with bowSpeed, and the dot moves with bowPosition/bowPressure. Can update reactively from relay valueChangedEvents -- no polling needed.

**Complexity:** MEDIUM -- analytical curves + region fill + reactive dot

---

## 4. Reference Plugin Patterns

### Seed Knob (O-Orbit -- definitive pattern)

**CSS:** 10-segment conic-gradient with radial core circle and outer ring
- File: `plugins/O-Orbit/Resources/ui/css/styles.css` lines 128-170
- Size: 55px × 55px
- Indicator: Green bar (`#8BA870`) at top, rotated with knob value
- Rotation: `transform: rotate(Xdeg)` on `.knob::after` pseudo-element
- Range: -135° to +135° (270° total sweep)

**JS relative drag interaction:**
- `mousedown` → `state.sliderDragStarted()`, record lastY
- `mousemove` → compute delta, `state.setNormalisedValue(clamp(norm + delta))`
- `mouseup` → `state.sliderDragEnded()`
- `dblclick` → reset to default (if `propertiesChangedEvent` provides default)
- Sensitivity: ~200px full range

### Tab Switching (O-Wind/O-Bells pattern)

```javascript
const tabs = document.querySelectorAll('.tab');
const contents = document.querySelectorAll('.tab-content');
tabs.forEach(tab => {
    tab.addEventListener('click', () => {
        tabs.forEach(t => t.classList.remove('active'));
        contents.forEach(c => c.classList.remove('active'));
        tab.classList.add('active');
        document.getElementById(tab.dataset.tab).classList.add('active');
    });
});
```

### Preset Browser (O-Bells pattern)

- Header bar with ◀ ▶ nav buttons, center preset name, save/load buttons
- Dropdown appears on click of preset name
- Uses `getNativeFunction()` for all preset operations
- Requires OuariconPresetManager module in processor

### Parameter Binding (O-Orbit pattern)

```javascript
import { getSliderState, getComboBoxState } from './juce/index.js';

function bindKnob(paramId) {
    const el = document.getElementById(paramId);
    const state = getSliderState(paramId);
    const updateUI = () => {
        const norm = state.getNormalisedValue();
        el.style.setProperty('--rotation', `${-135 + norm * 270}deg`);
    };
    state.valueChangedEvent.addListener(updateUI);
    updateUI(); // Initial sync
    // ... mouse drag handlers
}

function bindDropdown(paramId) {
    const el = document.getElementById(paramId);
    const state = getComboBoxState(paramId);
    state.valueChangedEvent.addListener(() => { /* update UI */ });
}
```

---

## 5. Aesthetic Application

### Template: ouaricon-naturalist-001

Full spec at: `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md`

**Key CSS variables (from O-Wind reference):**
```css
:root {
    --bg-paper: #F5E6D3;
    --bg-paper-dark: #E8D5BE;
    --brown-text: #3C2F2F;
    --brown-border: #8B7355;
    --brown-frame: #5C4033;
    --green-light: #8BA870;
    --green-mid: #6B8E4E;
    --knob-size: 55px;
}
```

**Botanical Illustration:**
- Plugin type: Synthesizer (bowed strings)
- Character: Warm, classical, physical
- Suggested category: fauna or anatomy
- Best fit: **Horsehair plant / flax botanical** (connects to bow hair material) or **wood grain cross-section** (connects to instrument body)
- Alternatively: Bird illustration (strings → birdsong connection) or anatomy (string vibration → physical structure)
- Placement: Right side, 70% height, opacity 0.12-0.25, pointer-events: none
- Image format: PNG with transparent background from `Ouaricon Audio Images/`

**Typography:**
- Font: Garamond, Georgia, Times New Roman, serif
- Title: 18-22px, uppercase, letter-spacing 2px
- Section labels: 12-14px, uppercase, letter-spacing 1px
- Param labels: 9-11px, uppercase, letter-spacing 0.5px

---

## 6. Module Opportunities

### Preset Manager (RECOMMENDED)

**Module:** `modules/persistence/preset-manager`
**Status:** Used by O-Bells, O-Wind, O-Orbit, O-FreqPulse
**Add via:** `/module-add O-Bowed preset-manager`

Required for Phase 4.3 preset browser. Provides:
- Factory/user preset storage
- JSON serialization
- Category organization
- Previous/next navigation
- Native function bindings for WebView

### Tuning Panel (ALREADY LINKED)

Already in CMakeLists.txt and resource provider. The tuning panel JS/CSS is served. Just needs initialization in the index.html Tuning tab (follows O-Bells/O-Wind pattern with lazy loading).

### VU Meter (OPTIONAL)

**Module:** `modules/metering/vu-meter`
Could add output level metering. Low priority -- O-Bowed is a synth, metering is nice-to-have.

---

## 7. Pitfalls & Mitigations

### Known Issues

1. **Canvas replaced element gotcha** (from MEMORY.md): `<canvas>` ignores `right: Npx; bottom: Npx;` for sizing. Use explicit `width: calc()` and `height: calc()`. Also need DPR-aware backing store for Retina.

2. **Resource provider receives paths, not URLs** (from MEMORY.md): Compare `url == "/img/botanical.png"` directly, not after stripping scheme.

3. **WebView2 user data folder** (from MEMORY.md): Already configured in PluginEditor.cpp with `withUserDataFolder(tempDirectory)`. ✓

4. **Destruction order**: Relays → WebView → Attachments. Already correct in PluginEditor.h member ordering. New relays (frictionTier, bowNoise) must follow same ordering.

5. **900x600 space constraint**: 23 params + 3 visualizations is dense. Conditional visibility for per-string tuning and compact "Impossible Physics" horizontal row are essential.

6. **Visualization performance**: Three canvases but only one visible at a time (tabbed). Only animate the active tab. Use `requestAnimationFrame` with visibility check.

7. **Body spectrum calculation**: Computing biquad frequency response requires `std::complex` math on message thread. Pre-compute on parameter change, not per-frame. Cache the curve points.

### Cross-Platform Notes

- WebView2 static linking already configured (`JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` in CMakeLists.txt) ✓
- Resource provider root: Use `getResourceProviderRoot()` in C++, `getBackendResourceAddress()` in JS -- never hard-code scheme ✓
- Font fallback: Garamond → Georgia → Times New Roman → serif ensures cross-platform rendering

---

## 8. Phase-Specific Recommendations

### Phase 4.1: Layout + Basic Controls

**Scope:**
1. Add frictionTier and bowNoise relays/attachments to PluginEditor
2. Replace placeholder index.html with full Naturalist layout
3. All 23 parameter controls rendered (seed knobs + dropdowns + tier selector)
4. Section grouping: Bow (left), Body/Strings (right), Viz tabs (center), Impossible (center bottom), Output (footer)
5. Botanical illustration placed
6. Update CMakeLists.txt binary resources

**Template:** Start from O-Wind index.html structure, adapt layout to center-viz flanking design.

### Phase 4.2: Full Parameter Binding

**Scope:**
1. JS binding for all 23 params via getSliderState/getComboBoxState
2. Host automation → UI updates (valueChangedEvent listeners)
3. UI → host (sliderDragStarted/setNormalisedValue/sliderDragEnded)
4. Value display formatting (units: m/s, N, Hz, cents, dB, %)
5. Conditional visibility (stringTuning1-4, sympatheticAmount)
6. Preset change updates all UI elements

### Phase 4.3: Visualizations + Preset Browser + Tuning

**Scope:**
1. Bow-string animation canvas (default tab)
2. Body resonance spectrum canvas (tab 2)
3. Schelleng diagram canvas (tab 3)
4. Native function for viz state polling (~15Hz)
5. Preset browser (OuariconPresetManager integration)
6. Tuning panel initialization (lazy-loaded, shared module)
7. Scala/TUN file browser via native function

---

## 9. File Structure Plan

```
plugins/O-Bowed/
  Resources/ui/
    index.html          ← Full Naturalist UI (single file, inline CSS+JS)
    img/
      botanical.png     ← Botanical illustration (TBD selection)
    js/juce/
      index.js          ← JUCE WebView bridge (existing)
      check_native_interop.js  ← (existing)
    css/
      tuning-panel.css  ← (served from shared module, existing)
    js/
      tuning-panel.js   ← (served from shared module, existing)
  Source/
    PluginEditor.h      ← Add frictionTierRelay, bowNoiseRelay + attachments
    PluginEditor.cpp    ← Add relay creation, WebView options, attachments, new resource routes
    PluginProcessor.h   ← Add OuariconPresetManager, VisualizationState, native functions
    PluginProcessor.cpp ← Add preset init, viz state snapshot, native function registration
```

---

## 10. Complexity Assessment

| Phase | Estimated Complexity | Rationale |
|-------|---------------------|-----------|
| 4.1 | MEDIUM | 23 controls + seed knob CSS + layout + C++ relay additions |
| 4.2 | LOW-MEDIUM | Binding patterns well-established, conditional visibility adds some complexity |
| 4.3 | HIGH | 3 canvas visualizations + DSP data bridge + preset module + tuning panel |

**Overall Stage 3:** MEDIUM-HIGH. The visualizations in 4.3 are the most complex part. Layout in 4.1 requires careful space management. Binding in 4.2 is mostly mechanical.
