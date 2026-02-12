# Stage 3: GUI - Execution Plan

> **Plugin:** O-Orbit
> **Stage:** 3-gui (3 sub-phases)
> **Date:** 2026-02-11
> **Inputs:** CONTEXT.md, RESEARCH.md, ROADMAP.md

---

## Goal

Build a WebView-based GUI for O-Orbit with all 17 parameter controls styled in the Ouaricon Botanical/Naturalist aesthetic, a real-time animated orbital visualizer, and an interactive speaker layout editor with file I/O.

---

## Phase 3.1: WebView UI + Parameter Controls

### Goal
WebView-based UI with all 17 parameter controls bound to APVTS, naturalist aesthetic applied, no visualizer yet (placeholder area).

### Tasks

1. [ ] **Update CMakeLists.txt with BinaryData target**
   - Add `juce_add_binary_data(OuariconOrbit_WebUI ...)` for all UI resource files
   - Link `OuariconOrbit_WebUI` to `OuariconOrbit` target
   - Files: `CMakeLists.txt`
   - Depends on: none

2. [ ] **Copy JUCE bridge JS files**
   - Copy `js/juce/index.js` and `js/juce/check_native_interop.js` from O-GrainScatter
   - These provide `getSliderState`, `getComboBoxState`, `getToggleState`, `getNativeFunction`
   - Files: `Resources/ui/js/juce/index.js`, `Resources/ui/js/juce/check_native_interop.js`
   - Depends on: none

3. [ ] **Create index.html with full layout structure**
   - 800x600 layout: header (30px), visualizer placeholder (320px), controls (230px), footer (20px)
   - Three parameter group sections: Motion / Spatial / Source
   - Knob containers, dropdown containers, toggle containers with labels
   - Script imports: `type="module"` for app.js, standard for JUCE bridge
   - Canvas element for visualizer area (placeholder gray)
   - Files: `Resources/ui/index.html`
   - Depends on: Task 2

