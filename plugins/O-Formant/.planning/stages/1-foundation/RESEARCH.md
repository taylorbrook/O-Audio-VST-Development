# Stage 1: Foundation - Research

**Plugin:** O-Formant
**Stage:** 1 of 4 (Foundation)
**Goal:** Build system, APVTS with 21 parameters, MPESynthesiser skeleton with 16 silent voices

---

## 1. JUCE APIs Required

### MPESynthesiser (First use in this project)

No existing plugin uses MPESynthesiser -- O-Formant is the first. Key differences from O-Prism's basic `juce::Synthesiser`:

| Aspect | Basic Synthesiser (O-Prism) | MPESynthesiser (O-Formant) |
|--------|---------------------------|----------------------------|
| Sound class | Requires `SynthesiserSound` subclass + `addSound()` | **No sound class needed** |
| Voice callbacks | `startNote(noteNum, velocity, sound, pitchwheel)` | `noteStarted()` -- access note via `currentlyPlayingNote` |
| Note data | Simple note number + velocity | Full `MPENote` object with pressure, pitchbend, timbre |
| Expression | Manual CC handling | Built-in `notePressureChanged()`, `notePitchbendChanged()`, `noteTimbreChanged()`, `noteKeyStateChanged()` |
| Legacy MIDI | Native | `enableLegacyMode(2, Range<int>(1, 17))` for standard MIDI fallback |

### MPESynthesiserVoice -- Pure Virtual Methods to Override

```cpp
void noteStarted() = 0;               // Voice assigned a note
void noteStopped(bool allowTailOff) = 0;  // Note released
void notePressureChanged() = 0;        // MPE pressure (Stage 2.2: -> breathiness)
void notePitchbendChanged() = 0;       // Per-note pitchbend
void noteTimbreChanged() = 0;          // MPE timbre/CC74 (Stage 2.2: -> vowelY offset)
void noteKeyStateChanged() = 0;        // Sustain/sostenuto state change
void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) = 0;
```

**Protected members available:**
- `currentlyPlayingNote` -- MPENote with `.initialNote`, `.noteOnVelocity`, `.pressure`, `.pitchbend`, `.timbre`, `.getFrequencyInHertz()`
- `currentSampleRate` -- set by synth
- `clearCurrentNote()` -- must call when voice finishes

### enableLegacyMode()

```cpp
void enableLegacyMode(int pitchbendRange = 2, Range<int> channelRange = Range<int>(1, 17));
```

- Disables zone-based MPE, uses all 16 MIDI channels for polyphonic notes
- Pitchbend applies per-channel as note pitchbend
- Essential for DAW compatibility without MPE controllers

### Voice Stealing Algorithm

Built into MPESynthesiser -- steals intelligently:
1. Oldest voice in release phase (not bass/melody protected)
2. Oldest voice with finger off
3. Oldest unprotected voice
4. Bass/melody protected voices last

No need for custom voice stealing -- defaults are good.

---

## 2. Reference Patterns from Existing Plugins

### CMakeLists.txt Pattern (O-Bells/O-Prism)

Established structure:
```cmake
juce_add_plugin(O-Formant
    COMPANY_NAME "${OUARICON_COMPANY_NAME}"
    PLUGIN_MANUFACTURER_CODE ${OUARICON_MANUFACTURER_CODE}
    PLUGIN_CODE OuFm                    # Unique 4-char from CONTEXT.md
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "O-Formant${OUARICON_DEV_SUFFIX}"
    VERSION 1.0.0
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    NEEDS_WEB_BROWSER FALSE             # Stage 1: no WebView yet
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
)
```

**Stage 1 specifics:**
- No `NEEDS_WEBVIEW2` or WebView until Stage 3
- No binary data resources yet
- No module dependencies yet (`webview-relay-manager` added in Stage 3)
- `JUCE_WEB_BROWSER=0` until Stage 3

### APVTS Pattern (O-Prism)

Static `createParameterLayout()` function with section helpers:
```cpp
static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
```

Constructor initializer:
```cpp
OFormantProcessor()
    : AudioProcessor(BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      parameters(*this, nullptr, juce::Identifier("OFormantParameters"), createParameterLayout())
```

### Voice APVTS Pointer Pattern (O-Prism)

Each voice gets APVTS pointer and caches `getRawParameterValue()` pointers once:
```cpp
void setAPVTS(juce::AudioProcessorValueTreeState* apvts) {
    parameters = apvts;
    // Cache all 21 parameter pointers here
    pVowelX = apvts->getRawParameterValue("vowelX");
    pVowelY = apvts->getRawParameterValue("vowelY");
    // ... all 21
}
```

This eliminates hash map lookups in the audio thread -- critical pattern.

---

## 3. Parameter Layout Implementation

### 21 Parameters Organized by Section

**Vowel Morph (3):**
- `vowelX` -- Float 0.0-1.0, default 0.5, linear
- `vowelY` -- Float 0.0-1.0, default 0.5, linear
- `vowelFocus` -- Float 1.0-6.0, default 2.5, linear

**Glottal Source (5):**
- `glottalRd` -- Float 0.3-2.7, default 1.0, linear
- `breathiness` -- Float 0.0-1.0, default 0.1, linear
- `vibratoRate` -- Float 0.5-12.0, default 5.5, linear (Hz)
- `vibratoDepth` -- Float 0.0-100.0, default 15.0, linear (cents)
- `vibratoDelay` -- Float 0.0-2000.0, default 300.0, **skew 0.4** (ms)

