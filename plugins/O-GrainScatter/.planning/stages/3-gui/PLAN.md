# Stage 3: GUI - Execution Plan

**Plugin:** O-GrainScatter
**Stage:** 3-gui
**Date:** 2026-02-07

---

## Goal

Implement the full Ouaricon Naturalist WebView UI for O-GrainScatter: real-time grain scatter visualization (Canvas 2D), Euclidean circle visualizer, 18 parameter controls (knobs, dropdowns, toggles), and lock-free C++→JS data flow — all at 900x700 in the botanical aesthetic.

---

## Tasks

### Layer 1: C++ Infrastructure (no UI dependencies)

#### Task 1: Add GrainVizSnapshot double-buffer to Processor
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Depends on:** none
- **Work:**
  - Add `GrainVizSnapshot` struct to `PluginProcessor.h`:
    - `struct Voice { bool active; float positionNorm; float pitchSemitones; float pan; float envelope; bool reverse; bool frozen; }`
    - `std::array<Voice, 64> voices; int activeCount;`
  - Add double-buffer: `std::array<GrainVizSnapshot, 2> vizSnapshots; std::atomic<int> vizWriteIndex{0};`
  - Add `std::atomic<int> currentEuclideanStep{0};` for Euclidean viz
  - Add public getter: `const GrainVizSnapshot& getVizSnapshot() const { return vizSnapshots[1 - vizWriteIndex.load(std::memory_order_acquire)]; }`
  - Add public getters: `const std::array<bool, 16>& getEuclideanPattern() const`, `int getEuclideanStep() const`, `int getEuclideanSteps() const`
  - At end of `processBlock()`: populate write-side snapshot from `grainPool.voices` data, then flip `vizWriteIndex`
  - Update `currentEuclideanStep` in scheduler callback (expose from GrainScheduler)

#### Task 2: Expose Euclidean step from GrainScheduler
- **Files:** `Source/dsp/GrainScheduler.h`
- **Depends on:** none
- **Work:**
  - Add `int getEuclideanStep() const { return euclideanStep; }`
  - This lets the Processor read current step after `processBlockSync()` and store to atomic

#### Task 3: Make GrainPool voices readable for viz snapshot
- **Files:** `Source/dsp/GrainPool.h`
- **Depends on:** none
- **Work:**
  - Add `const std::array<GrainVoice, MaxVoices>& getVoices() const { return voices; }`
  - The Processor will copy grain data into viz snapshot at end of processBlock

### Layer 2: C++ Editor + CMake (depends on Layer 1)

#### Task 4: Wire timerCallback to emit grain + Euclidean data to WebView
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** Tasks 1, 2, 3
- **Work:**
  - In `timerCallback()`: read viz snapshot via `audioProcessor.getVizSnapshot()`
  - Build JSON string with active grain data: `{"grains":[{"a":1,"p":0.5,"s":3.0,"pan":0.3,"e":0.8,"r":0,"f":1},...],"ac":12}`
    - Short keys for bandwidth: a=active, p=positionNorm, s=pitchSemitones, pan=pan, e=envelope, r=reverse, f=frozen, ac=activeCount
  - Call `webView->emitEventIfBrowserIsVisible("grainUpdate", jsonString)`
  - Build Euclidean JSON: `{"pattern":[1,0,1,0,1,0,1,0],"step":3,"steps":8}`
  - Call `webView->emitEventIfBrowserIsVisible("euclideanUpdate", euclideanJson)`

#### Task 5: Update window size and getResource() routes
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** none
- **Work:**
  - Change `setSize(800, 500)` → `setSize(900, 700)`
  - Add route for `/js/app.js` → `BinaryData::app_js` / `BinaryData::app_jsSize`

#### Task 6: Update CMakeLists.txt with new UI files
- **Files:** `CMakeLists.txt`
- **Depends on:** none
- **Work:**
  - Add `Source/ui/public/js/app.js` to `juce_add_binary_data` SOURCES list

