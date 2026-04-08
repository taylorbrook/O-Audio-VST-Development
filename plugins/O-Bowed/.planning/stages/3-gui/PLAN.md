# Stage 3: GUI - Execution Plan

**Created:** 2026-04-05
**Stage:** 3-gui (Phases 4.1, 4.2, 4.3)
**Goal:** Build full WebView UI for O-Bowed with Ouaricon Naturalist aesthetic, 23-parameter control surface, and 3 canvas visualizations

---

## Phase 4.1: Layout + All Controls + Binding

**Goal:** Replace placeholder index.html with complete Naturalist UI. All 23 parameters rendered with seed knobs/dropdowns, two-way bound, and section-grouped. Add missing C++ relays.

### Task 1: Add missing frictionTier and bowNoise relays to C++ editor

**Files:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
**Depends on:** none

- Add `frictionTierRelay` (WebComboBoxRelay) and `bowNoiseRelay` (WebSliderRelay) to header, in relay section (before webView)
- Add `frictionTierAttachment` (WebComboBoxParameterAttachment) and `bowNoiseAttachment` (WebSliderParameterAttachment) to header, in attachment section (after webView)
- Create relays in constructor before WebView construction
- Register both with `.withOptionsFrom()` in WebView options chain
- Create attachments after WebView construction
- Maintains CRITICAL ordering: Relays -> WebView -> Attachments

### Task 2: Build complete index.html with Naturalist layout and all controls

**Files:** `Resources/ui/index.html`
**Depends on:** Task 1 (relay IDs must match)

Full single-file HTML/CSS/JS implementing:

**Layout (900x600):**
```
+--------------------------------------------------+
|  O-BOWED           [Preset ◀ ▶]  [Tuning ▼]     |  40px header
+----------+---------------------+-----------------+
|   BOW    |   CENTER VIZ PANEL  |   BODY          |
| Speed    |  [Bow] [Body] [Sch] |  Material       |  520px main
| Pressure |  +-----------------+|  Size           |
| Position |  |                 ||  Brightness     |
| Rosin    |  |  Placeholder    ||                 |
| Noise    |  |  (Phase 4.3)    ||  STRINGS        |
|          |  |                 ||  Count          |
| [Tier ▼] |  +-----------------+|  Tune 1-4*     |
|          | IMPOSSIBLE PHYSICS  |  Symp Amount    |
|          | Inf.  Rev.  Sub.    |  Symp Count     |
+----------+---------------------+-----------------+
|  Ref Pitch        Width         Level             |  40px footer
+--------------------------------------------------+
* Tune 1-4 conditionally visible based on stringCount
```

