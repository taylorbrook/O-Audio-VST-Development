# Stage 3: GUI - Execution Plan (Phase 3.1)

## Goal

Build the complete WebView UI for O-Wind: 3-tab naturalist interface with all 14 slider parameters bound, preset browser (OuariconPresetManager + 8 factory presets), instrument preset selector, tone hole toggle (WebToggleButtonRelay), tuning panel integration, and botanical fern overlay. Replace the existing shell index.html with a fully functional UI.

## Phase Scope

Phase 3.1 only — Layout + Controls + Parameter Binding. Phase 3.2 (breath visualization, register indicator, polish) is deferred.

---

## Tasks

### 1. [ ] Add toneHoleEnabled parameter + instrumentPreset parameter to APVTS
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Depends on:** none
- **Details:**
  - Add `AudioParameterBool("toneHoleEnabled", 1)` to `createParameterLayout()` — default false
  - Add `AudioParameterInt("instrumentPreset", 1)` range 0-7, default 0 — replaces the ad-hoc `std::atomic<int> currentPresetIndex` with APVTS-backed parameter so it survives preset save/load
  - Update `parameterChanged()` listener to handle `instrumentPreset` changes (update the atomic or apply directly)
  - Read `toneHoleEnabled` in `processBlock()` and pass to voices (or read per-voice from APVTS)
  - Remove the manual `instrumentPreset` property save/restore from `getStateInformation()`/`setStateInformation()` since APVTS handles it now

### 2. [ ] Integrate OuariconPresetManager into PluginProcessor
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`, `CMakeLists.txt`
- **Depends on:** Task 1 (parameters must exist for factory presets to reference them)
- **Details:**
  - Add `#include "OuariconPresetManager.h"` to processor header
  - Add member `OuariconPresetManager presetManager;` and accessor `getPresetManager()`
  - Initialize: `presetManager(parameters, "O-Wind")`
  - Define 8 factory presets with curated APVTS parameter values per instrument
  - Replace `getStateInformation()` / `setStateInformation()` to use `presetManager.getStateAsXml()` / `presetManager.setStateFromXml()`
  - Add include path for preset module in CMakeLists.txt: `${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp`
  - Add OuariconPresetManager.h to target sources (or just include path — header-only)

### 3. [ ] Add tone hole toggle relay + instrument preset relay to PluginEditor
- **Files:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Depends on:** Task 1
- **Details:**
  - Add `WebToggleButtonRelay` + `WebToggleButtonParameterAttachment` for `toneHoleEnabled`
  - Add `WebSliderRelay` + `WebSliderParameterAttachment` for `instrumentPreset` (int param, 0-7 range — slider relay works for int/choice params via normalised value)
  - OR use `WebComboBoxRelay` + `WebComboBoxParameterAttachment` for `instrumentPreset` (cleaner for discrete choices) — check JUCE 8 API availability
  - Register both with `.withOptionsFrom()` on WebBrowserComponent options chain
  - Maintain correct member declaration order: relays → WebView → attachments

### 4. [ ] Register preset module native functions on PluginEditor
- **Files:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Depends on:** Task 2, Task 3
- **Details:**
  - Add native functions to WebBrowserComponent options chain:
    - `getPresetList` — returns array of preset names
    - `getCurrentPreset` — returns current preset name
    - `loadPreset` — loads by name
    - `savePreset` — saves by name
    - `selectNextPreset` — navigate forward
    - `selectPreviousPreset` — navigate backward
  - Add `std::shared_ptr<juce::FileChooser> fileChooser;` member for async preset save/load dialogs
  - Add instrument preset native functions:
    - `getInstrumentPresets` — returns array of 8 instrument names
    - `getInstrumentPreset` — returns current index
    - `setInstrumentPreset` — sets by index

### 5. [ ] Copy fern botanical image to Resources + update CMakeLists.txt binary resources
- **Files:** `Resources/ui/img/fern_naturalistsmisc1Geor_0089.png` (new), `CMakeLists.txt`
- **Depends on:** none
- **Details:**
  - Copy from `/Users/taylorbrook/Dev/Ouaricon Audio Images/flora/fern_naturalistsmisc1Geor_0089.png`
  - Create `Resources/ui/img/` directory
  - Add to `juce_add_binary_data` sources in CMakeLists.txt
  - Add `/img/fern.png` route in resource provider (PluginEditor.cpp `getResource()`)

### 6. [ ] Update resource provider with new routes
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** Task 5
- **Details:**
  - Add `/img/fern.png` → BinaryData fern image, `image/png` MIME type
  - Verify exact BinaryData identifier name after CMake configure (may need to check generated BinaryData.h)

