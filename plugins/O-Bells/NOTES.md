# O-Bells Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 3.1.1
- **Type:** Synth (Physical Modeling Bells)

## Lifecycle Timeline

- **2026-02-02:** Initial development completed (v1.0.0)
- **2026-02-02 (v1.0.0):** Physical modeling bell synthesizer with 18 parameters, WebView UI, 25 factory presets
- **2026-02-02 (v1.1.0):** Added reverb control for spaciousness (MINOR feature addition)
- **2026-02-02 (v1.1.1):** Fixed output clipping with proper DSP gain staging normalization
- **2026-02-02 (v1.2.0):** Implemented proper multi-stage decay envelope with 4 new parameters
- **2026-02-03 (v1.3.0):** Attack parameter, shimmer quality, material differentiation, bloom fix
- **2026-02-03 (v1.4.0):** Bloom split into Speed + Amount controls (BREAKING - presets need resave)
- **2026-02-03 (v1.4.1):** Expanded bloom speed ranges for more dramatic effects
- **2026-02-03 (v1.5.0):** Bloom Fine Controls - per-band independent speed/amount (6 new params)
- **2026-02-03 (v1.5.1):** Bloom speed readouts now display milliseconds instead of percentages
- **2026-02-03 (v1.5.2):** Attack Amount slider added to UI (was missing), parameter renamed for clarity
- **2026-02-03 (v1.5.3):** Fixed Strike parameter producing no sound at 0%/100% extremes
- **2026-02-03 (v1.5.4):** UI reorganization - Onsets section consolidates strike/transient controls
- **2026-02-03 (v1.6.0):** Complete factory preset redesign - 25 research-informed presets with new names
- **2026-02-03 (v1.6.1):** Brightness parameter range expanded [0.1, 2.0] for wider tonal control
- **2026-02-03 (v2.0.0):** BREAKING - Split brightness into Overtone + Acoustic brightness (31 params)
- **2026-02-03 (v2.1.0):** Air Absorption parameter - time-varying lowpass filter for acoustic realism (32 params)
- **2026-02-03 (v2.2.0):** GUI keyboard in footer panel with Gain slider relocated
- **2026-02-03 (v2.2.1):** Complete factory preset redesign with descriptive names
- **2026-02-03 (v2.3.0):** 16-voice polyphony (increased from 8 voices)
- **2026-02-04 (v2.4.0):** Humanize parameter for per-note organic variation (strike, mallet, decay, attack, inharmonicity)
- **2026-02-05 (v3.1.0):** TrueKeys interval reporting - real-time frequency-based interval display with note names and interval labels (ported from O-Lyrica)

## Known Issues

None

## Description

O-Bells is a physical modeling bell synthesizer that creates realistic tubular bells, chimes, gongs, and other metallic percussion through modal synthesis. It features:

- **Modal synthesis** with configurable inharmonicity for authentic bell partials
- **Strike modeling** with position and mallet hardness controls
- **Material simulation** spanning bronze, steel, glass, and crystal
- **Ensemble section** with unison, detune, and octave layering
- **Built-in reverb** for spacious, ambient bell tones

## Parameters (33 total)

### Synthesis (6)
- Strike Position (0-100%) - Center to edge strike point
- Mallet Hardness (0-100%) - Soft to hard striker
- Damping (0-100%) - Hand-damped to free-ring
- Brightness (0-100%) - Dark to brilliant tone
- Material (0-100%) - Bronze → Steel → Glass → Crystal
- Inharmonicity (0-100%) - Pure harmonic to gamelan-style

### Ensemble (5)
- Unison Count (1-4) - Number of detuned copies
- Unison Detune (0-50 cents) - Detune spread
- Octave Blend Sub (0-100%) - Sub-octave layer
- Octave Blend Oct (0-100%) - Upper octave layer
- Stereo Spread (0-100%) - Ensemble panning width

### Character (3 choices)
- Strike Noise: Click / Thud / Ping
- Velocity Curve: Linear / Exponential / Logarithmic
- Decay Shape: Linear / Exponential / Multi-stage

