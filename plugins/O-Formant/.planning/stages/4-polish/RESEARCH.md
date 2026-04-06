# Stage 4: Polish - Research

**Date:** 2026-04-05
**Phase:** 4.1 (DSP Completion + Presets) and 4.2 (Validation + Release)

---

## 1. DSP Completion: outputGain + stereoWidth

### 1.1 outputGain — Post-Synth Gain Stage

**Current state:** `PluginProcessor.cpp:211-216` — synthesiser writes directly to buffer, no post-processing.

```cpp
void OFormantAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midi)
{
    juce::ScopedNoDenormals noDenormals;
    buffer.clear();
    synthesiser.renderNextBlock (buffer, midi, 0, buffer.getNumSamples());
    // <-- outputGain goes HERE
}
```

**Reference pattern: O-Bass** (`plugins/O-Bass/Source/PluginProcessor.cpp:252-284`)
- SmoothedValue for gain, per-sample application after synthesiser
- `juce::Decibels::decibelsToGain(dB)` conversion
- Optional soft clip at 0.95 with `std::tanh` rolloff

**Implementation:**
- Add `juce::SmoothedValue<float> outputGainSmoothed { 1.0f };` member to PluginProcessor.h
- In `prepareToPlay()`: `outputGainSmoothed.reset(sampleRate, 0.050);` (50ms ramp per CONTEXT.md)
- In `processBlock()` after renderNextBlock:
  ```cpp
  float targetGain = juce::Decibels::decibelsToGain(parameters.getRawParameterValue("outputGain")->load());
  outputGainSmoothed.setTargetValue(targetGain);
  for (int i = 0; i < buffer.getNumSamples(); ++i) {
      float gain = outputGainSmoothed.getNextValue();
      for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
          buffer.setSample(ch, i, buffer.getSample(ch, i) * gain);
  }
  ```

### 1.2 stereoWidth — Per-Voice Pitch-Based Panning

**Current state:** `FormantVoice.cpp:270-273` — mono to both channels:
```cpp
outL[i] += sample;
if (outR != nullptr)
    outR[i] += sample;
```

**Reference pattern: O-Prism** (`plugins/O-Prism/Source/PrismVoice.cpp:487-505`)
- Equal-power panning: `cos/sin` of normalized pan position
- `panNorm = (panValue + 1.0) * 0.5` maps [-1, 1] -> [0, 1]

**Implementation:**
- Read stereoWidth per-block (block-rate update per CONTEXT.md)
- Compute pan from MIDI note: `panPosition = (noteNorm - 0.5f) * stereoWidth * 2.0f`
- Equal-power gains: `leftGain = cos(panNorm * halfPi)`, `rightGain = sin(panNorm * halfPi)`
- Replace mono write with panned write:
  ```cpp
  outL[i] += sample * panLGain;
  if (outR != nullptr)
      outR[i] += sample * panRGain;
  ```
- No SmoothedValue needed — block-rate update sufficient for panning changes

### 1.3 Parameter Caching (Already Done)

`FormantVoice.h:105-106`:
```cpp
std::atomic<float>* pOutputGain   = nullptr;  // cached via getRawParameterValue
std::atomic<float>* pStereoWidth  = nullptr;  // cached via getRawParameterValue
```

Initialized in `FormantVoice.cpp:49-50` via `setAPVTS()`. Ready to read with `.load()`.

---

## 2. OuariconPresetManager Integration

### 2.1 File Pattern

Single header with inline implementation: `Source/OuariconPresetManager.h`
Copy from O-Bells (has category support needed for 4 genre categories).

### 2.2 Key API

| Method | Returns | Purpose |
|--------|---------|---------|
| `savePreset(name)` | bool | Save user preset |
| `loadPreset(name)` | bool | Load by name |
| `deletePreset(name)` | bool | Delete user preset |
| `getPresetList()` | StringArray | Flat list |
| `getPresetListWithCategories()` | map<String, StringArray> | Category-grouped |
| `getCurrentPresetName()` | String | Current name |
| `getNextPreset()` / `getPreviousPreset()` | String | Navigation |
| `isFactoryPreset(name)` | bool | Factory check |
| `initializeFactoryPresets(vector<FactoryPresetDef>)` | void | Define factory presets |
| `getStateAsXml()` / `setStateFromXml()` | — | State persistence |

