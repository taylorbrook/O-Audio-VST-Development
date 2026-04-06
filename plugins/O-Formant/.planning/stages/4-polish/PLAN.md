# Stage 4 Phase 4.1: DSP Completion + Presets — Execution Plan

**Date:** 2026-04-05
**Goal:** Wire missing outputGain/stereoWidth DSP, integrate OuariconPresetManager with 16 factory presets, add preset browser WebView UI

---

## Tasks

### 1. [ ] Wire outputGain in PluginProcessor
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Depends on:** none
- **Details:**
  - Add `juce::SmoothedValue<float> outputGainSmoothed { 1.0f };` member to PluginProcessor.h
  - In `prepareToPlay()`: `outputGainSmoothed.reset(sampleRate, 0.050);` (50ms ramp)
  - In `processBlock()` after `synthesiser.renderNextBlock()`:
    - Read `outputGain` param via `parameters.getRawParameterValue("outputGain")->load()`
    - Convert dB to linear: `juce::Decibels::decibelsToGain(dB)`
    - Per-sample loop: `outputGainSmoothed.setTargetValue(target)`, multiply all channels
  - Reference: O-Bass `PluginProcessor.cpp:252-284`

### 2. [ ] Wire stereoWidth in FormantVoice
- **Files:** `Source/FormantVoice.cpp`
- **Depends on:** none
- **Details:**
  - Read `pStereoWidth->load()` at block-rate (inside existing `if (sampleCounter % kCoeffUpdateInterval == 0)` block)
  - Compute per-voice pan from MIDI note: `panPosition = (noteNorm - 0.5f) * stereoWidth * 2.0f` where `noteNorm = currentlyPlayingNote.initialNote / 127.0f`
  - Equal-power gains: `panLGain = cos(panNorm * halfPi)`, `panRGain = sin(panNorm * halfPi)` where `panNorm = (panPosition + 1) * 0.5` clamped [0, 1]
  - Replace mono write at line 270-273:
    ```cpp
    outL[i] += sample * panLGain;
    if (outR != nullptr)
        outR[i] += sample * panRGain;
    ```
  - When stereoWidth=0, panPosition=0 → equal L/R (mono center) — correct
  - Reference: O-Prism `PrismVoice.cpp:487-505`

### 3. [ ] Copy OuariconPresetManager.h from O-Bells
- **Files:** `Source/OuariconPresetManager.h` (new, copy from O-Bells)
- **Depends on:** none
- **Details:**
  - Copy `plugins/O-Bells/Source/OuariconPresetManager.h` → `plugins/O-Formant/Source/OuariconPresetManager.h`
  - 524-line single-header with inline implementation
  - Has category support (`getPresetListWithCategories()`, `loadPresetFromCategory()`)
  - Has factory preset infrastructure (`initializeFactoryPresets()`, `FactoryPresetDef`)
  - Has state persistence (`getStateAsXml()`, `setStateFromXml()`)
  - No modifications needed — module is plugin-agnostic

### 4. [ ] Integrate PresetManager in PluginProcessor
- **Files:** `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp`
- **Depends on:** Task 3
- **Details:**
  - **Header:** Add `#include "OuariconPresetManager.h"`, member `OuariconPresetManager presetManager;`, accessor `OuariconPresetManager& getPresetManager() { return presetManager; }`
  - **Constructor init list:** `presetManager(parameters, "O-Formant")` — AFTER `parameters` in init order
  - **Constructor body:** Define 16 factory presets as `std::vector<OuariconPresetManager::FactoryPresetDef>` and call `presetManager.initializeFactoryPresets(factoryPresets)`
  - **getStateInformation:** Merge APVTS state with `presetManager.getStateAsXml()` into combined XML
  - **setStateInformation:** Restore both APVTS and preset manager state from combined XML
  - **getNumPrograms:** Return `presetManager.getPresetList().size()` (or keep 1 if DAW program change not desired)
  - Factory preset definitions use RAW parameter values (manager normalizes internally)

### 5. [ ] Define 16 factory preset parameter maps
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Task 4
- **Details:**
  - 4 categories × 4 presets from CONTEXT.md preset definitions
  - Each preset: `{"Category", "Name", {{"paramId", rawValue}, ...}, juce::var()}`
  - All 21 parameters per preset — use defaults for unspecified params:
    - vowelX=0.5, vowelY=0.5, vowelFocus=2.5, glottalRd=1.0, breathiness=0.1
    - vibratoRate=5.5, vibratoDepth=15.0, vibratoDelay=300.0
    - consonantLevel=0.3, consonantTone=0.5, sibilance=0.0, autoConsonant=0.0
    - attack=0.01, decay=0.3, sustain=0.8, release=0.5
    - formantShift=0.0, formantSpread=1.0, pitchGlide=0.0
    - outputGain=0.0, stereoWidth=0.5
  - Cinematic: Creature Growl, Alien Whisper, Sci-Fi Choir, Spectral Voice
  - Electronic: Formant Bass, Vowel Pad, Glitch Vocal, Robotic Speech
  - Ambient: Ethereal Drone, Breath Texture, Overtone Chant, Wind Voice
  - Speech: Natural Tenor, Breathy Soprano, Pressed Baritone, Child Voice
  - Values from CONTEXT.md tables