**CSS:**
- Ouaricon Naturalist palette (--bg-paper: #F5E6D3, --green-mid: #6B8E4E, --brown-text: #3C2F2F, etc.)
- Garamond/Georgia/serif typography
- 55px seed cross-section knobs (conic-gradient, 10-segment, O-Orbit pattern)
- Botanical illustration overlay (right side, opacity 0.12, pointer-events: none)
- Section borders with brown-frame color
- Conditional visibility for stringTuning1-4 (CSS class toggle)

**JS parameter binding (all 23):**
- Import getSliderState/getComboBoxState from JUCE bridge
- Seed knob: relative drag interaction (mousedown/mousemove/mouseup/dblclick)
- Rotation: -135deg to +135deg (270deg sweep) via CSS custom property
- valueChangedEvent listener for host automation -> UI sync
- Combo box binding for tuningSystem and frictionTier
- Value display with units (m/s, N, Hz, cents, dB, %)

**Conditional visibility:**
- stringTuning1-4: show/hide based on stringCount value (listen to relay)
- sympatheticAmount: show only when sympatheticCount > 0

**Parameters by section:**

| Section | Parameters | Control Type |
|---------|-----------|--------------|
| Bow (left) | bowSpeed, bowPressure, bowPosition, rosin, bowNoise | Seed knobs |
| Bow (left) | frictionTier | Dropdown (3 options) |
| Body (right) | bodyMaterial, bodySize, brightness | Seed knobs |
| Strings (right) | stringCount, stringTuning1-4, sympatheticAmount, sympatheticCount | Seed knobs (tuning knobs conditional) |
| Impossible (center bottom) | infiniteSustain, reversedFriction, subHarmonics | Seed knobs (horizontal row) |
| Output (footer) | width, outputLevel | Seed knobs |
| Tuning (header/footer) | referencePitch | Seed knob |
| Tuning (header) | tuningSystem | Dropdown |

### Task 3: Add botanical illustration resource

**Files:** `Resources/ui/img/botanical.png`, `CMakeLists.txt`, `Source/PluginEditor.cpp`
**Depends on:** Task 2

- Select botanical illustration from `Ouaricon Audio Images/` (horsehair/flax plant or wood cross-section theme)
- Copy to `Resources/ui/img/botanical.png`
- Add to `juce_add_binary_data` in CMakeLists.txt
- Add resource provider route for `/img/botanical.png` in PluginEditor.cpp getResource()

### Task 4: Build and verify Phase 4.1

**Files:** none (build only)
**Depends on:** Tasks 1-3

- CMake configure + ninja build (O-Bowed_VST3 O-Bowed_AU)
- Clear AU cache, install fresh
- Verify in Standalone:
  - All 23 knobs/dropdowns visible and styled
  - Seed knob drag changes parameter values
  - Host automation updates UI (test via APVTS)
  - Conditional visibility works (stringCount changes hide/show tuning knobs)
  - Naturalist aesthetic renders correctly (paper bg, green accents, serif type)
  - Botanical illustration positioned correctly
  - 900x600 layout fits without overflow

### Phase 4.1 Success Criteria

- [ ] WebView opens at 900x600 with Naturalist aesthetic
- [ ] All 23 parameters have visible controls
- [ ] Seed knob drag interaction works (relative drag, -135 to +135 deg)
- [ ] Two-way binding: UI -> host and host -> UI for all params
- [ ] frictionTier dropdown shows Core/Enhanced/Quality
- [ ] tuningSystem dropdown shows available options
- [ ] stringTuning1-4 conditionally visible based on stringCount
- [ ] sympatheticAmount visible only when sympatheticCount > 0
- [ ] Section grouping matches layout spec (Bow left, Body/Strings right, Viz center, Impossible below, Output footer)
- [ ] Botanical illustration rendered at low opacity on right side
- [ ] Value labels display with correct units
- [ ] No console errors in WebView

---

## Phase 4.2: Visualizations

**Goal:** Three canvas visualizations in tabbed center panel — bow-string animation, body resonance spectrum, Schelleng diagram

### Task 5: Add VisualizationState to processor + native function

**Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
**Depends on:** Phase 4.1 complete

- Add `VisualizationState` struct (bowSpeed, bowPressure, bowPosition, bodyPeaks[8], bodyGains[8], schellengX, schellengY, isPlaying, stringAmplitude)
- Atomic snapshot updated in processBlock
- Register `getNativeFunction("getVisualizationState")` returning JSON
- Message-thread safe (processor writes, editor reads)

### Task 6: Bow-string animation canvas (default tab)

**Files:** `Resources/ui/index.html` (JS section)
**Depends on:** Task 5

- Canvas 2D in center panel, tab 1 (default active)
- Horizontal string as bezier curve with wave displacement
- Bow contact point indicator at beta position
- Bow pressure shown as penetration depth
- Speed shown as arrow length
- requestAnimationFrame at 60fps, polls viz state at 15Hz
- DPR-aware canvas sizing (canvas.width = clientWidth * dpr)
- Animates when isPlaying=true, rests when false

### Task 7: Body resonance spectrum canvas (tab 2)

**Files:** `Resources/ui/index.html` (JS section)
**Depends on:** Task 5

- Log-frequency X-axis (20Hz-20kHz), dB Y-axis (-24 to +12)
- 8 resonance peaks as smooth curve (bezier through peak points)
- Grid lines at octave intervals
- Color-coded by material type (green=wood, blue=metal, amber=membrane, white=glass)
- Only redraws when bodyMaterial or bodySize parameter changes (not every frame)
- DPR-aware canvas

### Task 8: Schelleng diagram canvas (tab 3)

**Files:** `Resources/ui/index.html` (JS section)
**Depends on:** Task 5

- X-axis: bow position (0.02-0.30), Y-axis: bow pressure (log, 0.01-5.0)
- Colored Helmholtz region (playable zone) between min/max pressure curves
- Analytical boundaries: P_min proportional to v_B/(beta^2 * Z), P_max proportional to v_B/(beta * Z)
- Current playing point as crosshair/dot
- Reactive: updates from relay valueChangedEvents (bowPosition, bowPressure, bowSpeed)
- No polling needed — parameter-derived

### Task 9: Tab switching implementation

**Files:** `Resources/ui/index.html` (JS section)
**Depends on:** Tasks 6-8

- Three tabs: [Bow-String] [Body Spectrum] [Schelleng]
- Only animate/poll the active tab canvas
- O-Wind/O-Bells tab pattern (classList toggle)
- Pause requestAnimationFrame for hidden canvases

### Task 10: Build and verify Phase 4.2

**Depends on:** Tasks 5-9

- Build, install, verify:
  - Bow-string animation responds to MIDI input
  - Body spectrum shows peaks, updates with Material/Size changes
  - Schelleng diagram shows playable region and current position
  - Tab switching works, inactive canvases don't waste CPU
  - No performance regression (smooth 60fps animation)

### Phase 4.2 Success Criteria

- [ ] Bow-string animation shows bow contact, string vibration, responds to MIDI
- [ ] Body spectrum displays 8 resonance peaks with material color-coding
- [ ] Schelleng diagram shows Helmholtz region with current playing point
- [ ] Tab switching works, only active canvas animates
- [ ] Visualization polling at ~15Hz doesn't cause CPU spikes
- [ ] Canvas renders crisp on Retina (DPR-aware)
- [ ] All parameter binding from Phase 4.1 still works

---

## Phase 4.3: Preset Browser + Tuning Panel

**Goal:** Preset management and tuning file browser for production-ready UI

### Task 11: Add OuariconPresetManager module

**Files:** `CMakeLists.txt`, `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
**Depends on:** Phase 4.2 complete

- `/module-add O-Bowed preset-manager`
- Initialize in processor constructor: `presetManager.initialize("O-Bowed", parameters)`
- Register native functions: getPresetListWithCategories, loadPresetFromCategory, selectPreviousPreset, selectNextPreset, savePresetWithDialog, getCurrentPreset

### Task 12: Preset browser UI in header bar

**Files:** `Resources/ui/index.html`
**Depends on:** Task 11

- Header bar: ◀ ▶ nav buttons, center preset name display, save button
- Dropdown on click of preset name (category-organized list)
- getNativeFunction() calls for all preset operations
- Preset load updates all 23 UI controls (already handled by relay valueChangedEvents)

### Task 13: Tuning panel integration

**Files:** `Resources/ui/index.html`, `Source/PluginEditor.cpp`
**Depends on:** Task 11

- Initialize tuning panel JS/CSS (already served via resource provider)
- Tuning system dropdown triggers panel visibility
- Scala/TUN file browser via native function (native file dialog from C++)
- Register `getNativeFunction("loadTuningFile")` for file picker

### Task 14: Final build + full verification

**Depends on:** Tasks 11-13

- Full build, install, pluginval level 5
- Verify all 23 params, 3 visualizations, preset browser, tuning panel
- Test host automation round-trip
- Verify conditional visibility still works
- Performance check

### Phase 4.3 Success Criteria

- [ ] Preset browser loads factory presets by category
- [ ] ◀ ▶ navigation cycles through presets
- [ ] Save dialog creates user presets
- [ ] Tuning panel opens for Scala/TUN selection
- [ ] File browser loads .scl/.tun files
- [ ] All Phase 4.1 + 4.2 functionality preserved
- [ ] Pluginval level 5 passes

---

## Summary

| Phase | Tasks | Key Deliverables |
|-------|-------|-----------------|
| 4.1 | 1-4 | C++ relay fixes, complete Naturalist UI, all 23 params bound, botanical illustration |
| 4.2 | 5-10 | 3 canvas visualizations (bow-string, body spectrum, Schelleng), viz data bridge |
| 4.3 | 11-14 | Preset browser, tuning panel, final validation |

**Total:** 14 tasks across 3 phases

**Requirements addressed:**
- UI-01: All parameters accessible and controllable from GUI (Phase 4.1)
- UI-02: Visual feedback for bow state (Phase 4.2 -- bow-string animation + Schelleng diagram)
