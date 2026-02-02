# Stage 4: Polish - Execution Plan

**Plugin:** O-Bells
**Stage:** 4 - Polish (Final)
**Date:** 2026-02-02

---

## Goal

Complete O-Bells with a production-ready preset system featuring 25 factory presets across 5 categories, integrated preset bar UI, and full validation testing.

---

## Tasks

### Phase 1: Preset Manager Integration (C++)

#### Task 1: Copy OuariconPresetManager to Plugin
- **Files:** `Source/OuariconPresetManager.h` (create)
- **Depends on:** None
- **Description:** Copy the header-only preset manager module to the plugin source directory. No modifications needed - the module already supports `loadPresetFromFile()` which we'll use for category-based loading.

#### Task 2: Add PresetManager to Processor
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Depends on:** Task 1
- **Description:**
  - Add `#include "OuariconPresetManager.h"`
  - Add `OuariconPresetManager presetManager` member (after `parameters`)
  - Add `getPresetManager()` accessor
  - Initialize in constructor: `presetManager(parameters, "O-Bells")`
  - Add `initializeFactoryPresets()` private method
  - Call `initializeFactoryPresets()` in constructor (one-time setup)

#### Task 3: Update State Save/Load
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Task 2
- **Description:**
  - Update `getStateInformation()` to use `presetManager.getStateAsXml()`
  - Update `setStateInformation()` to use `presetManager.setStateFromXml()`
  - This preserves preset name across DAW sessions

### Phase 2: Factory Preset Definitions

#### Task 4: Create Factory Preset Initialization
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Task 2
- **Description:** Implement `initializeFactoryPresets()` method that:
  - Creates category subdirectories: `Factory/Orchestral/`, `Factory/Sacred/`, etc.
  - Writes 25 preset JSON files with researched parameter values
  - Only runs if Factory directory is empty (first-run detection)

**Preset Parameter Values (from RESEARCH.md):**

| Preset | Strike | Mallet | Damping | Bright | Material | Inharm | Ensemble |
|--------|--------|--------|---------|--------|----------|--------|----------|
| **Orchestral** |
| Tubular Bells | 0.4 | 0.6 | 0.8 | 0.55 | 0.1 | 0.45 | Solo |
| Concert Chimes | 0.5 | 0.7 | 0.85 | 0.65 | 0.15 | 0.5 | Solo |
| Glockenspiel | 0.6 | 0.8 | 0.7 | 0.75 | 0.4 | 0.35 | Solo |
| Celesta Mallet | 0.35 | 0.45 | 0.6 | 0.6 | 0.7 | 0.25 | Solo |
| Vibraphone | 0.45 | 0.5 | 0.75 | 0.5 | 0.25 | 0.3 | Warm |
| **Sacred** |
| Church Bell | 0.3 | 0.65 | 0.95 | 0.5 | 0.15 | 0.6 | Full |
| Cathedral Carillon | 0.35 | 0.55 | 0.9 | 0.45 | 0.1 | 0.55 | Full |
| Meditation Bowl | 0.25 | 0.3 | 0.85 | 0.4 | 0.2 | 0.3 | Solo |
| Temple Gong | 0.2 | 0.5 | 0.95 | 0.35 | 0.12 | 0.65 | Full |
| Singing Bowl | 0.3 | 0.25 | 0.9 | 0.5 | 0.3 | 0.25 | Warm |
| **World** |
| Gamelan Saron | 0.55 | 0.6 | 0.5 | 0.6 | 0.15 | 0.85 | Solo |
| Gamelan Bonang | 0.5 | 0.55 | 0.6 | 0.55 | 0.2 | 0.75 | Warm |
| Tibetan Bowl | 0.25 | 0.2 | 0.88 | 0.45 | 0.25 | 0.35 | Solo |
| Steel Pan | 0.6 | 0.65 | 0.65 | 0.7 | 0.35 | 0.4 | Warm |
| Kalimba Bell | 0.45 | 0.4 | 0.55 | 0.65 | 0.1 | 0.2 | Solo |
| **Ambient** |
| Frozen Shimmer | 0.6 | 0.7 | 1.0 | 0.8 | 0.85 | 0.4 | Lush |
| Bell Pad | 0.4 | 0.35 | 0.95 | 0.5 | 0.5 | 0.5 | Lush |
| Crystal Drone | 0.5 | 0.45 | 1.0 | 0.7 | 0.95 | 0.35 | Full |
| Ethereal Chime | 0.55 | 0.6 | 0.9 | 0.75 | 0.8 | 0.3 | Lush |
| Submerged Bells | 0.3 | 0.25 | 0.92 | 0.3 | 0.6 | 0.55 | Full |
| **Cinematic** |
| Epic Bell | 0.35 | 0.7 | 0.95 | 0.6 | 0.15 | 0.55 | Wide |
| Tension Chime | 0.7 | 0.85 | 0.6 | 0.85 | 0.45 | 0.7 | Wide |
| Horror Stinger | 0.8 | 0.95 | 0.4 | 0.9 | 0.5 | 0.8 | Solo |
| Dramatic Swell | 0.4 | 0.55 | 0.98 | 0.55 | 0.2 | 0.5 | Full |
| Distant Thunder | 0.2 | 0.4 | 1.0 | 0.25 | 0.1 | 0.65 | Full |

