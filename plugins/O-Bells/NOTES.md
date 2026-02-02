# O-Bells Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.1.0
- **Type:** Synth (Physical Modeling Bells)

## Lifecycle Timeline

- **2026-02-02:** Initial development completed (v1.0.0)
- **2026-02-02 (v1.0.0):** Physical modeling bell synthesizer with 18 parameters, WebView UI, 25 factory presets
- **2026-02-02 (v1.1.0):** Added reverb control for spaciousness (MINOR feature addition)

## Known Issues

None

## Description

O-Bells is a physical modeling bell synthesizer that creates realistic tubular bells, chimes, gongs, and other metallic percussion through modal synthesis. It features:

- **Modal synthesis** with configurable inharmonicity for authentic bell partials
- **Strike modeling** with position and mallet hardness controls
- **Material simulation** spanning bronze, steel, glass, and crystal
- **Ensemble section** with unison, detune, and octave layering
- **Built-in reverb** for spacious, ambient bell tones

## Parameters (19 total)

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

### Advanced (4)
- Partial Tuning (-100 to +100 cents) - Minor-third partial adjustment
- Pitch Envelope (0-100%) - Initial pitch drop
- Pitch Env Time (5-200ms) - Pitch envelope return time
- Nonlinear Effects (0-100%) - Bell warping/distortion

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