### Advanced (5)
- Partial Tuning (-100 to +100 cents) - Minor-third partial adjustment
- Pitch Envelope (0-100%) - Initial pitch drop
- Pitch Env Time (5-200ms) - Pitch envelope return time
- Nonlinear Effects (0-100%) - Bell warping/distortion
- Humanize (0-100%) - Per-note organic variation [v2.4.0]

### Multi-Stage Envelope (4) - visible when Decay Shape = Multi-stage [v1.2.0]
- Strike Time (5-100ms) - Duration of bright metallic transient
- Brilliance (0-100%) - High-frequency sustain (0=warm, 100=bright)
- Body Time (100-5000ms) - Duration of main tonal decay
- Hum Sustain (0-100%) - Extension of low partial sustain

### Output (3)
- Reverb Mix (0-100%) - Spaciousness control [v1.1.0]
- Output Gain (-24 to +12 dB) - Master level
- Level Meter - Real-time stereo output metering

## Factory Presets (25)

- **Orchestral:** Tubular Bells, Concert Chimes, Glockenspiel, Celesta Mallet, Vibraphone
- **Sacred:** Church Bell, Cathedral Carillon, Meditation Bowl, Temple Gong, Singing Bowl
- **World:** Gamelan Saron, Gamelan Bonang, Tibetan Bowl, Steel Pan, Kalimba Bell
- **Ambient:** Frozen Shimmer, Bell Pad, Crystal Drone, Ethereal Chime, Submerged Bells
- **Cinematic:** Epic Bell, Tension Chime, Horror Stinger, Dramatic Swell, Distant Thunder

## Technical Details

- **DSP:** Modal synthesis with velocity-sensitive excitation
- **Reverb:** JUCE dsp::Reverb with bell-optimized settings (roomSize 0.7, damping 0.4)
- **UI:** WebView-based with botanical aesthetic theme
- **Formats:** VST3, AU
- **Installation:** ~/Library/Audio/Plug-Ins/VST3/ and ~/Library/Audio/Plug-Ins/Components/

## Improvement History

### v1.1.0 (2026-02-02)
**Request:** Add reverb control to make bells sound more spacious, positioned left of gain slider

**Implementation:**
- Added `reverbMix` parameter to APVTS (ID: "reverbMix")
- Implemented `juce::dsp::Reverb` in processBlock after synthesiser rendering
- Created WebSliderRelay/Attachment for WebView binding
- Added UI slider in Output section before Gain
- Optimized reverb settings for metallic resonance

**Files Modified:**
- PluginProcessor.h/cpp - DSP and parameter
- PluginEditor.h/cpp - Relay and attachment
- Resources/ui/index.html - UI element and JS binding

**Validation:** auval PASS, pluginval SUCCESS (strictness 5)

### v1.1.1 (2026-02-02)
**Request:** Default output level too loud, causes clipping

**Root Cause:** Signal exceeded 0 dBFS due to:
- 8 partials summing to ~2.7x amplitude (harmonic series)
- Unison voices stacking
- Octave layers (sub + upper) adding signal without normalization

**Fix:** Added proper gain staging normalization in BellVoice::renderNextBlock():
- Partial normalization: 0.4x factor to compensate for partial summing
- Unison normalization: 1/sqrt(unisonCount) - already existed
- Layer normalization: 1/(1 + subBlend + octBlend) for octave layers
- Output gain at 0 dB now produces unity gain as expected

**Files Modified:**
- BellVoice.cpp - renderNextBlock() gain staging

**Validation:** pluginval SUCCESS, DAW testing confirmed

### v1.2.0 (2026-02-02)
**Request:** Implement proper multi-stage decay envelope (previously a placeholder)

**Research-Based Implementation:**
- Comprehensive research from CCRMA, IRCAM, Arturia Pigments, AAS Chromaphone
- Academic formula: R_k = b₁ + b₃×f_k² for frequency-dependent damping
- Industry-standard parameter naming (Brilliance, Strike Time, Body Time, Hum Sustain)

**New Parameters (4):**
- **Strike Time** (5-100ms): Duration of bright metallic transient
- **Brilliance** (0-100%): High-frequency sustain (0=warm/woody, 100=bright/glassy)
- **Body Time** (0.1-5s): Duration of main tonal decay phase
- **Hum Sustain** (0-100%): Extension of low partial sustain

