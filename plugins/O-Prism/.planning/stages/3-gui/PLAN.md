# Stage 3: GUI - Execution Plan

## Goal

Implement the complete WebView-based GUI for O-Prism: 3-tab layout (SYNTH | TUNING | EFFECTS) at 1200x800 with Ouaricon Naturalist aesthetic, 73 slider + 1 toggle parameter bindings, wavetable Canvas displays, tuning panel integration, and two bug fixes from Stage 2 carry-over.

---

## Phase 3.1: Bug Fixes + Layout + Resource Infrastructure

### Task 1: Fix Bug — Filter Type BP24 Missing
- **Files:** `Source/PluginProcessor.cpp` (lines 159-160, 182-183)
- **Depends on:** none
- **Action:** Change filter type StringArrays from 6 to 7 choices:
  - `{ "LP12", "LP24", "HP12", "HP24", "BP", "Notch" }` -> `{ "LP12", "LP24", "HP12", "HP24", "BP12", "BP24", "Notch" }`
  - Both `filtAType` (line 160) and `filtBType` (line 183)
  - Rename "BP" to "BP12" for clarity
  - DSP already handles 7 types — this aligns APVTS choices with SVFFilter enum

### Task 2: Fix Bug — numSliderParams Count
- **Files:** `Source/PluginEditor.h` (line 65)
- **Depends on:** none
- **Action:** Change `static constexpr int numSliderParams = 67;` to `73`
  - Count verification: 10+10+5+4+5+5+5+1+7+4+4+3+3+4+3 = 73
  - The sliderParamIds array already has 73 entries (oscATable was missing from the count)

### Task 3: Copy Tuning Panel Module Assets
- **Files (create):**
  - `Source/ui/public/js/tuning-panel.js` (copy from `modules/tuning/scala-tuning-engine/js/tuning-panel.js`)
  - `Source/ui/public/css/tuning-panel.css` (copy from `modules/tuning/scala-tuning-engine/snippets/tuning-panel.css`)
- **Depends on:** none
- **Action:** Copy module files into plugin UI directory for BinaryData embedding

### Task 4: Obtain + Add Botanical Image
- **Files (create):** `Source/ui/public/images/botanical.png`
- **Depends on:** none
- **Action:** User must provide the botanical specimen image (BRIEF recommends butterfly; CONTEXT mentions snow bunting — user to confirm). Place in images directory. If not available, proceed with layout; image can be added later.
- **Note:** This task may block on user input. Layout can proceed without it.

### Task 5: Update CMakeLists.txt — Binary Resources
- **Files:** `CMakeLists.txt`
- **Depends on:** Tasks 3, 4
- **Action:** Add new files to `juce_add_binary_data`:
  ```cmake
  juce_add_binary_data(O-Prism_UIResources
      SOURCES
          Source/ui/public/index.html
          Source/ui/public/js/juce/index.js
          Source/ui/public/js/juce/check_native_interop.js
          Source/ui/public/js/tuning-panel.js
          Source/ui/public/css/tuning-panel.css
          Source/ui/public/images/botanical.png
  )
  ```

### Task 6: Update Resource Provider — URL Mappings
- **Files:** `Source/PluginEditor.cpp` (getResource function, lines 30-49)
- **Depends on:** Task 5
- **Action:** Add URL-to-BinaryData mappings for new files:
  - `/js/tuning-panel.js` -> `tuning_panel_js` (application/javascript)
  - `/css/tuning-panel.css` -> `tuning_panel_css` (text/css)
  - `/images/botanical.png` -> `botanical_png` (image/png)
  - Verify exact BinaryData variable names after first build

