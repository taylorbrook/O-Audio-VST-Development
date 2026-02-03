# O-Bells Notes

## Status
- **Current Status:** 📦 Installed
- **Version:** 1.5.1
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

## Known Issues

None

## Description

O-Bells is a physical modeling bell synthesizer that creates realistic tubular bells, chimes, gongs, and other metallic percussion through modal synthesis. It features:

- **Modal synthesis** with configurable inharmonicity for authentic bell partials
- **Strike modeling** with position and mallet hardness controls
- **Material simulation** spanning bronze, steel, glass, and crystal
- **Ensemble section** with unison, detune, and octave layering
- **Built-in reverb** for spacious, ambient bell tones

## Parameters (23 total)

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