**Key Features:**
- Physics-based three-phase envelope (Strike → Body → Hum)
- Per-partial decay coefficients pre-calculated for performance
- Damping parameter affects only Hum phase (user requirement)
- New "Envelope" UI section appears only when Multi-stage is selected

**Files Modified:**
- PluginProcessor.h/cpp - 4 new APVTS parameters
- BellVoice.h/cpp - Multi-stage envelope DSP implementation
- PluginEditor.h/cpp - 4 new WebSlider relays and attachments
- Resources/ui/index.html - New Envelope section with show/hide logic

**Validation:** pluginval SUCCESS (strictness 5), auval PASS

### v2.2.0 (2026-02-03)
**Request:** Add GUI keyboard to footer panel, move Gain slider to footer

**Implementation:**
- Expanded footer from 40px to 55px height
- Added 2-octave interactive keyboard (C3-B4) with QWERTY support
- Moved Gain slider from Output section to footer
- Added `sendMidiNote` native function for keyboard → synth communication
- Added `triggerNoteOn`/`triggerNoteOff` methods to PluginProcessor

**Files Modified:**
- PluginProcessor.h/cpp - Added note trigger methods
- PluginEditor.cpp - Added sendMidiNote native function
- Resources/ui/index.html - Footer expansion, keyboard CSS/JS

**Validation:** Manual DAW testing confirmed

---

## Footer Module Integration Notes (v2.2.0 Lessons Learned)

When integrating the `instrument-footer-panel` module (from `modules/ui/instrument-footer-panel/`) into a plugin, **DO NOT** use the module's JS file directly. Instead, follow this surgical approach:

### What Works

1. **Add CSS inline** - Copy only the CSS styles you need into the plugin's `<style>` block. Don't replace existing footer CSS entirely.

2. **Modify existing footer HTML** - Expand the existing `<div class="footer">` to include:
   - Gain slider (with same `data-param` binding pattern as other sliders)
   - Keyboard container `<div class="footer-keyboard-viz" id="keyboard-viz">`
   - Branding text

3. **Add keyboard JS after existing bindings** - The keyboard JS must come AFTER the slider parameter binding code so `parameterStates.get('outputGain')` is available.

4. **Adjust tab-content height** - Change `calc(100% - 130px)` to `calc(100% - 145px)` for 55px footer.

### What Breaks

1. **Replacing footer CSS entirely** - This can accidentally remove styles for expandable sections, bloom fine controls, etc. that share CSS with the footer area.

2. **Using the module's standalone JS** - The module expects its own initialization pattern. Instead, inline the keyboard-building code and use the plugin's existing JUCE binding pattern.

3. **Reordering HTML sections** - Moving the Output section or changing its structure can break meter bindings.

### C++ Requirements

Add to PluginProcessor.h:
```cpp
void triggerNoteOn(int midiNote, float velocity);
void triggerNoteOff(int midiNote);
```

Add to PluginProcessor.cpp:
```cpp
void MyProcessor::triggerNoteOn(int midiNote, float velocity) {
    midiNote = juce::jlimit(0, 127, midiNote);
    velocity = juce::jlimit(0.0f, 1.0f, velocity);
    synthesiser.noteOn(1, midiNote, velocity);
}

void MyProcessor::triggerNoteOff(int midiNote) {
    midiNote = juce::jlimit(0, 127, midiNote);
    synthesiser.noteOff(1, midiNote, 0.0f, true);
}
```

Add native function in PluginEditor.cpp WebView options:
```cpp
.withNativeFunction("sendMidiNote", [this](const juce::Array<juce::var>& args,
                                            std::function<void(juce::var)> complete) {
    if (args.size() >= 3) {
        int midiNote = static_cast<int>(args[0]);
        float velocity = static_cast<float>(args[1]);
        bool isNoteOn = static_cast<bool>(args[2]);
        if (isNoteOn) processorRef.triggerNoteOn(midiNote, velocity);
        else processorRef.triggerNoteOff(midiNote);
    }
    complete({});
})