### Layer 3: UI Files (depends on Layer 2 for build, but can be written in parallel)

#### Task 7: Create full index.html with Naturalist layout
- **Files:** `Source/ui/public/index.html`
- **Depends on:** none (replaces placeholder)
- **Work:**
  - 900x700 layout with inline CSS (Naturalist aesthetic)
  - Background: aged paper (`#F5E6D3`) with subtle texture gradient
  - Header: "O-GrainScatter" in Garamond, small tagline
  - Visualization area (~40% height, top):
    - Left ~65%: `<canvas id="grain-scatter">` for grain scatter plot
    - Right ~35%: `<canvas id="euclidean-circle">` for Euclidean pattern
  - Controls area (~60% height, bottom):
    - **Group 1 — Core Engine** (6 knobs): Grain Size, Density, Spread, Reverse, Feedback, Dry/Wet
    - **Group 2 — Pitch & Scale** (2 knobs + 3 dropdowns): Pitch Random, Pan Random, Scale, Root Note, Pitch Mode
    - **Group 3 — Beat Sync** (3 knobs + 1 dropdown + 1 toggle): Probability, Repeats, Sync Mode, Stutter Gate
    - **Group 4 — Euclidean** (2 knobs): Pulses, Steps
    - **Freeze toggle** positioned prominently between viz and controls
  - Each knob: botanical seed cross-section CSS, conic gradient, indicator triangle
  - Dropdowns: serif font, earth-tone borders, parchment background
  - Toggles: botanical green with active glow; Freeze has pulse animation
  - Group labels: small caps Garamond, earth-tone colors
  - All parameter elements have `data-param="param_id"` attributes for JS binding
  - Script tag: `<script type="module" src="./js/juce/index.js"></script>` + `<script src="./js/app.js"></script>`

#### Task 8: Create js/app.js — parameter binding + visualizations
- **Files:** `Source/ui/public/js/app.js`
- **Depends on:** Task 7 (needs HTML element IDs)
- **Work:**
  - **Parameter binding system:**
    - `initSliders()`: For each of 12 slider params, call `Juce.getSliderState(id)`, set up mousedown/mousemove/mouseup drag, rotation indicator, value display, `sliderDragStarted()`/`sliderDragEnded()`
    - `initComboBoxes()`: For each of 4 combobox params, call `Juce.getComboBoxState(id)`, bind change events, sync initial value
    - `initToggles()`: For 2 toggle params, call `Juce.getToggleState(id)`, bind click to toggle, sync active class
    - All use JUCE 8 pattern: `state.valueChangedEvent.addListener(() => { const val = state.getXxx(); ... })`
  - **GrainScatterViz class (Canvas 2D):**
    - Constructor: get canvas, set up retina scaling
    - `update(grainData)`: store latest grain array
    - `draw()`: clear, draw faint grid lines (time axis labels), draw each active grain as circle at (positionNorm * width, pitchMap(semitones)), radius proportional to envelope, alpha from envelope, brown for live / green for frozen, reverse grains get a small arrow indicator
  - **EuclideanCircleViz class (Canvas 2D):**
    - Constructor: canvas, retina scaling
    - `update(pattern, currentStep, steps)`: store state
    - `draw()`: draw circle outline, draw step dots evenly spaced, active pulses filled brown, current step highlighted green/larger, inactive steps faded, connect active steps with subtle arcs
  - **Event listeners:**
    - `window.__JUCE__.backend.addEventListener('grainUpdate', ...)` → parse JSON, call `grainViz.update()`
    - `window.__JUCE__.backend.addEventListener('euclideanUpdate', ...)` → parse JSON, call `euclideanViz.update()`
  - **Render loop:** `requestAnimationFrame` at 60 FPS, calls `grainViz.draw()` and `euclideanViz.draw()`
  - **Knob rotation:** -140° to +140° range (280° total), mapped from normalized 0-1