### 6. [ ] Add native functions to PluginEditor for preset communication
- **Files:** `Source/PluginEditor.h`, `Source/PluginEditor.cpp`
- **Depends on:** Task 4
- **Details:**
  - Add to WebBrowserComponent options chain (before `.withOptionsFrom()` calls):
    1. `getPresetList` → flat array of all preset names
    2. `getPresetListWithCategories` → `{category: [names]}` object
    3. `getCurrentPreset` → string
    4. `loadPreset(name)` → bool
    5. `loadPresetFromCategory(category, name)` → bool
    6. `savePreset(name)` → bool
    7. `selectNextPreset` → string (new preset name)
    8. `selectPreviousPreset` → string (new preset name)
    9. `deletePreset(name)` → bool
    10. `isFactoryPreset(name)` → bool
  - Pattern: O-Bells `PluginEditor.cpp:156-219` — exact same API shape
  - Add `std::unique_ptr<juce::FileChooser> fileChooser;` member if save dialog needed

### 7. [ ] Add preset browser HTML/CSS to index.html
- **Files:** `Source/ui/public/index.html`
- **Depends on:** none (parallel with C++ tasks)
- **Details:**
  - Preset bar in header area: prev arrow `‹` | preset name | next arrow `›`
  - Category dropdown below or integrated
  - Save button (opens name input)
  - Naturalist aesthetic: Garamond font, moss-green `#6B8E4E` arrows, `#F5E6D3` paper bg
  - Preset name centered, truncated with ellipsis if long
  - Layout: fixed-height preset bar above XY pad area

### 8. [ ] Add preset browser JavaScript to main.js
- **Files:** `Source/ui/public/js/main.js`
- **Depends on:** Tasks 6, 7
- **Details:**
  - Get native functions: `window.__JUCE__.backend.getNativeFunction("getPresetList")` etc.
  - On page load: `getCurrentPreset()` to display current name
  - Prev/Next click handlers: call `selectPreviousPreset()`/`selectNextPreset()`, update display
  - Category dropdown: call `getPresetListWithCategories()`, populate dropdown, on select call `loadPresetFromCategory()`
  - Save button: prompt for name, call `savePreset(name)`
  - After any preset load: update all relay controls (they auto-sync via APVTS parameter changes)
  - Inline in existing main.js — no separate module

### 9. [ ] Update state persistence for preset manager
- **Files:** `Source/PluginProcessor.cpp`
- **Depends on:** Task 4
- **Details:**
  - `getStateInformation`: Save preset manager state alongside APVTS XML
    ```cpp
    auto state = parameters.copyState();
    auto xml = state.createXml();
    // Add preset manager child element
    if (auto presetXml = presetManager.getStateAsXml())
        xml->addChildElement(presetXml.release());
    copyXmlToBinary(*xml, destData);
    ```
  - `setStateInformation`: Restore preset manager state from child element
    ```cpp
    if (auto* presetEl = xmlState->getChildByName("PresetManagerState"))
        presetManager.setStateFromXml(*presetEl);
    ```
  - Ensures selected preset name persists across DAW save/load

### 10. [ ] Build and verify
- **Files:** none (build/test)
- **Depends on:** Tasks 1-9
- **Details:**
  - `ninja O-Formant_VST3 O-Formant_AU` from build/
  - Install to system folders with cache clear
  - Verify: outputGain knob audibly changes level (sweep -60 to +12 dB)
  - Verify: stereoWidth spreads voices (play chord, sweep 0→1, check stereo field)
  - Verify: all 16 presets load without crash
  - Verify: presets produce distinct sounds matching their descriptions
  - Verify: save/load user preset round-trips correctly
  - Verify: preset name persists across DAW session save/reload

---

## Success Criteria

- [ ] outputGain parameter controls output level with SmoothedValue (no clicks)
- [ ] stereoWidth parameter spreads voices across stereo field by pitch
- [ ] OuariconPresetManager integrated with 16 factory presets (4 categories × 4)
- [ ] Preset browser UI in WebView (prev/next, category, save)
- [ ] All 16 presets load and produce distinct, appropriate sounds
- [ ] User presets save/load correctly
- [ ] State persistence: preset selection survives DAW session save/load
- [ ] Plugin builds clean (VST3 + AU, no warnings)

---

## Files Summary

| Action | File |
|--------|------|
| Modify | `Source/PluginProcessor.h` |
| Modify | `Source/PluginProcessor.cpp` |
| Modify | `Source/FormantVoice.cpp` |
| Modify | `Source/PluginEditor.h` |
| Modify | `Source/PluginEditor.cpp` |
| Modify | `Source/ui/public/index.html` |
| Modify | `Source/ui/public/js/main.js` |
| Create | `Source/OuariconPresetManager.h` (copy from O-Bells) |

---

## Requirements Addressed

| Requirement | Task |
|-------------|------|
| FUNC-12 (Factory presets) | Tasks 4, 5, 7, 8 |
| DSP-06 partial (outputGain smoothing) | Task 1 |
| Phase 2.3 gap (outputGain) | Task 1 |
| Phase 2.3 gap (stereoWidth) | Task 2 |