### Task 7: Build Complete index.html — Structure + Naturalist CSS
- **Files:** `Source/ui/public/index.html` (replace placeholder)
- **Depends on:** none (can be built concurrently with Tasks 1-6)
- **Action:** Create full HTML with:
  - **Inline CSS:** Ouaricon Naturalist design tokens (paper background `#F5E6D3`, text `#3C2F2F`, borders `#8B7355`, Garamond typography, uppercase section headers)
  - **CSS knob styles:** Seed cross-section conic-gradient knob (50px, 270-degree rotation, amber indicator)
  - **CSS tab styles:** Tab bar (`#D4C4B0` background, active state), tab-content show/hide
  - **CSS layout sections:** Header bar (O-PRISM branding), footer bar (persistent master strip)
  - **CSS botanical watermark:** Absolutely-positioned image with tab-shift classes and transitions
  - **HTML structure:**
    - Header bar with "O-PRISM" title + "Microtonal Wavetable Synthesizer" subtitle
    - Tab bar: SYNTH | TUNING | EFFECTS
    - SYNTH tab: Osc A section (canvas + 10 knobs), Osc B section (canvas + 10 knobs), Sub+Noise row, Filter A + Filter B sections, Amp Env + Filter Env sections
    - TUNING tab: `<div id="tuning-container"></div>` + supplementary tuning controls (pitch bend range, glide mode, glide time)
    - EFFECTS tab: Sub-tab bar (Reverb | Delay | Chorus | Distortion | EQ) + effect panels
    - Persistent footer: Master Volume knob, Osc Mix slider, Polyphony control
  - **HTML elements:** Each knob has `id="knob-{paramId}"` with `.knob-indicator` child; each value label has `id="val-{paramId}"`; dropdowns have `id="select-{paramId}"`
  - **Tab switching JS:** `switchTab()` function matching O-Marimba pattern
  - **Effects sub-tab JS:** `switchEffectTab()` for effects panel

### Task 8: Build + Verify Phase 3.1
- **Depends on:** Tasks 1-7
- **Action:**
  - Build VST3 + AU (`ninja O-Prism_VST3 O-Prism_AU`)
  - Verify BinaryData names match resource provider mappings (fix if needed)
  - Confirm WebView renders styled layout with tabs working
  - Confirm botanical watermark visible and shifting between tabs
  - Confirm filter type dropdown shows 7 choices
  - Confirm no build warnings from numSliderParams change

### Phase 3.1 Success Criteria
- [ ] Build passes (VST3 + AU, no errors)
- [ ] WebView shows Naturalist-styled 3-tab layout
- [ ] Tab switching works (SYNTH/TUNING/EFFECTS)
- [ ] Header bar and persistent footer visible on all tabs
- [ ] Botanical watermark visible and shifts position per tab
- [ ] Effects sub-tabs switch correctly
- [ ] All knob/dropdown HTML elements present with correct IDs
- [ ] Filter type bug fixed (7 choices in APVTS)
- [ ] numSliderParams bug fixed (73)

---

## Phase 3.2: Parameter Binding + Tuning Panel

### Task 9: Implement Core JS Binding Infrastructure
- **Files:** `Source/ui/public/index.html` (script section)
- **Depends on:** Task 7
- **Action:** In the `<script type="module">` block, implement:
  - `import * as Juce from './js/juce/index.js'`
  - `makeKnobDraggable(knobEl, state, onUpdate)` — mousemove drag handler with `sliderDragStarted()/sliderDragEnded()`, sensitivity `0.005`
  - `bindKnob(paramId, config)` — get slider state, wire knob indicator rotation (-135 to +135 deg), wire value label display
  - `bindDropdown(selectEl, paramId, numChoices)` — for choice params via slider relay, denormalize `Math.round(norm * (numChoices - 1))`
  - Double-click reset on knobs (call `state.setNormalisedValue(defaultNorm)`)
  - `requestAnimationFrame` debouncing for batch parameter updates on load

### Task 10: Define PARAMS Configuration Object
- **Files:** `Source/ui/public/index.html` (script section)
- **Depends on:** Task 9
- **Action:** Define data-driven parameter config:
  ```javascript
  const PARAMS = {
      oscAPos:    { label: 'Position', format: (n) => (n * 100).toFixed(0) + '%', default: 0.0 },
      oscALevel:  { label: 'Level',    format: (n) => (n * 100).toFixed(0) + '%', default: 0.8 },
      // ... all 73 slider params with label, format function, default normalized value
  };
  ```
  - Group by section for maintainability
  - Format functions must match actual parameter ranges (e.g., cutoff is log-scaled 20-20kHz, envelope times are skewed)
  - Choice params get special dropdown format functions

### Task 11: Bind All 73 Slider Parameters
- **Files:** `Source/ui/public/index.html` (script section)
- **Depends on:** Tasks 9, 10
- **Action:**
  - Loop over PARAMS, call `bindKnob()` for each continuous parameter
  - Call `bindDropdown()` for choice params: `filtAType` (7), `filtBType` (7), `filtRouting` (2), `subShape` (4), `noiseType` (6), `glideMode` (3), `delayMode` (2), `distType` (4), `tuningPreset` (11), `tonic` (12)
  - Wire oscATable/oscBTable dropdowns (4 factory shapes: Saw/Square/Triangle/Sine)

