# Stage 1: Foundation - Execution Plan

**Date:** 2026-02-16
**Plugin:** O-Prism (Microtonal Wavetable Synthesizer)
**Stage:** 1-foundation
**Phase:** plan

---

## Goal

Build the complete project scaffolding for O-Prism: CMake build system, APVTS with all 68 parameters, PluginProcessor with Synthesiser + TuningEngine, PluginEditor with WebView shell using WebViewRelayManager for 68 parameter relays, stub voice/sound classes, and placeholder WebView UI. The plugin must build, load in a DAW as an instrument, accept MIDI, expose all 68 parameters to automation, and produce silence.

---

## Tasks

### Task 1: Copy tuning engine source files from module
- **Files to create:**
  - `Source/TuningEngine.h` (copy from `modules/tuning/scala-tuning-engine/cpp/`)
  - `Source/TuningEngine.cpp`
  - `Source/ScaleGenerator.h`
  - `Source/ScaleGenerator.cpp`
  - `Source/EmbeddedTunings.h`
  - `Source/EmbeddedTunings.cpp`
  - `Source/TuningExporter.h`
  - `Source/TuningExporter.cpp`
- **Depends on:** None
- **Notes:** Direct file copy. No modifications needed at this stage -- parameter ID adaptation happens in the processor when reading APVTS values, not in the engine itself.

### Task 2: Copy WebView bridge files
- **Files to create:**
  - `Source/ui/public/js/juce/index.js` (copy from `plugins/O-AnalogEQ/Source/ui/public/js/juce/`)
  - `Source/ui/public/js/juce/check_native_interop.js` (copy from same)
