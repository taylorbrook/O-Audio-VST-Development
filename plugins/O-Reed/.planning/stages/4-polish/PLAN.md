# Stage 4: Polish - Execution Plan

**Date:** 2026-04-06
**Goal:** Ship-ready O-Reed v1.0.0 with 24 factory presets, pluginval level 10 validation, and changelog
**Tasks:** 9
**Estimated Complexity:** Low-Medium (integration work, no new DSP)

---

## Tasks

### 1. [ ] Add preset-manager module to CMakeLists.txt
- **Files:** `CMakeLists.txt`
- **Depends on:** none
- **Action:** Add `${CMAKE_SOURCE_DIR}/modules/persistence/preset-manager/cpp` to `target_include_directories`

### 2. [ ] Integrate OuariconPresetManager in PluginProcessor.h
- **Files:** `Source/PluginProcessor.h`
- **Depends on:** Task 1
- **Action:**
  - Add `#include "OuariconPresetManager.h"`
  - Add `OuariconPresetManager presetManager;` private member (after `parameters`)
  - Add `OuariconPresetManager& getPresetManager() { return presetManager; }` public accessor
  - Add `void initializeFactoryPresets();` private method declaration

### 3. [ ] Implement preset manager integration in PluginProcessor.cpp
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Task 2
- **Action:**
  - Add `presetManager(parameters, "O-Reed")` to constructor init list
  - Call `initializeFactoryPresets()` at end of constructor
  - Replace `getStateInformation()` with preset manager delegate (`presetManager.getStateAsXml()`)
  - Replace `setStateInformation()` with preset manager delegate, **preserving dronePitch v1→v2 migration** after `presetManager.setStateFromXml()`
  - Implement `initializeFactoryPresets()` with all 24 presets using `normalize()` lambda for non-0-1 parameters
  - Guard: skip if factory directory already populated (O-Bowed pattern)

### 4. [ ] Define all 24 factory preset parameter snapshots
- **Files:** `Source/PluginProcessor.cpp` (inside `initializeFactoryPresets()`)
- **Depends on:** Task 3
- **Action:** 24 `FactoryPresetDef` entries:
  - **Western (9):** Bb Clarinet, Bass Clarinet, Alto Sax, Tenor Sax, Soprano Sax, Baritone Sax, Oboe, English Horn, Bassoon
  - **Non-Western (9):** Duduk, Shehnai, Suona, Hichiriki, Zurna, Piri, Arghul, Launeddas, Mijwiz
  - **Sound Design (6):** Glass Reed, Metal Wind, Impossible Bore, Breath Drone, Giant Clarinet, Micro Reed
- **Critical:** Use `normalize(paramId, rawValue)` for: `toneHoleCutoff`, `vibratoRate`, `dronePitch`, `referencePitch`, `maxVoices`, `outputGain`, `instrumentPreset`, all Choice params. Raw values OK for linear 0-1 floats. `dualBore` bool = 0.0/1.0.

### 5. [ ] Add preset native functions to PluginEditor
- **Files:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Depends on:** Task 2
- **Action:**
  - Add `std::shared_ptr<juce::FileChooser> fileChooser;` member in PluginEditor.h
  - Add 7 `.withNativeFunction()` calls to WebView builder chain:
    - `getPresetList` → returns array of preset names
    - `getCurrentPreset` → returns current preset name
    - `loadPreset` → takes name, returns bool
    - `savePreset` → takes name, returns bool
    - `selectNextPreset` → navigates forward, returns new name
    - `selectPreviousPreset` → navigates backward, returns new name
    - `savePresetWithDialog` → launches FileChooser, returns saved name

### 6. [ ] Build and smoke test
- **Files:** none (build artifacts)
- **Depends on:** Tasks 1-5
- **Action:**
  - `ninja O-Reed_VST3 O-Reed_AU` — zero errors required
  - Cache clear + install to system folders
  - Verify presets load in Standalone: spot-check Bb Clarinet, Duduk, Arghul (dual bore), Glass Reed
  - Verify state save/restore round-trips (save, close, reopen — same preset)

### 7. [ ] Run pluginval level 10
- **Files:** none
- **Depends on:** Task 6
- **Action:**
  - VST3: `pluginval --strictness-level 10 --timeout-ms 120000 --validate ...O-Reed-dev.vst3`
  - AU: `pluginval --strictness-level 10 --timeout-ms 120000 --validate ...O-Reed-dev.component`
  - Fix any failures (likely none — already passes L10 from Stage 2, preset manager adds no audio-thread changes)
  - Run `auval -v aumu ORed OuAu` for AU validation

### 8. [ ] Write CHANGELOG.md
- **Files:** `plugins/O-Reed/CHANGELOG.md` (new)
- **Depends on:** Task 7 (wait for validation to confirm final feature set)
- **Action:** v1.0.0 changelog covering:
  - Physical modeling reed wind synthesis
  - 24 factory presets (9 Western + 9 Non-Western + 6 Sound Design)
  - 35 parameters, MPE, microtonal tuning
  - Impossible physics features
  - WebView UI
  - pluginval L10 pass

### 9. [ ] Final install and DAW verification
- **Files:** none
- **Depends on:** Task 8
- **Action:**
  - Cache clear + fresh install (VST3 + AU)
  - `auval -a | grep -i reed` — verify AU appears
  - Load in DAW, cycle through presets, verify no crashes
  - Confirm dual bore presets (Arghul, Launeddas, Mijwiz) produce drone

---

## Success Criteria

- [ ] OuariconPresetManager integrated, 24 factory presets written to `~/Library/O-Reed/Presets/Factory/`
- [ ] All 24 presets load correctly with appropriate timbral character
- [ ] State save/restore preserves preset selection and dronePitch migration
- [ ] pluginval level 10 PASS for both VST3 and AU
- [ ] auval PASS
- [ ] CHANGELOG.md written
- [ ] Plugin installed and verified in DAW

---

## Files Summary

| File | Action |
|------|--------|
| `CMakeLists.txt` | Add include path |
| `Source/PluginProcessor.h` | Add include, member, accessor, method |
| `Source/PluginProcessor.cpp` | Preset manager init, state delegation, 24 presets |
| `Source/PluginEditor.h` | Add fileChooser member |
| `Source/PluginEditor.cpp` | Add 7 preset native functions |
| `CHANGELOG.md` | New file — v1.0.0 |

---

## Risk Notes

- dronePitch migration MUST be preserved after `setStateFromXml()` — old saved states use semitones (-24..24), new uses cents (-2400..2400)
- Use `normalize()` lambda for all non-0-1 and Choice params — manual normalization math is error-prone with skewed ranges
- Factory preset guard prevents re-writing on every plugin instantiation