**Ensemble Mappings:**
- Solo: unison=0, detune=0, sub=0, oct=0, spread=0.5
- Warm: unison=0.33, detune=8/50, sub=0.2, oct=0, spread=0.6
- Lush: unison=0.67, detune=15/50, sub=0.3, oct=0.2, spread=0.8
- Full: unison=1.0, detune=20/50, sub=0.4, oct=0.35, spread=0.9
- Wide: unison=1.0, detune=12/50, sub=0.5, oct=0.3, spread=1.0

### Phase 3: Editor Native Functions

#### Task 5: Add FileChooser Member
- **Files:** `Source/PluginEditor.h`
- **Depends on:** None
- **Description:** Add `std::shared_ptr<juce::FileChooser> fileChooser` member for save/load dialogs.

#### Task 6: Add Preset Native Functions to Editor
- **Files:** `Source/PluginEditor.cpp`
- **Depends on:** Task 2, Task 5
- **Description:** Add native functions to WebBrowserComponent options:
  - `getPresetList` → Returns flat array of preset names
  - `getPresetListWithCategories` → Returns `{category: [presets...]}` object
  - `getCurrentPreset` → Returns current preset name
  - `loadPreset` → Loads flat preset by name
  - `loadPresetFromCategory` → Loads preset from category subfolder
  - `savePreset` → Saves user preset
  - `selectNextPreset` → Navigates and returns new name
  - `selectPreviousPreset` → Navigates and returns new name
  - `savePresetWithDialog` → Opens save dialog
  - `loadPresetFromFile` → Opens load dialog

### Phase 4: WebView Preset Bar UI

#### Task 7: Add Preset Bar CSS
- **Files:** `Resources/ui/index.html`
- **Depends on:** None
- **Description:** Add CSS styles for preset browser:
  - `.preset-browser` container (flexbox, right-aligned)
  - `.preset-arrow` navigation buttons
  - `.preset-name-wrapper` with dropdown trigger
  - `.preset-dropdown` with category headers
  - `.preset-dropdown-header` category styling
  - `.preset-dropdown-item` preset item styling
  - `.preset-action-btn` Save/Load buttons
  - Ouaricon Naturalist theme consistency

#### Task 8: Add Preset Bar HTML
- **Files:** `Resources/ui/index.html`
- **Depends on:** Task 7
- **Description:** Add preset bar to header section:
  ```html
  <div class="header-bar">
      <h1 class="plugin-title">O-Bells</h1>
      <div class="preset-browser">
          <div class="preset-arrow" id="preset-prev">◀</div>
          <div class="preset-name-wrapper">
              <div class="preset-name" id="preset-name-display">Default</div>
              <div class="preset-dropdown" id="preset-dropdown"></div>
          </div>
          <div class="preset-arrow" id="preset-next">▶</div>
          <div class="preset-action-btn" id="preset-save">Save</div>
          <div class="preset-action-btn" id="preset-load">Load</div>
      </div>
  </div>
  ```

#### Task 9: Add Preset Bar JavaScript
- **Files:** `Resources/ui/index.html`
- **Depends on:** Task 6, Task 8
- **Description:** Add JavaScript for preset functionality:
  - `showPresetDropdown()` - Populates categorized dropdown
  - `hidePresetDropdown()` - Closes dropdown
  - `updatePresetDisplay(name)` - Updates display text
  - Event listeners for prev/next arrows
  - Event listener for dropdown toggle
  - Event listeners for save/load buttons
  - Keyboard shortcuts (arrow keys when focused)
  - Close dropdown on outside click

### Phase 5: Build and Validation

#### Task 10: Build Plugin
- **Files:** None (build system)
- **Depends on:** Tasks 1-9
- **Description:**
  ```bash
  cd build && ninja O-Bells_VST3 O-Bells_AU
  ```

