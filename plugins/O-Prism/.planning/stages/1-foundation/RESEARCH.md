# Stage 1: Foundation - Research

**Date:** 2026-02-16
**Plugin:** O-Prism (Microtonal Wavetable Synthesizer)
**Stage:** 1-foundation
**Phase:** research

---

## 1. Module Integration Research

### 1a. scala-tuning-engine v2.1.0

**Location:** `modules/tuning/scala-tuning-engine/`

**Files to copy to `Source/`:**

| File | Lines | Purpose |
|------|-------|---------|
| `TuningEngine.h/cpp` | ~440 | Core frequency calculation, atomic 128-note table, Scala/KBM parsing |
| `ScaleGenerator.h/cpp` | ~170 | Static utility: EDO, harmonic series, rank-2 generators |
| `EmbeddedTunings.h/cpp` | ~160 | 24+ factory presets across 5 categories |
| `TuningExporter.h/cpp` | ~160 | HTML/SVG export (optional, low priority) |

**Core API for audio thread:**
```cpp
double freq = tuningEngine.getFrequency(midiNoteNumber);  // Lock-free atomic read
```

**Thread safety:** Uses `std::array<std::atomic<double>, 128> frequencyTable` -- no locks needed on audio thread.

**Parameter ID adaptation (from CONTEXT.md):**

| Module Convention | O-Prism ID | Notes |
|-------------------|-----------|-------|
| `tuning_masterTune` | `masterTune` | Range: 420-460 Hz (BRIEF says 420-460, module supports 400-480) |
| `tuning_tuningMode` | *(not used)* | No MTS-ESP in v1.0; mode implicit from tuningPreset |
| `tuning_octaveStretch` | `octaveStretch` | Range: 0.95-1.25 |
| `tuning_pitchBendRange` | `pitchBendRange` | Range: 1-48 semitones |
| `tuning_temperamentPreset` | `tuningPreset` | Maps to EmbeddedTunings index |

**Native functions for WebView:** 23 total functions registered via `webView->withNativeFunction()`. These are documented in `snippets/native-functions.cpp` but have 3 API discrepancies to fix:
- `EmbeddedTunings::getTuningList()` should be `getAllTunings()`
- `EmbeddedTunings::getTuning(id)` should be `getTuningById(id)`
- `TuningExporter::generateHTML()` should be `toHTML()`

**Integration checklist:** 11-step guide at `snippets/INTEGRATION-CHECKLIST.md`. Covers file copy, CMake, APVTS params, processor header, native functions, state persistence, JS/CSS copy, HTML container, audio processing, and testing.

**State persistence format:**
```xml
<tuningEngine intervals="0,100,200,...,1100" scaleName="12-TET" tonic="0" preset="0"/>
```
Intervals are comma-separated cents values (6 decimal places), excluding period.

### 1b. webview-relay-manager v1.0.0

**Location:** `modules/core/webview-relay-manager/`
**Type:** Header-only (`cpp/WebViewRelayManager.h`)

**Three-step API pattern:**

```cpp
// Step 1: Create relays (before WebView)
relayManager.createSliderRelay("oscAPos");
relayManager.createToggleRelay("delaySync");  // For booleans

// Step 2: Initialize WebView (auto-applies .withOptionsFrom() for all relays)
auto* webView = relayManager.initializeWebView(
    [this](const auto& url) { return getResource(url); });

// Step 3: Create attachments (after WebView)
relayManager.createSliderAttachment("oscAPos",
    *processorRef.parameters.getParameter("oscAPos"));
```

**Scales to 68+ parameters:** Uses `std::vector` + `std::map` internally for O(1) lookup.

**Critical destruction order (handled automatically):**
1. Attachments destroyed first (WebView still alive -- safe)
2. WebView destroyed second
3. Relays destroyed last

**Toggle relays needed for:** `delaySync` (boolean parameter). All other 67 params use slider relays.

### 1c. OuariconModules.cmake Integration

**CMake pattern for module inclusion:**
```cmake
include(${CMAKE_SOURCE_DIR}/modules/cmake/OuariconModules.cmake)
ouaricon_add_module(O-Prism webview-relay-manager)
```

This adds the module's `cpp/` directory to the include path.

---

## 2. Reference Architecture Research (O-Lyrica Pattern)

### Voice Architecture Pattern

**Processor owns shared engines, passes pointers to voices:**