### 2.3 PluginProcessor Integration

**Header (PluginProcessor.h):**
```cpp
#include "OuariconPresetManager.h"
// Members (order matters):
juce::AudioProcessorValueTreeState parameters;  // AFTER DSP
OuariconPresetManager presetManager;            // AFTER APVTS
// Public:
OuariconPresetManager& getPresetManager() { return presetManager; }
```

**Constructor (PluginProcessor.cpp):**
```cpp
: parameters(*this, nullptr, "Parameters", createParameterLayout())
, presetManager(parameters, "O-Formant")
{
    std::vector<OuariconPresetManager::FactoryPresetDef> factoryPresets = {
        {"Cinematic", "Creature Growl", {{"glottalRd", 0.4f}, ...}, juce::var()},
        // ... 15 more
    };
    presetManager.initializeFactoryPresets(factoryPresets);
}
```

### 2.4 Factory Preset Storage

```
~/Library/O-Formant/Presets/
├── Factory/
│   ├── Cinematic/
│   │   ├── Creature Growl.json
│   │   ├── Alien Whisper.json
│   │   ├── Sci-Fi Choir.json
│   │   └── Spectral Voice.json
│   ├── Electronic/ (4 presets)
│   ├── Ambient/ (4 presets)
│   └── Speech/ (4 presets)
└── User/
```

### 2.5 JSON Format

```json
{
  "parameters": { "paramId": 0.5, ... },
  "customState": {},
  "version": "1.0.0",
  "plugin": "O-Formant",
  "factory": true,
  "category": "Cinematic"
}
```

**Note:** Parameter values stored as **normalized [0, 1]** (via `param->getValue()`), not raw range values. Factory preset definitions use raw values that get normalized during `initializeFactoryPresets()`.

---

## 3. Preset Browser WebView Integration

### 3.1 Native Functions (10 required)

From O-Bass/O-DigiDelay pattern (no tuning system needed):

1. `savePreset(name)` -> bool
2. `savePresetWithDialog()` -> {success, name}
3. `loadPreset(name)` -> bool
4. `loadPresetFromFile()` -> {success, name}
5. `getPresetList()` -> array
6. `getPresetListWithCategories()` -> {category: [names]}
7. `getCurrentPreset()` -> string
8. `selectNextPreset()` -> string
9. `selectPreviousPreset()` -> string
10. `deletePreset(name)` -> bool

### 3.2 PluginEditor Integration

Add native functions in constructor via `withNativeFunction()` chain on WebBrowserComponent options.

**Member declaration order (critical for destruction):**
1. Relays (no deps)
2. WebBrowserComponent (depends on relays)
3. Attachments (depend on both)

### 3.3 Preset Browser HTML/CSS

Integrate into existing Naturalist aesthetic header area:
- Preset name display (Garamond, centered)
- Prev/Next arrows (moss-green, `#6B8E4E`)
- Save/Load buttons (subtle, bottom-right or header area)
- Category dropdown (optional — can use prev/next to cycle through all)

### 3.4 JavaScript Pattern

Use `window.__JUCE__.backend` native function calls:
```javascript
const loadPreset = window.__JUCE__.backend.getNativeFunction("loadPreset");
const getPresetList = window.__JUCE__.backend.getNativeFunction("getPresetList");
// etc.
```

No external module needed — inline in main.js following existing O-Formant JS patterns.

---

## 4. Pluginval Level 10 Validation

### 4.1 Level 10 vs Level 5 Differences

| Test | Level 5 | Level 10 |
|------|---------|----------|
| Parameter fuzz (random values) | Basic | Extensive (500x iterations) |
| State save/restore cycles | Single | Multiple with verification checksums |
| Sample rate transitions | Standard | Rapid switching (44.1k -> 96k -> 44.1k) |
| Buffer size stress | Normal | Variable sizes, zero-length buffers |
| Concurrent access | Basic | Simultaneous message + audio thread |
| auval stress | `-strict -v` | `-strict -stress 20 -v` |

