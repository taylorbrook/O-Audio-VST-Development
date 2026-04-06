# Stage 1: Foundation - Execution Plan

**Plugin:** O-Formant
**Stage:** 1 of 4 (Foundation)
**Goal:** Build system, APVTS with all 21 parameters, MPESynthesiser skeleton with 16 silent voices

---

## Tasks

### 1. [ ] Create CMakeLists.txt

- **Files:** `plugins/O-Formant/CMakeLists.txt`
- **Depends on:** none
- **Details:**
  - `juce_add_plugin()` with IS_SYNTH TRUE, NEEDS_MIDI_INPUT TRUE, PLUGIN_CODE OuFm
  - NEEDS_WEB_BROWSER FALSE (no WebView until Stage 3)
  - No NEEDS_WEBVIEW2, no binary data, no ouaricon modules
  - JUCE_WEB_BROWSER=0
  - Modules: juce_audio_basics, juce_audio_devices, juce_audio_formats, juce_audio_plugin_client, juce_audio_processors, juce_audio_utils, juce_core, juce_data_structures, juce_dsp, juce_events, juce_graphics, juce_gui_basics, juce_gui_extra
  - Source files: PluginProcessor.cpp, PluginEditor.cpp, FormantVoice.cpp
  - Reference: O-Prism CMakeLists.txt (minus WebView/modules/binary data)

### 2. [ ] Register plugin in top-level CMakeLists.txt

- **Files:** `CMakeLists.txt` (project root)
- **Depends on:** Task 1
- **Details:**
  - Add `add_subdirectory(plugins/O-Formant)` in the plugins section

### 3. [ ] Create FormantVoice (MPESynthesiserVoice stub)

- **Files:** `plugins/O-Formant/Source/FormantVoice.h`, `plugins/O-Formant/Source/FormantVoice.cpp`
- **Depends on:** none
- **Details:**
  - Extends `juce::MPESynthesiserVoice`
  - Override all 7 pure virtuals: `noteStarted()`, `noteStopped()`, `notePressureChanged()`, `notePitchbendChanged()`, `noteTimbreChanged()`, `noteKeyStateChanged()`, `renderNextBlock()`
  - `renderNextBlock()` outputs silence (no-op)
  - `noteStarted()` logs via DBG for voice allocation verification
  - `noteStopped()` calls `clearCurrentNote()`
  - Stores `juce::AudioProcessorValueTreeState*` pointer + 21 cached `std::atomic<float>*` parameter pointers
  - `setAPVTS()` method caches all parameter pointers via `getRawParameterValue()`
  - Bool flag `isActive` for voice state tracking

### 4. [ ] Create PluginProcessor with MPESynthesiser and APVTS

- **Files:** `plugins/O-Formant/Source/PluginProcessor.h`, `plugins/O-Formant/Source/PluginProcessor.cpp`
- **Depends on:** Task 3
- **Details:**
  - Constructor: `BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)`
  - APVTS member initialized with `createParameterLayout()`
  - `juce::MPESynthesiser` member
  - Constructor: create 16 FormantVoice instances, `addVoice()` each, call `setAPVTS()` on each, `enableLegacyMode(2, Range<int>(1, 17))`
  - `prepareToPlay()`: `synthesiser.setCurrentPlaybackSampleRate(sampleRate)`, `setLatencySamples(0)`
  - `processBlock()`: `ScopedNoDenormals`, `buffer.clear()`, `synthesiser.renderNextBlock(buffer, midi, 0, buffer.getNumSamples())`
  - `isBusesLayoutSupported()`: stereo output only
  - State serialization: `getStateInformation()` / `setStateInformation()` using APVTS XML
  - `acceptsMidi()` returns true, `producesMidi()` returns false
  - `static createParameterLayout()` with all 21 parameters (see Task 5)

### 5. [ ] Implement createParameterLayout() with all 21 parameters

- **Files:** `plugins/O-Formant/Source/PluginProcessor.cpp` (within Task 4)
- **Depends on:** Task 4 (same file)
- **Details:**
  - All parameters use `juce::ParameterID{"id", 1}` (version 1)
  - **Vowel Morph (3):** vowelX (0-1, 0.5), vowelY (0-1, 0.5), vowelFocus (1-6, 2.5)
  - **Glottal Source (5):** glottalRd (0.3-2.7, 1.0), breathiness (0-1, 0.1), vibratoRate (0.5-12, 5.5 Hz), vibratoDepth (0-100, 15 cents), vibratoDelay (0-2000, 300 ms, skew 0.4)
  - **Consonant/Noise (4):** consonantLevel (0-1, 0.3), consonantTone (0-1, 0.5), sibilance (0-1, 0.0), autoConsonant (bool, off)
  - **Envelope (4):** attack (0.001-5, 0.01, skew 0.3), decay (0.001-5, 0.3, skew 0.3), sustain (0-1, 0.8), release (0.001-10, 0.5, skew 0.3)
  - **Voice Character (3):** formantShift (-24-24, 0 st), formantSpread (0.5-2, 1.0), pitchGlide (0-1000, 0 ms, skew 0.3)
  - **Output (2):** outputGain (-60-12, 0 dB), stereoWidth (0-1, 0.5)
  - 20 AudioParameterFloat + 1 AudioParameterBool (autoConsonant)

### 6. [ ] Create PluginEditor (generic placeholder)

- **Files:** `plugins/O-Formant/Source/PluginEditor.h`, `plugins/O-Formant/Source/PluginEditor.cpp`
- **Depends on:** Task 4
- **Details:**
  - Use `juce::GenericAudioProcessorEditor` — shows all 21 parameters as sliders
  - `createEditor()` returns `new juce::GenericAudioProcessorEditor(*this)`
  - `hasEditor()` returns true
  - Set window size to 600x800 to accommodate all parameters

### 7. [ ] Build and validate

- **Files:** none (build system commands)
- **Depends on:** Tasks 1-6
- **Details:**
  - CMake configure: `cmake -B build -G Ninja`
  - Build: `ninja -C build O-Formant_VST3 O-Formant_AU`
  - Fix any compile warnings/errors
  - Run pluginval basic scan
  - Install to system folders and verify DAW load

---

## File Summary

| File | Action |
|------|--------|
| `plugins/O-Formant/CMakeLists.txt` | Create |
| `CMakeLists.txt` (root) | Edit (add subdirectory) |
| `plugins/O-Formant/Source/FormantVoice.h` | Create |
| `plugins/O-Formant/Source/FormantVoice.cpp` | Create |
| `plugins/O-Formant/Source/PluginProcessor.h` | Create |
| `plugins/O-Formant/Source/PluginProcessor.cpp` | Create |
| `plugins/O-Formant/Source/PluginEditor.h` | Create |
| `plugins/O-Formant/Source/PluginEditor.cpp` | Create |

**Total:** 7 new files + 1 edit

---

## Success Criteria

- [ ] Plugin builds (VST3 + AU) with no warnings
- [ ] Plugin loads in DAW as instrument
- [ ] MIDI notes trigger voice allocation (DBG log)
- [ ] All 21 parameters visible in DAW automation
- [ ] pluginval passes basic scan
- [ ] State serialization works (save/reload DAW session)
