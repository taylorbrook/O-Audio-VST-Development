# Stage 1: Foundation - Execution Plan

**Created:** 2026-02-01
**Stage Goal:** Create project structure with CMakeLists.txt and PluginProcessor skeleton with APVTS parameters

---

## Goal

Create the foundational build system and APVTS parameter infrastructure for O-Freeze. The plugin must build successfully as VST3/AU and load in a DAW, with all 5 parameters (FREEZE, THRESHOLD, MODE, DRIFT, MIX) registered and accessible. Audio passes through unchanged (passthrough mode).

---

## Tasks

### 1. [ ] Create CMakeLists.txt
- **Files:** `plugins/O-Freeze/CMakeLists.txt`
- **Depends on:** none
- **Details:**
  - Plugin code: `OFCR` (4 chars for AU)
  - Manufacturer code: `OuDv`
  - Formats: VST3, AU, Standalone
  - Product name: `O-Freeze`
  - NEEDS_WEB_BROWSER: TRUE (for future WebView UI)
  - Link JUCE modules: juce_audio_basics, juce_audio_processors, juce_dsp, juce_gui_basics, juce_gui_extra
  - Generate JuceHeader.h (JUCE 8 requirement)
  - Define: JUCE_WEB_BROWSER=1, JUCE_VST3_CAN_REPLACE_VST2=0, JUCE_USE_CURL=0

### 2. [ ] Create PluginProcessor.h
- **Files:** `plugins/O-Freeze/Source/PluginProcessor.h`
- **Depends on:** none
- **Details:**
  - Class: `OFreezeAudioProcessor`
  - Include: `<juce_audio_processors/juce_audio_processors.h>`, `<juce_dsp/juce_dsp.h>`
  - Declare APVTS: `juce::AudioProcessorValueTreeState parameters`
  - Declare static `createParameterLayout()` method
  - Standard AudioProcessor overrides (prepareToPlay, processBlock, etc.)
  - getName() returns "O-Freeze"
  - acceptsMidi/producesMidi/isMidiEffect return false
  - getTailLengthSeconds returns 0.0 (will update in Stage 2)
  - State management: getStateInformation/setStateInformation using APVTS

### 3. [ ] Create PluginProcessor.cpp
- **Files:** `plugins/O-Freeze/Source/PluginProcessor.cpp`
- **Depends on:** Task 2
- **Details:**
  - Implement `createParameterLayout()` with all 5 parameters:
    ```
    FREEZE (AudioParameterBool): default false
    THRESHOLD (AudioParameterFloat): -60 to 0 dB, default -40 dB
    MODE (AudioParameterChoice): ["Manual", "Threshold"], default Manual
    DRIFT (AudioParameterFloat): 0 to 100%, default 0%
    MIX (AudioParameterFloat): 0 to 100%, default 100% (fully wet)
    ```
  - Constructor: Initialize APVTS with layout, stereo I/O buses
  - prepareToPlay(): Store sample rate (placeholder for Stage 2)
  - releaseResources(): Empty stub
  - processBlock(): Audio passthrough (no DSP yet)
  - getStateInformation/setStateInformation: Serialize APVTS via copyXmlToBinary
  - createEditor(): Return new OFreezeAudioProcessorEditor
  - createPluginFilter(): Factory function

### 4. [ ] Create PluginEditor.h
- **Files:** `plugins/O-Freeze/Source/PluginEditor.h`
- **Depends on:** Task 2
- **Details:**
  - Class: `OFreezeAudioProcessorEditor`
  - Include: `"PluginProcessor.h"`, `<juce_gui_basics/juce_gui_basics.h>`
  - Constructor takes `OFreezeAudioProcessor&`
  - Override: paint(), resized()
  - Store processor reference
  - No parameter controls yet (added in Stage 3)