**Consonant/Noise (4):**
- `consonantLevel` -- Float 0.0-1.0, default 0.3, linear
- `consonantTone` -- Float 0.0-1.0, default 0.5, linear
- `sibilance` -- Float 0.0-1.0, default 0.0, linear
- `autoConsonant` -- Bool, default off

**Envelope (4):**
- `attack` -- Float 0.001-5.0, default 0.01, **skew 0.3** (seconds)
- `decay` -- Float 0.001-5.0, default 0.3, **skew 0.3** (seconds)
- `sustain` -- Float 0.0-1.0, default 0.8, linear
- `release` -- Float 0.001-10.0, default 0.5, **skew 0.3** (seconds)

**Voice Character (3):**
- `formantShift` -- Float -24.0-24.0, default 0.0, linear (semitones)
- `formantSpread` -- Float 0.5-2.0, default 1.0, linear (multiplier)
- `pitchGlide` -- Float 0.0-1000.0, default 0.0, **skew 0.3** (ms)

**Output (2):**
- `outputGain` -- Float -60.0-12.0, default 0.0, linear (dB)
- `stereoWidth` -- Float 0.0-1.0, default 0.5, linear

### JUCE Parameter Types

- `juce::AudioParameterFloat` with `juce::ParameterID{"id", 1}` (version 1)
- `juce::NormalisableRange<float>(min, max, step, skew)` -- step 0.0f for continuous, skew != 1.0 for fast-skewed
- `juce::AudioParameterBool` for autoConsonant

---

## 4. File Structure for Stage 1

```
plugins/O-Formant/
  CMakeLists.txt
  Source/
    PluginProcessor.h
    PluginProcessor.cpp
    PluginEditor.h
    PluginEditor.cpp
    FormantVoice.h
    FormantVoice.cpp
```

**Minimal file count.** No DSP component files yet (those come in Stage 2). FormantVoice outputs silence but has the full MPESynthesiserVoice interface stubbed.

---

## 5. Pitfalls and Mitigations

### MPESynthesiser Has No addSound()

Unlike basic Synthesiser (which requires `addSound(new PrismSound())`), MPESynthesiser does NOT use SynthesiserSound. Just `addVoice()`. Do NOT create a FormantSound class.

### processBlock Must Forward MIDI to Synth

```cpp
void processBlock(AudioBuffer<float>& buffer, MidiBuffer& midi) {
    ScopedNoDenormals noDenormals;
    buffer.clear();
    synthesiser.renderNextBlock(buffer, midi, 0, buffer.getNumSamples());
}
```

MPESynthesiser parses MIDI internally -- no manual MIDI iteration needed (unlike O-Prism which extracts CC1/aftertouch manually).

### setCurrentPlaybackSampleRate() Required

Must call `synthesiser.setCurrentPlaybackSampleRate(sampleRate)` in `prepareToPlay()`. MPESynthesiser propagates this to all voices automatically.

### Output-Only Bus Configuration

Synth with no audio input:
```cpp
BusesProperties().withOutput("Output", juce::AudioChannelSet::stereo(), true)
```

### ParameterID Version Number

All parameters use version 1: `juce::ParameterID{"paramId", 1}`. This enables future parameter versioning for backwards compatibility.

### Generic Editor for Stage 1

Use `juce::GenericAudioProcessorEditor` as placeholder -- shows all parameters as sliders:
```cpp
juce::AudioProcessorEditor* createEditor() override {
    return new juce::GenericAudioProcessorEditor(*this);
}
```

Alternatively, a custom minimal editor that just sets window size and shows plugin name. The CONTEXT.md says "PluginEditor placeholder (generic or WebView scaffold)" -- generic is simpler for Stage 1.

### isBusesLayoutSupported

Synths must accept output-only stereo:
```cpp
bool isBusesLayoutSupported(const BusesLayout& layouts) const override {
    return layouts.getMainOutputChannelSet() == juce::AudioChannelSet::stereo();
}
```

---

## 6. Module Opportunities

**Stage 1: None required.** All functionality is standard JUCE.

**Stage 3 (future):**
- `webview-relay-manager` -- WebView parameter binding (via `ouaricon_add_module()`)
- `persistence/preset-manager` -- Preset system (Stage 4)

---

## 7. State Serialization

Use APVTS built-in XML serialization:
```cpp
void getStateInformation(MemoryBlock& destData) override {
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml(state.createXml());
    copyXmlToBinary(*xml, destData);
}

void setStateInformation(const void* data, int sizeInBytes) override {
    std::unique_ptr<juce::XmlElement> xml(getXmlFromBinary(data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName(parameters.state.getType()))
        parameters.replaceState(juce::ValueTree::fromXml(*xml));
}
```

Standard pattern from O-Prism/O-Bells. Ensures DAW session recall works from Stage 1.

---

## 8. Verification Strategy

1. **Build:** `ninja O-Formant_VST3 O-Formant_AU` -- zero warnings
2. **pluginval:** Basic scan passes
3. **DAW load:** Plugin appears as instrument, accepts MIDI
4. **Voice allocation:** MIDI notes trigger voice noteStarted() (verified via DBG log)
5. **Parameters:** All 21 visible in DAW automation
6. **State recall:** Save/reload DAW session preserves parameter values