### Task 12: Bind delaySync Toggle
- **Files:** `Source/ui/public/index.html` (script section)
- **Depends on:** Task 9
- **Action:**
  - `const syncToggle = Juce.getToggleState('delaySync')`
  - Wire toggle button element to `setToggleState()` / `getToggleState()`
  - Update visual state on `toggleStateChangedEvent`

### Task 13: Integrate Tuning Panel
- **Files:** `Source/ui/public/index.html` (script section)
- **Depends on:** Tasks 3, 6, 7
- **Action:**
  - Add `<link rel="stylesheet" href="/css/tuning-panel.css">` in HTML head
  - In JS: `const { TuningPanel } = await import('./js/tuning-panel.js')`
  - Initialize: `const tuningPanel = new TuningPanel(document.getElementById('tuning-container'), Juce)`
  - Call `await tuningPanel.init()`
  - Override CSS variables if needed to match Naturalist aesthetic
  - Test: tuning presets load, scale generator works, embedded library browses

### Task 14: Add Missing Native Functions (if needed)
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** Task 13
- **Action:** Check if tuning panel calls functions not yet registered:
  - `applyGeneratedScale` — applies generated scale intervals to tuning engine
  - `saveScalaFile` / `saveKBMFile` — file export (if called by tuning panel)
  - Add any missing functions following the existing pattern

### Task 15: Build + Verify Phase 3.2
- **Depends on:** Tasks 9-14
- **Action:**
  - Build VST3 + AU
  - Test: all knobs respond to mouse drag, indicators rotate correctly
  - Test: value labels update in real-time during drag
  - Test: choice dropdowns switch correctly (filter types, osc shapes, etc.)
  - Test: delaySync toggle works
  - Test: tuning panel initializes and is functional
  - Test: host automation moves UI knobs (C++ -> JS sync)
  - Test: double-click resets knobs to default
  - Test: effects sub-tabs show correct parameters

### Phase 3.2 Success Criteria
- [ ] All 73 slider parameters controllable from WebView UI
- [ ] delaySync toggle functional
- [ ] Value labels display formatted values (%, Hz, dB, ms, etc.)
- [ ] Choice parameters display correct options in dropdowns
- [ ] Double-click reset works on all knobs
- [ ] Tuning panel renders and is functional in TUNING tab
- [ ] Host automation syncs to UI (C++ -> JS)
- [ ] UI changes sync to host (JS -> C++)
- [ ] Effects sub-tabs show correct parameters per effect

---

## Phase 3.3: Wavetable Display + Polish

### Task 16: Add Native Functions for Wavetable Display
- **Files:** `Source/PluginEditor.cpp`, `Source/PluginProcessor.h`
- **Depends on:** none (can parallel with Phase 3.2)
- **Action:** Add native functions to PluginEditor.cpp:
  - `getWavetableFrame(oscId, frameIndex)` — reads level 0, specified frame from factoryTables, returns JSON array of 256 downsampled floats (stride 8 over 2048 samples)
  - `getWavetableInfo(oscId)` — returns `{numFrames, shapeName}` JSON
  - `getWavetableFrameForPosition(oscId, normalizedPos)` — maps position 0-1 to frame index, returns 256 downsampled floats
  - Add public accessor to PluginProcessor.h: `const WavetableData* getFactoryTable(int index) const`

### Task 17: Implement WavetableDisplay Canvas Class
- **Files:** `Source/ui/public/index.html` (script section)
- **Depends on:** Task 16
- **Action:** Implement JS class:
  - Constructor: get canvas element, set up 2D context, handle DPI (`devicePixelRatio`)
  - `async fetchFrame(oscId, position)` — call native function, parse JSON response
  - `draw(samples)` — clear, draw aged paper background (`#EBD9C7`), center line, gradient fill (amber), waveform stroke (brown `#5C4033`), border
  - Create 2 instances: `wavetableDisplayA` (Osc A canvas), `wavetableDisplayB` (Osc B canvas)
  - Wire to `oscAPos`/`oscBPos` `valueChangedEvent` — on change, fetch frame and redraw
  - Also trigger redraw on `oscATable`/`oscBTable` change (shape selector)