### Layer 4: Integration + Polish (depends on all above)

#### Task 9: Populate viz snapshot in processBlock
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Tasks 1, 2, 3
- **Work:**
  - At end of `processBlock()`, after the sample loop:
    - Get write-side snapshot: `auto& snap = vizSnapshots[vizWriteIndex.load(std::memory_order_relaxed)];`
    - Iterate `grainPool.getVoices()`, populate each `snap.voices[i]` with:
      - `active`, `positionNorm` (positionOffset normalized to 2s buffer), `pitchSemitones` (log2(playbackRate)*12), `pan`, `envelope` (1 - samplesRemaining/grainLength), `reverse`, `frozen` (readFromFrozen)
    - Count active voices into `snap.activeCount`
    - Flip: `vizWriteIndex.store(1 - vizWriteIndex.load(std::memory_order_relaxed), std::memory_order_release);`
  - After scheduler call, store: `currentEuclideanStep.store(scheduler.getEuclideanStep(), std::memory_order_relaxed);`

#### Task 10: Build and verify
- **Files:** none (build/test)
- **Depends on:** Tasks 1-9
- **Work:**
  - Run CMake reconfigure (new binary data files)
  - Build: `ninja OuariconGrainScatter_VST3 OuariconGrainScatter_AU`
  - Open Standalone and verify:
    - All 18 controls visible and responsive
    - Knob drag changes parameter values
    - Grain scatter visualization shows dots appearing/fading
    - Euclidean circle updates when pulses/steps change
    - Freeze toggle shows glow animation
    - No console errors in WebView
  - Run pluginval at strictness 5

---

## File Summary

| File | Action | Description |
|------|--------|-------------|
| `Source/PluginProcessor.h` | Modify | Add GrainVizSnapshot, double buffer, Euclidean step atomic, getters |
| `Source/PluginProcessor.cpp` | Modify | Populate viz snapshot end of processBlock, store Euclidean step |
| `Source/dsp/GrainPool.h` | Modify | Add `getVoices()` const accessor |
| `Source/dsp/GrainScheduler.h` | Modify | Add `getEuclideanStep()` const accessor |
| `Source/PluginEditor.cpp` | Modify | timerCallback emits events, window size 900x700, new getResource route |
| `CMakeLists.txt` | Modify | Add app.js to binary data sources |
| `Source/ui/public/index.html` | Replace | Full Naturalist UI layout with all controls |
| `Source/ui/public/js/app.js` | Create | Parameter binding, grain viz, Euclidean viz, render loop |

---

## Success Criteria

- [ ] Window renders at 900x700 with Naturalist aesthetic (aged paper, botanical knobs, Garamond)
- [ ] All 18 parameters bound and functional (12 knobs, 4 dropdowns, 2 toggles)
- [ ] Knob drag correctly updates parameter values in processor (verify with DAW automation)
- [ ] Grain scatter canvas shows active grains as fading dots (position x pitch)
- [ ] Euclidean circle shows pattern with current step highlighted
- [ ] Freeze toggle has glow/pulse animation when active
- [ ] Grain visualization changes appearance when frozen (green tint)
- [ ] No audio glitches from viz snapshot mechanism (double buffer lockfree)
- [ ] pluginval passes at strictness 5
- [ ] Build succeeds for VST3 + AU

---

## Risks & Mitigations

| Risk | Mitigation |
|------|------------|
| BinaryData name collision (app_js vs index_js) | JUCE mangles filenames — verify generated names in BinaryData.h |
| Canvas blurry on Retina | Scale by devicePixelRatio, set CSS size vs canvas size |
| Drag sensitivity too fast/slow | Start with 0.005 per pixel (O-IntonationPad tested value), tune if needed |
| JSON too large for 64 grains | Only send active grains (~5KB worst case at 30Hz — well within budget) |
| emitEventIfBrowserIsVisible not available | Guarded by JUCE 8 WebView — already verified in O-SpectralShaper |