### 4.2 O-Formant Specific Risks

| Component | Risk | Mitigation |
|-----------|------|------------|
| Newton-Raphson (Rd boundaries) | Convergence failure at Rd=0.3/2.7 | Already has iteration cap; verify boundary handling |
| Biquad coefficients (extreme formantShift) | Unstable at Nyquist | FormantBiquad.h:34-39 has NaN guard; verify frequency clamping in FormantFilterBank |
| outputGain (currently unwired) | Randomized but no effect | **MUST wire before level 10** |
| stereoWidth (currently unwired) | Randomized but no effect | **MUST wire before level 10** |
| ConsonantEngine crossfade | NaN on rapid tone changes | Add NaN guard in processing |
| ADSR state restore | Envelope jump at different sample rates | Verify APVTS state serialization covers this |
| VibratoLFO phase | Jitter on sample rate change | Verify prepare() resets correctly |
| Zero-length buffers | processBlock with numSamples=0 | MPESynthesiser handles this (no iteration) |

### 4.3 Precedent: Other Plugin Passes

- **O-Bass:** Level 10 pass after fixing buffer size mismatch in oversampler
- **O-Prism:** Level 10 pass after adding `std::tanh` soft clipping to noise generators
- **Lesson:** Most failures come from parameter randomization exposing edge cases in custom DSP

### 4.4 Pre-Validation Checklist

Before running level 10:
1. Wire outputGain + stereoWidth (both must respond to automation)
2. Verify Newton-Raphson convergence at Rd=0.3 and Rd=2.7
3. Verify formant frequency clamping with formantShift=+18/-18
4. Quick level 5 smoke test first
5. Run level 10 with `--timeout-ms 120000` (synth tests take longer)

---

## 5. Module Reuse Opportunities

| Module | Source | Purpose |
|--------|--------|---------|
| OuariconPresetManager.h | O-Bells | Preset system with categories |
| preset-manager.js | O-DigiDelay/O-Bass | JS preset browser (optional — may inline) |

No other shared modules needed. DSP completion and preset UI are self-contained.

---

## 6. Implementation Order Summary

### Phase 4.1 (DSP + Presets)
1. Wire outputGain in PluginProcessor (SmoothedValue, post-synth)
2. Wire stereoWidth in FormantVoice (equal-power pan by pitch)
3. Copy OuariconPresetManager.h from O-Bells
4. Integrate PresetManager in PluginProcessor (member, constructor, factory presets)
5. Add 10 native functions to PluginEditor
6. Add preset browser HTML/CSS/JS to index.html
7. Build + test all 16 presets load correctly

### Phase 4.2 (Validation + Release)
1. pluginval level 5 smoke test
2. pluginval level 10 (VST3 + AU)
3. Fix any failures (expect 1-2 iterations)
4. CHANGELOG.md
5. Final build + install

---

## References

**Code patterns used:**
- O-Bass `PluginProcessor.cpp:252-284` — Post-synth gain with SmoothedValue
- O-Prism `PrismVoice.cpp:487-505` — Equal-power pan formula
- O-Bells `Source/OuariconPresetManager.h` — Preset manager with categories
- O-Bass `PluginEditor.cpp:38-122` — Native function pattern for presets
- O-DigiDelay `Source/ui/public/index.html` — Preset browser UI pattern

**O-Formant files to modify:**
- `Source/PluginProcessor.h` — Add outputGainSmoothed member, presetManager member
- `Source/PluginProcessor.cpp` — Wire gain in processBlock, init factory presets
- `Source/FormantVoice.cpp` — Wire stereo width panning in renderNextBlock
- `Source/PluginEditor.h` — (minor, if needed for fileChooser member)
- `Source/PluginEditor.cpp` — Add 10 native functions
- `Resources/index.html` — Add preset browser UI
- `CMakeLists.txt` — No changes expected (WebView already configured)