```cpp
// In OPrismAudioProcessor constructor:
for (int i = 0; i < 16; ++i) {
    auto* voice = new PrismVoice();
    voice->setAPVTS(&parameters);
    voice->setTuningEngine(&tuningEngine);
    synthesiser.addVoice(voice);
}
synthesiser.addSound(new PrismSound());
```

**Voice reads parameters atomically:**
```cpp
float value = parameters->getRawParameterValue("oscAPos")->load();
```

**prepareToPlay sequence:**
1. `synthesiser.setCurrentPlaybackSampleRate(sampleRate)`
2. Prepare shared engines (TuningEngine doesn't need explicit prepare)
3. Loop voices: `voice->prepare(sampleRate, samplesPerBlock)`

**processBlock sequence:**
1. `juce::ScopedNoDenormals noDenormals;`
2. `buffer.clear();`
3. Update TuningEngine params from APVTS (masterTune, octaveStretch, etc.)
4. `synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());`
5. Apply effects chain (global, post-voice-sum)
6. Apply master volume

**Voice startNote:**
```cpp
double freq = tuningEngine->getFrequency(midiNoteNumber);
freq *= std::pow(2.0, (coarseSemitones + fineCents / 100.0) / 12.0);
```

**Voice pitchWheelMoved:**
```cpp
float normalizedBend = (newPitchWheelValue - 8192.0f) / 8192.0f;
tuningEngine->setPitchBend(currentMidiNote, normalizedBend);
currentFrequency = tuningEngine->getFrequency(currentMidiNote);
```

**PrismSound (minimal):**
```cpp
class PrismSound : public juce::SynthesiserSound {
    bool appliesToNote(int) override { return true; }
    bool appliesToChannel(int) override { return true; }
};
```

### BusesProperties for Synth

```cpp
// Output-only (synth instrument, no audio input)
AudioProcessor(BusesProperties()
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
```

---

## 3. CMakeLists.txt Research

### Required Configuration

From O-AnalogEQ (correct cross-platform), O-Marimba (synth), and O-Texture (complex synth):

```cmake
juce_add_plugin(O-Prism
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuPr                    # Unique 4-char code
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Prism${OUARICON_DEV_SUFFIX}"
    VERSION 0.1.0
    IS_SYNTH TRUE                       # Instrument, not effect
    NEEDS_MIDI_INPUT TRUE               # Required for synth
    NEEDS_MIDI_OUTPUT FALSE
    NEEDS_WEB_BROWSER TRUE              # WebView UI
    NEEDS_WEBVIEW2 TRUE                 # Windows WebView2
)

juce_generate_juce_header(O-Prism)      # JUCE 8 requirement (Pattern #1)

target_compile_definitions(O-Prism
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1  # Windows static linking
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)
```

### JUCE Module Dependencies (7 modules)

| Module | Required For |
|--------|-------------|
| `juce_audio_basics` | Synthesiser, SynthesiserVoice, ADSR, MidiBuffer, AudioBuffer |
| `juce_audio_processors` | AudioProcessor, AudioProcessorValueTreeState |
| `juce_audio_formats` | AudioFormatManager, AudioFormatReader (wavetable import) |
| `juce_dsp` | StateVariableTPTFilter, DelayLine, Reverb, Chorus, FFT, Oversampling, IIR |
| `juce_gui_basics` | Component |
| `juce_gui_extra` | WebBrowserComponent |
| `juce_core` | File, Random, String, ValueTree |

### BinaryData for WebView

```cmake
juce_add_binary_data(O-Prism_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js  # CRITICAL (Pattern #13)
)

target_link_libraries(O-Prism
    PRIVATE
        O-Prism_UIResources
        juce::juce_audio_basics
        juce::juce_audio_processors
        juce::juce_audio_formats
        juce::juce_dsp
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_core
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
```

---

## 4. JUCE 8 Critical Patterns (Applicable to Stage 1)

From `troubleshooting/patterns/juce8-critical-patterns.md`:

| # | Pattern | Impact | Notes |
|---|---------|--------|-------|
| 1 | `juce_generate_juce_header()` placement | Build failure | After `target_link_libraries`, before `target_compile_definitions` |
| 8 | Resource provider explicit URL mapping | Blank page | Map each BinaryData file to URL path in `resourceProvider` |
| 9 | `NEEDS_WEB_BROWSER TRUE` | VST3 invisible | Required for VST3 to appear in DAW |
| 11 | WebView member init via unique_ptr | Crash on unload | Use WebViewRelayManager (handles this) |
| 12 | WebSliderParameterAttachment 3 params | Compile error | `(parameter, relay, nullptr)` -- third arg is undoManager |
| 13 | check_native_interop.js | WebView crash | Must include in BinaryData and serve from resource provider |
| 15 | valueChangedEvent callback | Wrong values | Callback receives NO params; call `getNormalisedValue()` inside |
| 19 | Boolean parameters | Runtime error | Use `getToggleState()` NOT `getSliderState()` for `delaySync` |
| 21 | ES6 module loading | JS error | Script tags MUST use `type="module"` |
| 22 | IS_SYNTH flag | Wrong category | Must set `IS_SYNTH TRUE` + `NEEDS_MIDI_INPUT TRUE` |

### HTML Template Pattern

```html
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <style>
        html, body {
            height: 100%;       /* NOT 100vh -- JUCE requirement */
            margin: 0;
            padding: 0;
            overflow: hidden;
        }
    </style>
</head>
<body>
    <div id="app">O-Prism Loading...</div>
    <script type="module">
        import { getSliderState, getToggleState } from './js/juce/index.js';
        // Parameter binding here
    </script>
</body>
</html>
```

---

## 5. APVTS Parameter Organization

### 68 Parameters by Section

**Oscillator A (10):** oscATable, oscAPos, oscALevel, oscAPan, oscACoarse, oscAFine, oscAPhase, oscAUnison, oscADetune, oscAWidth

**Oscillator B (10):** oscBTable, oscBPos, oscBLevel, oscBPan, oscBCoarse, oscBFine, oscBPhase, oscBUnison, oscBDetune, oscBWidth

**Sub + Noise (5):** subShape, subOctave, subLevel, noiseType, noiseLevel

**Amplitude Envelope (4):** ampAttack, ampDecay, ampSustain, ampRelease

**Filter Envelope (5):** filtAttack, filtDecay, filtSustain, filtRelease, filtEnvDepth

**Filter A (5):** filtAType, filtACutoff, filtARes, filtADrive, filtAKeyTrack

**Filter B (5):** filtBType, filtBCutoff, filtBRes, filtBDrive, filtBKeyTrack

**Filter Routing (1):** filtRouting

**Tuning (7):** tuningPreset, tonic, masterTune, octaveStretch, pitchBendRange, glideMode, glideTime

**Effects - Reverb (4):** reverbSize, reverbDamp, reverbPredelay, reverbMix

**Effects - Delay (5):** delayTime, delayFeedback, delaySync, delayMode, delayMix

**Effects - Chorus (3):** chorusRate, chorusDepth, chorusMix

**Effects - Distortion (3):** distType, distDrive, distMix

**Effects - EQ (4):** eqLowGain, eqMidGain, eqMidFreq, eqHighGain

**Global (3):** masterVol, oscMix, polyphony

**Total: 68 parameters**

### Parameter Type Breakdown

| Type | Count | APVTS Class | Relay Type |
|------|-------|-------------|------------|
| Float (continuous) | 49 | `AudioParameterFloat` | WebSliderRelay |
| Int (discrete) | 8 | `AudioParameterInt` | WebSliderRelay |
| Choice (enum) | 10 | `AudioParameterChoice` | WebSliderRelay |
| Bool (toggle) | 1 | `AudioParameterBool` | WebToggleButtonRelay |

**Bool parameter:** `delaySync` (On/Off)

**Choice parameters (10):** subShape, noiseType, filtAType, filtBType, filtRouting, tuningPreset, glideMode, delayMode, distType, tonic

**Int parameters (8):** oscATable, oscBTable, oscACoarse, oscBCoarse, oscAUnison, oscBUnison, subOctave, pitchBendRange, polyphony
*(Note: 9 ints -- pitchBendRange and polyphony are also ints)*

**Correction:** Counting from ARCHITECTURE.md parameter table:
- oscATable (Int), oscBTable (Int), oscACoarse (Int), oscBCoarse (Int), oscAUnison (Int), oscBUnison (Int), subOctave (Int), tonic (Int), pitchBendRange (Int), polyphony (Int) = **10 Ints**
- But tonic is listed as Choice (0-11) in CONTEXT.md. ARCHITECTURE.md says Int 0-11. Use **AudioParameterChoice** with note names for better DAW display.

### Skew factors needed

| Parameter | Range | Skew | Reason |
|-----------|-------|------|--------|
| filtACutoff, filtBCutoff | 20-20000 Hz | ~0.25 (logarithmic) | Frequency perception is logarithmic |
| ampAttack, ampDecay, filtAttack, filtDecay | 0.001-10.0s | ~0.35 | Time perception is logarithmic |
| ampRelease, filtRelease | 0.001-20.0s | ~0.3 | Same |
| glideTime | 0.001-5.0s | ~0.35 | Same |
| reverbPredelay | 0-200ms | ~0.5 | Moderate skew |
| delayTime | 0.001-2.0s | ~0.35 | Time is logarithmic |
| eqMidFreq | 200-8000 Hz | ~0.35 | Frequency is logarithmic |
| chorusRate | 0.1-10.0 Hz | ~0.4 | Rate perception |

---

## 6. State Persistence Strategy

### APVTS Handles 68 Parameters Automatically

```cpp
void getStateInformation(juce::MemoryBlock& destData) override {
    auto state = parameters.copyState();
    // Add custom tuning state
    auto tuningState = state.getOrCreateChildWithName("tuningEngine", nullptr);
    tuningState.setProperty("intervals", tuningEngine.getIntervalsAsString(), nullptr);
    tuningState.setProperty("scaleName", tuningEngine.getScaleName(), nullptr);
    tuningState.setProperty("tonic", tuningEngine.getTonicNote(), nullptr);

    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}
```

### Custom State (non-APVTS)

- Tuning intervals (comma-separated cents string)
- Scale name
- Wavetable selection per oscillator (table index or file path) -- deferred, Stage 1 uses default
- KBM state -- deferred, not critical for Stage 1

---

## 7. Potential Pitfalls

### High-Risk Items for Stage 1

1. **68-parameter APVTS creation** -- Largest in catalog. Must organize `createParameterLayout()` carefully with helper functions per section.

2. **WebViewRelayManager with 68 relays** -- Ensure all relays are created before `initializeWebView()` call. Loop through parameter definitions rather than 68 manual calls.

3. **check_native_interop.js missing from BinaryData** -- Pattern #13. Without this file, WebView will crash on load. Must be included in `juce_add_binary_data()` and served from resource provider.

4. **JUCE header generation order** -- Pattern #1. `juce_generate_juce_header()` must come after `target_link_libraries` but before `target_compile_definitions`.

5. **Tuning module API discrepancies in snippets** -- The native-functions.cpp snippet references 3 methods that don't match headers. Fix during integration.

6. **Float vs Choice for tonic parameter** -- ARCHITECTURE.md says Int 0-11, but for better DAW display use `AudioParameterChoice` with note names (C, C#, D, ..., B).

### Low-Risk Items

- **PrismVoice/PrismSound stubs** -- Minimal code, proven pattern from O-Lyrica.
- **TuningEngine copy** -- Proven code, just file copy + parameter ID adaptation.
- **WebView placeholder HTML** -- Simple template, can be minimal for Stage 1.

---

## 8. File Structure Plan

```
plugins/O-Prism/
    CMakeLists.txt
    Source/
        PluginProcessor.h
        PluginProcessor.cpp
        PluginEditor.h
        PluginEditor.cpp
        PrismVoice.h
        PrismVoice.cpp
        PrismSound.h
        TuningEngine.h
        TuningEngine.cpp
        ScaleGenerator.h
        ScaleGenerator.cpp
        EmbeddedTunings.h
        EmbeddedTunings.cpp
        TuningExporter.h
        TuningExporter.cpp
        WebViewRelayManager.h          (copied from module)
        ui/
            public/
                index.html             (placeholder)
                js/
                    juce/
                        index.js       (JUCE WebView bridge)
                        check_native_interop.js  (CRITICAL)
```

---

## 9. Research Summary

| Area | Finding | Confidence |
|------|---------|------------|
| Tuning module integration | Well-documented, 11-step checklist, 3 API fixes needed in snippets | HIGH |
| WebView relay management | Header-only module, 3-step API, scales to 68+ params | HIGH |
| Voice architecture | O-Lyrica pattern proven, shared engine pointers to voices | HIGH |
| CMakeLists.txt | Multiple reference plugins available, pattern well-established | HIGH |
| APVTS 68 parameters | Organize by section using helper functions, use loops for relay creation | HIGH |
| Critical JUCE 8 patterns | 10 patterns applicable to Stage 1, all documented | HIGH |
| State persistence | APVTS auto + custom ValueTree for tuning state | HIGH |
| Destruction order | WebViewRelayManager handles automatically | HIGH |

**Overall assessment:** Stage 1 Foundation is well-researched with proven patterns from the codebase. The primary complexity is the 68-parameter count (highest in catalog) requiring careful organization but no novel techniques. All modules are documented and ready for integration.

---

*Research completed: 2026-02-16*
*Next phase: plan*