#### Task 11: Install and Clear Cache
- **Files:** System plugin directories
- **Depends on:** Task 10
- **Description:**
  ```bash
  killall -9 AudioComponentRegistrar 2>/dev/null || true
  rm -rf ~/Library/Caches/AudioUnitCache/
  rm -rf ~/Library/Caches/com.apple.audiounits.cache
  rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Bells.vst3
  rm -rf ~/Library/Audio/Plug-Ins/Components/O-Bells.component
  cp -R build/plugins/O-Bells/O-Bells_artefacts/Release/VST3/O-Bells.vst3 ~/Library/Audio/Plug-Ins/VST3/
  cp -R build/plugins/O-Bells/O-Bells_artefacts/Release/AU/O-Bells.component ~/Library/Audio/Plug-Ins/Components/
  ```

#### Task 12: Run pluginval
- **Files:** None (validation)
- **Depends on:** Task 11
- **Description:**
  ```bash
  /Applications/pluginval.app/Contents/MacOS/pluginval \
      --validate ~/Library/Audio/Plug-Ins/VST3/O-Bells.vst3 \
      --strictness-level 5 \
      --timeout-ms 60000
  ```

#### Task 13: Manual DAW Testing
- **Files:** None (manual testing)
- **Depends on:** Task 12
- **Description:**
  - [ ] Plugin loads in Logic Pro without blank WebView
  - [ ] Preset dropdown shows 5 categories with headers
  - [ ] All 25 factory presets load correctly
  - [ ] Prev/Next navigation works and wraps
  - [ ] Save creates new user preset
  - [ ] Load opens file chooser
  - [ ] Preset state saves with project
  - [ ] No audio glitches during preset changes
  - [ ] CPU usage <60% with full ensemble

---

## File Summary

### Files to Create
| File | Lines (est.) | Purpose |
|------|--------------|---------|
| `Source/OuariconPresetManager.h` | ~550 | Header-only preset manager (copy from module) |

### Files to Modify
| File | Changes |
|------|---------|
| `Source/PluginProcessor.h` | +5 lines: include, member, accessor |
| `Source/PluginProcessor.cpp` | +200 lines: presetManager init, factory presets, state methods |
| `Source/PluginEditor.h` | +1 line: fileChooser member |
| `Source/PluginEditor.cpp` | +150 lines: 10 native functions for presets |
| `Resources/ui/index.html` | +200 lines: preset bar HTML/CSS/JS |

### Factory Presets Created (at runtime)
```
~/Library/O-Bells/Presets/Factory/
├── Orchestral/  (5 presets)
├── Sacred/      (5 presets)
├── World/       (5 presets)
├── Ambient/     (5 presets)
└── Cinematic/   (5 presets)
```

---

## Success Criteria

### Automated Checks
- [ ] Build succeeds (VST3 + AU)
- [ ] AU validation passes (`auval -v aumu OBls OuDv`)
- [ ] pluginval level 5 passes
- [ ] All 25 factory presets exist in `~/Library/O-Bells/Presets/Factory/`

### Manual Verification
- [ ] Preset dropdown displays 5 categories with headers
- [ ] Each category contains 5 presets
- [ ] Loading preset audibly changes sound
- [ ] Prev/Next navigation wraps correctly
- [ ] User preset save/load works
- [ ] Preset name persists across DAW session save/load
- [ ] No blank WebView or UI glitches
- [ ] CPU < 60% with 8-voice chord + full ensemble

---

## Dependencies

```
Task 1 ──► Task 2 ──► Task 3
              │
              ▼
           Task 4
              │
Task 5 ───────┼──► Task 6 ──► Task 9
              │
Task 7 ──► Task 8 ──┘
              │
              ▼
          Task 10 ──► Task 11 ──► Task 12 ──► Task 13
```

**Critical Path:** 1 → 2 → 4 → 6 → 9 → 10 → 11 → 12 → 13

---

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Category scanning slow | Test with 25 presets - should be <5ms |
| Preset JSON format mismatch | Use exact format from OuariconPresetManager |
| Native function async issues | Use completion callbacks consistently |
| First-run factory creation fails | Check for empty directory, create recursively |

---

## Estimated Effort

- **Phase 1 (C++ Integration):** Tasks 1-3
- **Phase 2 (Factory Presets):** Task 4
- **Phase 3 (Native Functions):** Tasks 5-6
- **Phase 4 (WebView UI):** Tasks 7-9
- **Phase 5 (Validation):** Tasks 10-13

---

*Plan created: 2026-02-02*