4. [ ] **Create styles.css with Ouaricon Naturalist aesthetic**
   - Background: aged paper tone (#F5E6D3) with CSS paper grain texture
   - Typography: Garamond serif, warm dark brown (#3C2F2F), uppercase labels with letter-spacing
   - Botanical seed knobs: 55px diameter, 10-segment conic gradient, brown borders, green indicator
   - Dropdown styling: warm brown borders, paper background, Garamond text
   - Toggle button styling: muted green (#8BA870) active state
   - Section borders: warm brown (#8B7355), 2-3px solid
   - Shadows: soft organic (2px 2px 6px rgba(0,0,0,0.25))
   - Shell botanical overlay: positioned right side, 71.25% height, 0.35 opacity, pointer-events: none
   - Visualizer area: slightly darker plate background
   - Files: `Resources/ui/css/styles.css`
   - Depends on: none

5. [ ] **Create app.js with parameter binding**
   - Import JUCE bridge: `getSliderState`, `getComboBoxState`, `getToggleState`
   - Create rotary knob component (relative drag, double-click reset, sensitivity mapping)
   - Bind 11 slider relays: speed, width, depth, tilt, phase, elevation_range, distance, air_absorption, center_diverge, lr_offset, mix
   - Bind 5 combo box relays: path, tempo_sync, speaker_layout, attenuation_curve, source_mode
   - Bind 1 toggle relay: elevation_enable
   - Value display below each knob (formatted with units)
   - Files: `Resources/ui/js/app.js`
   - Depends on: Tasks 2, 3

6. [ ] **Rewrite PluginEditor.h with WebView + relay declarations**
   - Follow O-GrainScatter member declaration order: Relays -> WebView -> Attachments
   - 11 `WebSliderRelay` unique_ptrs (speed, width, depth, tilt, phase, elevation_range, distance, air_absorption, center_diverge, lr_offset, mix)
   - 5 `WebComboBoxRelay` unique_ptrs (path, tempo_sync, speaker_layout, attenuation_curve, source_mode)
   - 1 `WebToggleButtonRelay` unique_ptr (elevation_enable)
   - 1 `WebBrowserComponent` unique_ptr
   - 17 matching attachment unique_ptrs
   - Inherit from `juce::Timer` for later motion state push
   - Declare `getResource()`, `parentHierarchyChanged()`
   - Files: `Source/PluginEditor.h`
   - Depends on: none

7. [ ] **Rewrite PluginEditor.cpp with WebView setup**
   - Constructor: create all 17 relays, build WebView options with `.withOptionsFrom()` for each relay
   - Register resource provider via `.withResourceProvider()`
   - Lazy navigation via `parentHierarchyChanged()` pattern
   - Create WebBrowserComponent with options
   - Create all 17 parameter attachments (3-arg constructor: parameter, relay, nullptr)
   - `addAndMakeVisible(webView)`
   - `setSize(800, 600)`
   - `getResource()`: explicit URL-to-BinaryData mapping for each resource file
   - `resized()`: webView fills full bounds
   - Files: `Source/PluginEditor.cpp`
   - Depends on: Task 6

8. [ ] **Add botanical shell image and paper texture**
   - Source or generate a shell/nautilus SVG or PNG for botanical overlay
   - Paper grain texture image or CSS-only texture
   - Files: `Resources/ui/img/shell.png` (or SVG), optionally `Resources/ui/img/paper-bg.png`
   - Depends on: none

9. [ ] **Build and verify Phase 3.1**
   - `cmake --build build --target OuariconOrbit_VST3 OuariconOrbit_AU`
   - Verify standalone launches with WebView UI
   - All 17 parameter controls visible and interactive
   - Knob drag changes parameter values
   - Host automation reflected in UI
   - Naturalist aesthetic renders correctly
   - Files: none (build/test only)
   - Depends on: Tasks 1-8

### Phase 3.1 Success Criteria
- [ ] All 17 parameter controls visible in WebView UI
- [ ] Knobs respond to mouse drag (relative, not absolute)
- [ ] Dropdowns show correct options and change parameter values
- [ ] Elevation toggle works
- [ ] Parameter changes audible (verify via ear)
- [ ] Host automation reflected in UI knob positions
- [ ] Naturalist aesthetic applied (paper background, seed knobs, Garamond typography)
- [ ] Shell botanical overlay visible at ~35% opacity
- [ ] Standalone launches without crash

---

## Phase 3.2: Orbital Visualizer (Animated)

### Goal
Real-time animated orbital path visualization showing source position, path trails, and speaker positions, driven by motion state from the audio processor.

### Tasks

10. [ ] **Add atomic motion snapshot to PluginProcessor**
    - Add individual `std::atomic<float>` members for UI: `uiAzimuthL`, `uiElevationL`, `uiAzimuthR`, `uiElevationR`, `uiDistance`
    - Update at end of `processBlock()` with current motion state
    - Add public getter methods: `getUIAzimuthL()`, etc.
    - Files: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
    - Depends on: Phase 3.1 complete

11. [ ] **Add Timer-based motion state push to editor**
    - Start `startTimerHz(30)` in constructor
    - `timerCallback()`: read atomic motion values, format as JS call
    - `evaluateJavascript("if(window.updateMotion) window.updateMotion(azL, elL, azR, elR, dist, isLRSplit)")`
    - Files: `Source/PluginEditor.cpp`
    - Depends on: Task 10

12. [ ] **Add native function for speaker positions**
    - `getSpeakerLayout` native function: serialize current speaker layout as JSON array
    - Returns array of {azimuth, elevation, distance, label, isLFE} per speaker
    - Register in WebView options with `.withNativeFunction()`
    - Files: `Source/PluginEditor.cpp`
    - Depends on: Phase 3.1 complete

13. [ ] **Implement Canvas orbital visualizer in JS**
    - Canvas 2D rendering in the visualizer area
    - High-DPI handling (devicePixelRatio scaling)
    - `window.updateMotion()` callback stores latest state
    - 60fps `requestAnimationFrame` render loop
    - Coordinate mapping: azimuth 0=top, counter-clockwise, distance maps to radius
    - Draw speaker icons (cream/brown circles with labels) around perimeter
    - Draw source dot (muted green #8BA870, radial gradient glow, 8-10px)
    - Draw path trail: circular buffer of last 120 positions, warm brown/amber with decreasing opacity
    - L+R split mode: two dots (green for L, amber for R) with separate trails
    - Visualizer background: slightly darker plate area
    - Files: `Resources/ui/js/app.js` (or separate `visualizer.js`)
    - Depends on: Tasks 11, 12

14. [ ] **Build and verify Phase 3.2**
    - Build and launch standalone
    - Source dot moves smoothly along orbital path in real-time
    - Path trail follows source with fade
    - Speaker icons match current layout preset
    - L+R split: two dots with correct phase offset
    - Switching path type (Orbit/Pendulum/Linear/Drift) changes visible motion
    - Tempo sync visual speed matches audio
    - Animation smooth at 60fps
    - Files: none (build/test only)
    - Depends on: Tasks 10-13

### Phase 3.2 Success Criteria
- [ ] Source dot moves in real-time matching audio motion
- [ ] Path trails render with warm brown/amber fade
- [ ] Speaker positions display correctly for each preset layout
- [ ] L+R split mode shows two dots (green + amber)
- [ ] Animation is smooth (60fps, no jank)
- [ ] Path shape visually changes when Path parameter changes
- [ ] Tempo sync: visual speed matches audible movement

---

## Phase 3.3: Speaker Layout Editor + File I/O

### Goal
Interactive speaker layout editor (toggle view with visualizer), drag-to-reposition, add/remove speakers, preset buttons, save/load/import/export layouts, downmix warning badge.

### Tasks

15. [ ] **Add native functions for speaker layout modification**
    - `addSpeaker(azimuth, elevation, distance, label)` — adds speaker, triggers VBAP recompute
    - `removeSpeaker(index)` — removes speaker by index, triggers recompute
    - `moveSpeaker(index, azimuth, elevation)` — repositions speaker, triggers recompute
    - `setCustomLayout(speakersJSON)` — replace entire layout from JS
    - Need to add custom layout storage to processor (not just preset index)
    - Register all in WebView options
    - Files: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `Source/PluginEditor.cpp`
    - Depends on: Phase 3.2 complete

16. [ ] **Add native functions for file I/O**
    - `saveSpeakerLayout(name)` — save current layout to user presets directory
    - `loadSpeakerLayout()` — FileChooser to load layout file, return layout JSON
    - `exportLayout()` — FileChooser save mode, write current layout as .json
    - `importLayout()` — FileChooser open mode, read .json, apply layout
    - Use `std::shared_ptr<juce::FileChooser>` member (async requirement)
    - Use `launchAsync()` for all file dialogs
    - Files: `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
    - Depends on: Task 15

17. [ ] **Add downmix status native function**
    - `getDownmixStatus()` — returns {active, sourceChannels, targetChannels}
    - Called periodically by JS to show/hide downmix badge
    - Files: `Source/PluginEditor.cpp`
    - Depends on: Phase 3.2 complete

18. [ ] **Implement toggle view (Motion ↔ Speaker Editor)**
    - Toggle button in header: "Motion View" / "Speaker Editor"
    - Show/hide appropriate canvas content and editor controls
    - Speaker editor: top-down polar view of speaker positions
    - Preset buttons row: Stereo, Quad, 5.1, 7.1, 5.1.4, 7.1.4, Hex, Oct
    - Files: `Resources/ui/js/app.js`, `Resources/ui/index.html`, `Resources/ui/css/styles.css`
    - Depends on: Phase 3.2 complete

19. [ ] **Implement speaker editor canvas interactions**
    - Drag-to-reposition: mousedown finds nearest speaker, mousemove updates position, mouseup commits via `moveSpeaker()` native call
    - Click-to-add: click empty area adds speaker at that azimuth/elevation via `addSpeaker()`
    - Right-click-to-remove: contextmenu on speaker calls `removeSpeaker()`
    - Coordinate mapping: canvas pixel ↔ polar (azimuth, distance)
    - Visual feedback: highlight speaker on hover, drag ghost
    - Text inputs for precise az/el/dist values (optional toolbar below canvas)
    - Files: `Resources/ui/js/app.js`
    - Depends on: Tasks 15, 18

20. [ ] **Implement save/load/import/export UI buttons**
    - Save button: prompts for name, calls `saveSpeakerLayout(name)`
    - Load button: calls `loadSpeakerLayout()`, updates editor view on response
    - Export button: calls `exportLayout()`, shows success/cancel feedback
    - Import button: calls `importLayout()`, updates editor view on response
    - Styled in botanical aesthetic
    - Files: `Resources/ui/js/app.js`, `Resources/ui/index.html`, `Resources/ui/css/styles.css`
    - Depends on: Tasks 16, 18

21. [ ] **Implement downmix warning badge**
    - Small badge near speaker layout dropdown
    - Shows "Layout: 7.1 → DAW: Stereo" when downmix is active
    - Earth-tone styling, unobtrusive
    - Polls `getDownmixStatus()` every 2 seconds (not high frequency)
    - Hidden when downmix is not active
    - Files: `Resources/ui/js/app.js`, `Resources/ui/css/styles.css`
    - Depends on: Task 17

22. [ ] **Add custom layout persistence to state**
    - Custom speaker positions need to persist in `getStateInformation()` / `setStateInformation()`
    - Serialize custom layout as XML child element in state ValueTree
    - Restore on `setStateInformation()` and trigger VBAP recompute
    - Files: `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
    - Depends on: Task 15

23. [ ] **Build and verify Phase 3.3**
    - Build and launch standalone
    - Toggle between motion view and speaker editor
    - Drag speakers to reposition, verify VBAP updates
    - Add/remove speakers, verify audio routing changes
    - Preset buttons load correct layouts
    - Save/load/import/export speaker layouts
    - Downmix badge appears in stereo DAW with 7.1 layout
    - Files: none (build/test only)
    - Depends on: Tasks 15-22

### Phase 3.3 Success Criteria
- [ ] Toggle view switches cleanly between motion and speaker editor
- [ ] Drag-to-reposition updates speaker position and VBAP recalculates
- [ ] Click-to-add creates new speaker at correct position
- [ ] Right-click-to-remove deletes speaker
- [ ] Preset buttons load correct speaker configurations
- [ ] Export layout saves .json file, import restores it
- [ ] Save/load custom layouts work (user presets)
- [ ] Downmix badge shows when DAW channels < layout channels
- [ ] Custom layout persists across plugin close/reopen (state save/restore)

---

## Overall Stage 3 Success Criteria

- [ ] All 17 parameters controllable via WebView UI with botanical aesthetic
- [ ] Real-time orbital visualizer with 60fps animation
- [ ] Source dot, path trails, speaker icons all render correctly
- [ ] L+R split mode shows dual source dots
- [ ] Speaker layout editor with drag-to-reposition, add/remove
- [ ] Speaker layout file I/O (save/load/import/export)
- [ ] Downmix warning badge functional
- [ ] Custom speaker layout persists in plugin state
- [ ] Standalone, VST3, and AU all build and launch
- [ ] No audio glitches introduced by UI changes

---

## Deferred Requirements Addressed

| Requirement | Status | Phase |
|-------------|--------|-------|
| FR-3.2: Custom speaker layout editor | Planned | 3.3 (Tasks 18-19) |
| FR-3.4: Save/load custom layouts | Planned | 3.3 (Task 20) |
| FR-3.5: Import/export layout files | Planned | 3.3 (Task 20) |
| FR-6.4: Visual downmix warning | Planned | 3.3 (Task 21) |

---

## Key Reference Files

| Reference | Path |
|-----------|------|
| O-GrainScatter editor (30 params) | `plugins/O-GrainScatter/Source/PluginEditor.h` |
| Aesthetic template | `.claude/aesthetics/ouaricon-naturalist-001/aesthetic.md` |
| JUCE bridge JS | `plugins/O-Bells/Resources/ui/js/juce/index.js` |
| Rotary knob reference | `plugins/O-SpectralShaper/Resources/ui/js/components/RotaryKnob.js` |
| Lazy navigation template | `.claude/templates/code-snippets/webview/lazy-navigation.yaml` |
| Resource provider template | `.claude/templates/code-snippets/webview/resource-provider.yaml` |

---

**End of Plan — 23 tasks across 3 sub-phases**