### Task 18: Visual Polish
- **Files:** `Source/ui/public/index.html`
- **Depends on:** Tasks 15, 17
- **Action:**
  - Hover effects on knobs (subtle glow or border highlight)
  - Active/focus states on dropdowns
  - Consistent spacing and alignment audit across all sections
  - Ensure labels don't overflow at 50px knob width
  - Verify botanical watermark opacity and position looks good on all tabs
  - Section dividers between parameter groups (subtle `#8B7355` borders)
  - Footer bar visual separation from tab content

### Task 19: Final Build + Validation
- **Depends on:** Tasks 16-18
- **Action:**
  - Clean build VST3 + AU
  - Install and clear AU cache
  - Run pluginval (strictness 10)
  - Verify all tabs render correctly
  - Verify all 74 parameters (73 slider + 1 toggle) are controllable
  - Verify wavetable displays update on position/shape change
  - Verify tuning panel fully functional
  - Verify no console errors in WebView
  - Document any cross-platform considerations for Windows testing

### Phase 3.3 Success Criteria
- [ ] Wavetable Canvas displays render for both Osc A and Osc B
- [ ] Waveform updates when position knob changes
- [ ] Waveform updates when wavetable shape changes
- [ ] Waveform styled as "specimen illustration" (brown ink, aged paper)
- [ ] Visual polish: hover effects, focus states, consistent spacing
- [ ] pluginval PASSES (strictness 10)
- [ ] Build clean (VST3 + AU, no warnings)
- [ ] All 74 parameters functional end-to-end

---

## File Summary

### Files Modified
| File | Changes |
|------|---------|
| `Source/PluginProcessor.cpp` | Fix filtAType/filtBType to 7 choices (add BP12, BP24) |
| `Source/PluginEditor.h` | Fix numSliderParams 67 -> 73 |
| `Source/PluginEditor.cpp` | Add resource provider mappings, add wavetable native functions |
| `Source/PluginProcessor.h` | Add `getFactoryTable()` accessor |
| `CMakeLists.txt` | Add tuning-panel.js, tuning-panel.css, botanical.png to binary resources |
| `Source/ui/public/index.html` | Complete rewrite: full Naturalist UI with tabs, knobs, bindings |

### Files Created
| File | Purpose |
|------|---------|
| `Source/ui/public/js/tuning-panel.js` | Copy from scala-tuning-engine module |
| `Source/ui/public/css/tuning-panel.css` | Copy from scala-tuning-engine module |
| `Source/ui/public/images/botanical.png` | Specimen watermark image (user-provided) |

### Files Unchanged
All DSP files (`Source/dsp/*`) remain untouched. Stage 3 is UI-only except for the two bug fixes.

---

## Dependencies Graph

```
Task 1 (fix BP24) ────────────┐
Task 2 (fix numSliderParams) ─┤
Task 3 (copy tuning assets) ──┼─> Task 5 (CMake) ──> Task 6 (resource provider) ──┐
Task 4 (botanical image) ─────┘                                                    │
                                                                                    │
Task 7 (index.html layout) ───────────────────────────────────────────────────────┤
                                                                                    │
                                                  Task 8 (build 3.1) <─────────────┘
                                                         │
                        ┌────────────────────────────────┘
                        v
Task 9 (JS bind infra) ──> Task 10 (PARAMS obj) ──> Task 11 (bind 73 params)
                        │                                       │
                        ├──> Task 12 (bind toggle)              │
                        │                                       │
Task 13 (tuning panel) ─┤──> Task 14 (missing natives)         │
                        │                                       │
                        └──> Task 15 (build 3.2) <─────────────┘
                                    │
                                    v
Task 16 (wavetable natives) ──> Task 17 (Canvas class) ──> Task 18 (polish)
                                                                    │
                                                           Task 19 (final build)
```

---

## Risk Notes

1. **Botanical image not yet in repo** — BRIEF says butterfly, CONTEXT says snow bunting. User must provide. Layout works without it (watermark is decorative).
2. **73 parameters on one page** — SYNTH tab has 46 params. Research confirms 12-14 knobs per row at 50px fits. Careful section grouping essential.
3. **BinaryData name mangling** — JUCE converts hyphens/dots to underscores. Must verify exact names after first build and update resource provider if needed.
4. **Tuning panel CSS conflicts** — TuningPanel has its own styles that may need Naturalist overrides. Test during Phase 3.2.
5. **Choice params via slider relays** — Denormalization must use `Math.round(norm * (numChoices - 1))`. Off-by-one errors will map to wrong filter type, etc.