- **Depends on:** None
- **Notes:** These are standard JUCE 8 WebView bridge files. `check_native_interop.js` is CRITICAL (Pattern #13) -- without it, WebView crashes on load.

### Task 3: Create placeholder index.html
- **Files to create:**
  - `Source/ui/public/index.html`
- **Depends on:** None
- **Notes:** Minimal placeholder with:
  - `height: 100%` (NOT 100vh -- JUCE requirement)
  - `margin: 0; padding: 0; overflow: hidden`
  - `<script type="module">` (Pattern #21: ES6 module loading)
  - Import from `./js/juce/index.js`
  - Display "O-Prism Loading..." placeholder text

### Task 4: Create PrismSound.h
- **Files to create:**
  - `Source/PrismSound.h`
- **Depends on:** None
- **Notes:** Minimal SynthesiserSound subclass. `appliesToNote()` returns true for all notes, `appliesToChannel()` returns true for all channels. Header-only, no .cpp needed.

### Task 5: Create PrismVoice.h/cpp (stub)
- **Files to create:**
  - `Source/PrismVoice.h`
  - `Source/PrismVoice.cpp`
- **Depends on:** None
- **Notes:** Stub SynthesiserVoice subclass following O-Lyrica pattern:
  - Raw pointers: `juce::AudioProcessorValueTreeState* parameters`, `TuningEngine* tuningEngine`
  - Setter methods: `setAPVTS()`, `setTuningEngine()`
  - `canPlaySound()` -- returns true for PrismSound
  - `startNote()` -- stores frequency from TuningEngine, sets `isActive`
  - `stopNote()` -- clears voice
  - `renderNextBlock()` -- produces silence (buffer.clear(), no DSP yet)
  - `pitchWheelMoved()` -- stub (stores value)
  - `controllerMoved()` -- stub (empty)
  - `prepare(double sampleRate, int samplesPerBlock)` -- stores sample rate

### Task 6: Create PluginProcessor.h/cpp
- **Files to create:**
  - `Source/PluginProcessor.h`
  - `Source/PluginProcessor.cpp`
- **Depends on:** Task 1 (TuningEngine), Task 4 (PrismSound), Task 5 (PrismVoice)
- **Notes:** This is the largest task. Key components:
  - **APVTS with 68 parameters** organized by section using helper functions:
    - `createOscAParameters()` (10 params)
    - `createOscBParameters()` (10 params)
    - `createSubNoiseParameters()` (5 params)
    - `createAmpEnvelopeParameters()` (4 params)
    - `createFilterEnvelopeParameters()` (5 params)
    - `createFilterAParameters()` (5 params)
    - `createFilterBParameters()` (5 params)
    - `createFilterRoutingParameters()` (1 param)
    - `createTuningParameters()` (7 params)
    - `createReverbParameters()` (4 params)
    - `createDelayParameters()` (5 params)
    - `createChorusParameters()` (3 params)
    - `createDistortionParameters()` (3 params)
    - `createEQParameters()` (4 params)
    - `createGlobalParameters()` (3 params) -- masterVol, oscMix, polyphony
    - Each returns `std::vector<std::unique_ptr<juce::RangedAudioParameter>>`
    - Combined in `createParameterLayout()` static method
  - **Skew factors** for logarithmic parameters:
    - filtACutoff, filtBCutoff: ~0.25 (20-20000 Hz)
    - ampAttack, ampDecay, filtAttack, filtDecay: ~0.35 (0.001-10s)
    - ampRelease, filtRelease: ~0.3 (0.001-20s)
    - glideTime: ~0.35 (0.001-5s)
    - delayTime: ~0.35 (0.001-2s)
    - eqMidFreq: ~0.35 (200-8000 Hz)
    - chorusRate: ~0.4 (0.1-10 Hz)
    - reverbPredelay: ~0.5 (0-200ms)
  - **Members:**
    - `juce::AudioProcessorValueTreeState parameters`
    - `juce::Synthesiser synthesiser`
    - `TuningEngine tuningEngine`
    - `ScaleGenerator scaleGenerator`
    - `EmbeddedTunings embeddedTunings` (if needed as member)
    - `TuningExporter tuningExporter`
  - **Constructor:** Create 16 PrismVoice instances, inject APVTS + TuningEngine pointers, add PrismSound
  - **BusesProperties:** Output-only stereo (synth, no audio input)
  - **processBlock():**
    1. `juce::ScopedNoDenormals noDenormals;`
    2. `buffer.clear();`
    3. Update TuningEngine params from APVTS (masterTune, octaveStretch, pitchBendRange)
    4. `synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());`
    5. Apply masterVol
  - **prepareToPlay():** Set synth sample rate, `setLatencySamples(0)`
  - **getStateInformation/setStateInformation:** APVTS state + tuning engine custom state (intervals, scaleName, tonic)
  - **Public accessors:** `getTuningEngine()`, `getScaleGenerator()`, `getTuningExporter()`
  - **Parameter types:**
    - 49 `AudioParameterFloat` (continuous knobs)
    - 8 `AudioParameterInt` (oscATable, oscBTable, oscACoarse, oscBCoarse, oscAUnison, oscBUnison, subOctave, polyphony)
    - 10 `AudioParameterChoice` (subShape, noiseType, filtAType, filtBType, filtRouting, tuningPreset, tonic, glideMode, delayMode, distType)
    - 1 `AudioParameterBool` (delaySync)

### Task 7: Create PluginEditor.h/cpp
- **Files to create:**
  - `Source/PluginEditor.h`
  - `Source/PluginEditor.cpp`
- **Depends on:** Task 6 (PluginProcessor), Task 2 (WebView JS files), Task 3 (index.html)
- **Notes:** Uses WebViewRelayManager module for 68 parameters.
  - **Include:** `WebViewRelayManager.h` from module include path
  - **Member:** `WebViewRelayManager relayManager;`
  - **Constructor sequence (3-step API):**
    1. Create 67 slider relays + 1 toggle relay via loops over parameter definition arrays
    2. `relayManager.initializeWebView(resourceProvider)` -- constructs WebBrowserComponent with all relay options
    3. Create 67 slider attachments + 1 toggle attachment via loops
  - **Parameter definition arrays:** Use `struct ParamDef { const char* id; bool isToggle; }` arrays organized by section for clean loop-based relay/attachment creation
  - **Resource provider:** `getResource(url)` maps BinaryData files to URL paths (Pattern #8)
  - **WebView size:** `setSize(1200, 800)`
  - **Windows WebView2 user data folder:** Set `withUserDataFolder()` to temp directory
  - **Tuning native functions:** Register 23 native functions from module snippet (with 3 API name fixes from RESEARCH.md)
  - **resized():** WebView fills full editor bounds

### Task 8: Create CMakeLists.txt
- **Files to create:**
  - `CMakeLists.txt` (in `plugins/O-Prism/`)
- **Depends on:** All other tasks (needs to reference all source files)
- **Notes:** Follow canonical ordering from reference plugins:
  1. `cmake_minimum_required(VERSION 3.15)`
  2. `include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)`
  3. `juce_add_plugin` with:
     - `PLUGIN_CODE OuPr` (unique 4-char)
     - `VERSION 0.1.0`
     - `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `NEEDS_MIDI_OUTPUT FALSE`
     - `NEEDS_WEB_BROWSER TRUE`, `NEEDS_WEBVIEW2 TRUE`
  4. `target_sources` -- all 12 .cpp files:
     - `Source/PluginProcessor.cpp`
     - `Source/PluginEditor.cpp`
     - `Source/PrismVoice.cpp`
     - `Source/TuningEngine.cpp`
     - `Source/ScaleGenerator.cpp`
     - `Source/EmbeddedTunings.cpp`
     - `Source/TuningExporter.cpp`
  5. `target_include_directories` -- `Source`
  6. `target_link_libraries` -- all standard JUCE modules (14 modules from reference)
  7. `juce_generate_juce_header` -- AFTER target_link_libraries (Pattern #1)
  8. `ouaricon_add_module(O-Prism webview-relay-manager)`
  9. `target_compile_definitions`:
     - `JUCE_WEB_BROWSER=1`
     - `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`
     - `JUCE_USE_CURL=0`
     - `JUCE_VST3_CAN_REPLACE_VST2=0`
  10. `juce_add_binary_data(O-Prism_UIResources)`:
      - `Source/ui/public/index.html`
      - `Source/ui/public/js/juce/index.js`
      - `Source/ui/public/js/juce/check_native_interop.js`
  11. `target_link_libraries(O-Prism PRIVATE O-Prism_UIResources)`

### Task 9: Build and verify
- **Depends on:** All tasks (1-8)
- **Actions:**
  1. Run CMake configure from project root
  2. Build with `ninja O-Prism_VST3 O-Prism_AU` in build directory
  3. Fix any compilation errors
  4. Clear AU cache and install to system plugin folders
  5. Verify plugin loads in DAW as instrument
  6. Verify 68 parameters appear in automation list
  7. Verify WebView opens at 1200x800
  8. Verify plugin produces silence with no crashes

---

## File Summary

### Files to Create (16 total)

| # | File | Source | Size Est. |
|---|------|--------|-----------|
| 1 | `CMakeLists.txt` | New | ~80 lines |
| 2 | `Source/PluginProcessor.h` | New | ~80 lines |
| 3 | `Source/PluginProcessor.cpp` | New | ~450 lines (68 params) |
| 4 | `Source/PluginEditor.h` | New | ~40 lines |
| 5 | `Source/PluginEditor.cpp` | New | ~250 lines (relays, native funcs) |
| 6 | `Source/PrismVoice.h` | New | ~40 lines |
| 7 | `Source/PrismVoice.cpp` | New | ~60 lines |
| 8 | `Source/PrismSound.h` | New | ~20 lines |
| 9 | `Source/TuningEngine.h` | Copy from module | ~200 lines |
| 10 | `Source/TuningEngine.cpp` | Copy from module | ~240 lines |
| 11 | `Source/ScaleGenerator.h` | Copy from module | ~50 lines |
| 12 | `Source/ScaleGenerator.cpp` | Copy from module | ~120 lines |
| 13 | `Source/EmbeddedTunings.h` | Copy from module | ~40 lines |
| 14 | `Source/EmbeddedTunings.cpp` | Copy from module | ~120 lines |
| 15 | `Source/TuningExporter.h` | Copy from module | ~30 lines |
| 16 | `Source/TuningExporter.cpp` | Copy from module | ~130 lines |
| 17 | `Source/ui/public/index.html` | New | ~25 lines |
| 18 | `Source/ui/public/js/juce/index.js` | Copy from O-AnalogEQ | standard |
| 19 | `Source/ui/public/js/juce/check_native_interop.js` | Copy from O-AnalogEQ | standard |

### Files to Modify (0)

No existing files modified -- this is a new plugin.

---

## Dependencies

```
Task 1 (Tuning Engine Copy) ──┐
Task 2 (WebView JS Copy) ─────┤
Task 3 (index.html) ──────────┤
Task 4 (PrismSound) ──────────┼──→ Task 6 (Processor) ──→ Task 7 (Editor) ──→ Task 8 (CMake) ──→ Task 9 (Build)
Task 5 (PrismVoice) ──────────┘
```

Tasks 1-5 are independent and can be executed in parallel.
Task 6 depends on 1, 4, 5 (needs TuningEngine types, PrismVoice, PrismSound).
Task 7 depends on 6 (needs processor type).
Task 8 depends on all source files being defined.
Task 9 depends on everything.

---

## Success Criteria

- [ ] Project builds with no errors (VST3 + AU + Standalone)
- [ ] Plugin loads in DAW as instrument (appears in synth/instrument category)
- [ ] DAW routes MIDI to plugin (verify with MIDI monitor)
- [ ] All 68 parameters visible in DAW automation list
- [ ] Plugin produces silence (no DSP yet, but no crashes)
- [ ] WebView opens at 1200x800 with placeholder content
- [ ] State save/restore works (APVTS round-trip)
- [ ] No crashes on plugin load, unload, or DAW session restore

---

## Risk Mitigations

| Risk | Mitigation |
|------|------------|
| 68-parameter APVTS is largest in catalog | Organize with per-section helper functions; systematic naming |
| 68 relays via WebViewRelayManager | Use loop-based creation with parameter definition arrays |
| check_native_interop.js missing (Pattern #13) | Explicitly listed in BinaryData and resource provider |
| juce_generate_juce_header order (Pattern #1) | Place after target_link_libraries per reference plugins |
| Windows WebView2 blank page | NEEDS_WEBVIEW2 TRUE + JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1 |
| Tuning module API discrepancies | 3 fixes documented in RESEARCH.md; apply in native function registration |

---

*Plan created: 2026-02-16*
*Ready for: execute phase*