### 7. [ ] Build full index.html — 3-tab naturalist UI with all controls
- **Files:** `Resources/ui/index.html` (replace existing shell)
- **Depends on:** none (can be developed in parallel, but must match relay/native function names)
- **Details:**
  - Single file: inline `<style>`, `<body>` structure, `<script type="module">`
  - **Layout:**
    - Preset browser bar (top, ~40px): prev/next arrows, preset name display, dropdown, save button
    - Tab bar (~35px): SOUND | TUNING | EFFECTS
    - Tab content area (~525px)
  - **Sound tab:** Signal-flow parameter groups:
    - Row 1: Excitation (3 knobs), Resonator (4 knobs), Expression (2 knobs), Output (2 knobs)
    - Row 2: Impossible Physics (3 knobs), Tone Hole toggle, Instrument Preset selector
  - **Tuning tab:** `#tuning-container` div for tuning-panel.js module
  - **Effects tab:** "Coming Soon" placeholder
  - **CSS:** Ouaricon Naturalist variables (#F5E6D3 paper, #3C2F2F text, #8BA870 green, Garamond serif, 2px walnut borders)
  - **Botanical overlay:** Fern image, right side, ~70% height, 0.35 opacity, pointer-events none
  - **JS bindings:**
    - 14 slider params via `Juce.getSliderState(paramName)` — drag to set, listen for automation
    - Tone hole toggle via `Juce.getToggleState("toneHoleToggle")` — click to toggle, listen for changes
    - Instrument preset selector via `Juce.getNativeFunction("getInstrumentPresets")` etc.
    - Preset browser via `Juce.getNativeFunction("getPresetList")` etc.
    - Tuning panel via dynamic import of `/js/tuning-panel.js`
  - **Dimensions:** 900x600 fixed pixel, no viewport units
  - **Estimated:** ~1200-1500 lines

### 8. [ ] Build and verify compilation
- **Files:** none (build step)
- **Depends on:** Tasks 1-7
- **Details:**
  - `cmake --preset default` to regenerate (pick up new binary resources)
  - `ninja O-Wind_VST3 O-Wind_AU`
  - Fix any compilation errors (BinaryData identifier names, missing includes)
  - Verify resource provider serves all routes correctly

### 9. [ ] Install and validate in DAW
- **Files:** none (install + test step)
- **Depends on:** Task 8
- **Details:**
  - Clear AU cache, remove old binaries, install fresh
  - `auval -a | grep -i wind` — verify AU registration
  - Open in DAW (Standalone first, then Ableton/Logic)
  - Verify: all 14 sliders respond to drag and reflect automation
  - Verify: tone hole toggle works and persists state
  - Verify: instrument preset selector loads all 8 instruments
  - Verify: preset browser save/load/navigate works
  - Verify: tuning tab initializes tuning panel module
  - Verify: botanical fern image visible at correct opacity
  - Verify: Effects tab shows placeholder
  - Verify: no console errors in WebView

---

## Files Summary

### New Files
| File | Task | Purpose |
|------|------|---------|
| `Resources/ui/img/fern_naturalistsmisc1Geor_0089.png` | 5 | Botanical overlay image |

### Modified Files
| File | Tasks | Changes |
|------|-------|---------|
| `Source/PluginProcessor.h` | 1, 2 | Add toneHoleEnabled + instrumentPreset params, OuariconPresetManager member |
| `Source/PluginProcessor.cpp` | 1, 2 | Add params to layout, preset manager init, factory presets, update state save/load |
| `Source/PluginEditor.h` | 3, 4 | Add toggle relay/attachment, instrument preset relay, FileChooser, preset native functions |
| `Source/PluginEditor.cpp` | 3, 4, 6 | Add relays, native functions, resource provider routes |
| `CMakeLists.txt` | 2, 5 | Add fern image to binary resources, add preset module include path |
| `Resources/ui/index.html` | 7 | Replace shell with full 3-tab naturalist UI |

### Replaced Files
| File | Task | Notes |
|------|------|-------|
| `Resources/ui/index.html` | 7 | Existing shell replaced entirely |

---

## Success Criteria

- [ ] All 14 slider parameters respond to mouse drag and reflect DAW automation
- [ ] Tone hole toggle (WebToggleButtonRelay) toggles and persists via APVTS
- [ ] Instrument preset selector loads all 8 instrument presets
- [ ] Preset browser: save, load, navigate (prev/next) all functional
- [ ] 3 tabs switch correctly (Sound, Tuning, Effects)
- [ ] Tuning panel initializes on Tuning tab with A4 and system selector
- [ ] Botanical fern overlay visible at 0.35 opacity, right side
- [ ] Effects tab shows "Coming Soon" placeholder
- [ ] 900x600 window renders correctly with Ouaricon Naturalist aesthetic
- [ ] Builds clean: `ninja O-Wind_VST3 O-Wind_AU` zero errors
- [ ] AU validates: `auval -a | grep -i wind` passes
- [ ] No WebView console errors at startup or during interaction

---

## Dependency Graph

```
Task 1 (APVTS params) ──┬──> Task 2 (preset manager) ──┐
                         ├──> Task 3 (relays)            ├──> Task 4 (native functions) ──┐
Task 5 (fern image) ─────┴──> Task 6 (resource routes)  │                                 │
Task 7 (index.html) ──────────────────────────────────────┴─────────────────────────────> Task 8 (build)
                                                                                            │
                                                                                         Task 9 (validate)
```

**Parallelizable:** Tasks 1 + 5 + 7 can start simultaneously. Tasks 2 + 3 after Task 1. Tasks 4 + 6 after their dependencies.

---

## Risk Notes

- **BinaryData identifier for fern image:** Name depends on JUCE's filename-to-identifier conversion. Check generated `BinaryData.h` after cmake configure if build fails.
- **instrumentPreset as APVTS param:** Changing from ad-hoc atomic to APVTS breaks existing DAW session state for anyone who had O-Wind loaded. Acceptable since Stage 2 was just verified — no users have sessions yet.
- **Preset dropdown z-index:** Must be elevated above tab content. Use `z-index: 9999` on dropdown + parent stacking context elevation.