### 5. [ ] Create PluginEditor.cpp
- **Files:** `plugins/O-Freeze/Source/PluginEditor.cpp`
- **Depends on:** Task 4
- **Details:**
  - Constructor: Set window size to 400x300
  - paint(): Fill with dark gray background (#2a2a2a), draw "O-Freeze" text
  - resized(): Empty (no components to layout yet)
  - Basic placeholder UI indicating plugin name

### 6. [ ] Create Source directory structure
- **Files:** `plugins/O-Freeze/Source/` (directory)
- **Depends on:** none
- **Details:**
  - Create directory if not exists
  - Will contain PluginProcessor.h/cpp, PluginEditor.h/cpp

### 7. [ ] Verify build configuration
- **Files:** none (verification step)
- **Depends on:** Tasks 1-5
- **Details:**
  - Run `cmake --build build --target O-Freeze_VST3 O-Freeze_AU`
  - Verify both formats compile without errors
  - Check for any linker warnings

### 8. [ ] Install and validate in DAW
- **Files:** none (validation step)
- **Depends on:** Task 7
- **Details:**
  - Clear AU cache: `killall -9 AudioComponentRegistrar; rm -rf ~/Library/Caches/AudioUnitCache/`
  - Copy VST3 to: `~/Library/Audio/Plug-Ins/VST3/O-Freeze.vst3`
  - Copy AU to: `~/Library/Audio/Plug-Ins/Components/O-Freeze.component`
  - Verify AU registration: `auval -a | grep -i freeze`
  - Load in DAW, confirm plugin loads without crash
  - Verify audio passes through unchanged

---

## Implementation Notes

### Parameter ID Convention
Use SCREAMING_CASE for parameter IDs to match existing O-Tremolo, O-Detune patterns:
- `FREEZE`, `THRESHOLD`, `MODE`, `DRIFT`, `MIX`

### APVTS Pattern (from O-Tremolo)
```cpp
juce::AudioProcessorValueTreeState::ParameterLayout layout;

layout.add(std::make_unique<juce::AudioParameterBool>(
    juce::ParameterID{"FREEZE", 1}, "Freeze", false));

layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID{"THRESHOLD", 1}, "Threshold",
    juce::NormalisableRange<float>(-60.0f, 0.0f, 0.1f), -40.0f, "dB"));

// etc.
```

### Bus Configuration
Stereo in/out as per existing plugins:
```cpp
AudioProcessor(BusesProperties()
    .withInput("Input", juce::AudioChannelSet::stereo(), true)
    .withOutput("Output", juce::AudioChannelSet::stereo(), true))
```

### State Persistence Notes
- FREEZE button: NOT saved with preset (always starts Off per CONTEXT.md decision)
- For Stage 1, basic APVTS serialization is sufficient
- Stage 3 will handle custom FREEZE state exclusion

---

## Success Criteria

- [ ] CMakeLists.txt follows workspace conventions (matches O-Tremolo/O-Detune patterns)
- [ ] Plugin builds successfully: `ninja O-Freeze_VST3 O-Freeze_AU`
- [ ] All 5 parameters registered in APVTS:
  - FREEZE (Bool, default Off)
  - THRESHOLD (Float, -60 to 0 dB, default -40 dB)
  - MODE (Choice, Manual/Threshold, default Manual)
  - DRIFT (Float, 0-100%, default 0%)
  - MIX (Float, 0-100%, default 100%)
- [ ] Plugin loads in DAW without crash (Logic Pro or Ableton)
- [ ] Audio passes through unchanged (passthrough mode)
- [ ] AU validation passes: `auval -a | grep -i freeze` shows plugin
- [ ] No compiler warnings related to O-Freeze code

---

## Estimated File Count

| File | Action | Lines (approx) |
|------|--------|----------------|
| CMakeLists.txt | Create | 60 |
| Source/PluginProcessor.h | Create | 50 |
| Source/PluginProcessor.cpp | Create | 120 |
| Source/PluginEditor.h | Create | 25 |
| Source/PluginEditor.cpp | Create | 40 |

**Total:** 5 files, ~295 lines

---

## Next Phase

After Stage 1 completes:
1. **Execute** → Run this plan (create files, build, validate)
2. **Verify** → Confirm success criteria met
3. **Stage 2: DSP** → Implement granular freeze engine
