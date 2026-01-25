# OuariconLyrica - Implementation Plan

## Overview

**Plugin:** OuariconLyrica
**Type:** Physical Modeling Harp Synthesizer
**Complexity:** Advanced (Waveguide + Sympathetic Resonance)
**Estimated Stages:** 3 stages (Foundation → DSP → GUI)

---

## Stage 1: Foundation & Shell

**Goal:** Create buildable JUCE synth plugin with APVTS parameter infrastructure

### 1.1 Project Setup
- [ ] Create JUCE project via Projucer or CMake
- [ ] Configure as Synth plugin (with MIDI input)
- [ ] Set plugin ID: `Ouar` / `OLyr`
- [ ] Configure AU/VST3 formats

### 1.2 Parameter Infrastructure (APVTS)

**Macro Parameters:**
```cpp
// Core sound
{ "masterVolume", "Master Volume", -60.0f, 6.0f, 0.0f, "dB" },
{ "stringMaterial", "String Material", {"Gut", "Nylon", "Wire", "Carbon", "Metal Alloy", "Glass", "Crystal", "Energy"}, 1 },
{ "brightness", "Brightness", 0.0f, 1.0f, 0.5f },
{ "sustain", "Sustain", 0.0f, 1.0f, 0.7f },

// Body
{ "bodySize", "Body Size", 0.0f, 1.0f, 0.5f },
{ "bodyResonance", "Body Resonance", 0.0f, 1.0f, 0.6f },
{ "woodType", "Wood Type", {"Spruce", "Maple", "Exotic", "Synthetic"}, 0 },

// Sympathetic
{ "sympatheticAmount", "Sympathetic Resonance", 0.0f, 1.0f, 0.3f },

// Pluck mechanics
{ "pluckPosition", "Pluck Position", 0.0f, 1.0f, 0.5f },
{ "fingerHardness", "Finger Hardness", 0.0f, 1.0f, 0.5f },

// Expression
{ "technique", "Technique", {"Normal", "Harmonic", "Muted", "Près de la table"}, 0 },
{ "glissandoMode", "Glissando Mode", {"Off", "Free", "Scale-Locked"}, 0 },
{ "glissandoScale", "Glissando Scale", {"Major", "Minor", "Pentatonic", "Custom"}, 0 },

// Tuning
{ "masterTune", "Master Tune", 400.0f, 480.0f, 440.0f, "Hz" },
{ "pitchBendRange", "Pitch Bend Range", 1.0f, 48.0f, 2.0f, "st" },

// Advanced (hidden/expert)
{ "stringTension", "String Tension", 0.0f, 1.0f, 0.5f },
{ "stringGauge", "String Gauge", 0.0f, 1.0f, 0.5f },
{ "stringLength", "String Length", 0.0f, 1.0f, 0.5f },
{ "stringStiffness", "String Stiffness", 0.0f, 1.0f, 0.2f },
```

### 1.3 Synthesizer Framework
- [ ] Create `HarpSynthSound` class (accepts all MIDI notes)
- [ ] Create `HarpSynthVoice` class (placeholder process)
- [ ] Initialize `juce::Synthesiser` with 16 voices
- [ ] Wire MIDI input to synthesiser

### 1.4 Processor Shell
- [ ] `prepareToPlay()` - Initialize synth with sample rate
- [ ] `processBlock()` - Route to synthesiser
- [ ] `getStateInformation()` / `setStateInformation()`
- [ ] Report latency (0 samples initially)

### 1.5 Validation
- [ ] Plugin loads in DAW (Logic/Ableton/REAPER)
- [ ] Parameters visible in automation
- [ ] MIDI input produces silence (no audio yet)
- [ ] No crashes on preset recall

**Deliverable:** Buildable plugin shell with all parameters, voice framework

---

## Stage 2: DSP Implementation

**Goal:** Implement complete physical modeling engine

### Phase 2.1: Basic String Model (Karplus-Strong Baseline)

**Files:**
- `Source/DSP/StringVoice.h/.cpp`
- `Source/DSP/DelayLine.h/.cpp`

**Implementation:**
- [ ] Implement efficient delay line (power-of-2, Lagrange interpolation)
- [ ] Basic Karplus-Strong: delay + one-pole lowpass
- [ ] MIDI note → frequency → delay length
- [ ] Velocity → excitation amplitude
- [ ] Simple noise burst exciter

**Test:** Single notes play with basic plucked sound, correct pitch

### Phase 2.2: Bidirectional Waveguide

**Files:**
- `Source/DSP/WaveguideString.h/.cpp`
- `Source/DSP/BridgeFilter.h/.cpp`

**Implementation:**
- [ ] Bidirectional delay (upper/lower rails)
- [ ] Bridge filter (frequency-dependent reflection)
- [ ] Nut filter (inverted reflection)
- [ ] Loop damping filter (material-based)
- [ ] Connect to `stringMaterial`, `brightness`, `sustain` params

**Test:** Richer harmonics, material switching audible

### Phase 2.3: Pluck Exciter

**Files:**
- `Source/DSP/PluckExciter.h/.cpp`

**Implementation:**
- [ ] Filtered noise burst generator
- [ ] ADSR envelope for excitation
- [ ] Pluck position → comb filter effect
- [ ] Finger hardness → brightness filter
- [ ] Velocity sensitivity

**Test:** `pluckPosition` changes timbre, velocity dynamics work

### Phase 2.4: Stiffness & Dispersion

**Files:**
- `Source/DSP/StiffnessFilter.h/.cpp`

**Implementation:**
- [ ] Allpass cascade (2-4 stages) for inharmonicity
- [ ] Connect to `stringStiffness` parameter
- [ ] Per-note scaling (bass strings stiffer)

**Test:** Higher stiffness produces piano-like inharmonic tones

### Phase 2.5: String Material System

**Files:**
- `Source/DSP/StringMaterial.h/.cpp`

**Implementation:**
- [ ] Material presets with damping/brightness/stiffness values
- [ ] Material interpolation for morphing
- [ ] Connect all 8 material types
- [ ] Fantasy materials (Glass, Crystal, Energy)

**Test:** Each material sounds distinctly different

### Phase 2.6: Body Resonance

**Files:**
- `Source/DSP/BodyResonance.h/.cpp`
- `Source/Resources/HarpBodyIR.wav` (embedded binary)

**Implementation:**
- [ ] Convolution with embedded IR (50-100ms)
- [ ] Modal fallback (5 bandpass filters)
- [ ] `bodySize` scales frequencies
- [ ] `bodyResonance` controls wet/dry
- [ ] `woodType` selects IR preset

**Test:** Body adds acoustic depth, size changes low-end

### Phase 2.7: Sympathetic Resonance

**Files:**
- `Source/DSP/SympatheticResonance.h/.cpp`

**Implementation:**
- [ ] Track active voices and frequencies
- [ ] Compute harmonic relationships (unison, octave, fifth)
- [ ] Damped coupling based on frequency match
- [ ] Material-based coupling intensity
- [ ] `sympatheticAmount` parameter control

**Test:** Playing low notes causes high strings to shimmer

### Phase 2.8: Tuning Engine

**Files:**
- `Source/DSP/TuningEngine.h/.cpp`

**Implementation:**
- [ ] 12-TET base implementation
- [ ] MTS-ESP client integration (libMTSClient)
- [ ] Scala file loader (optional)
- [ ] Per-note pitch bend
- [ ] `masterTune` and `pitchBendRange` params

**Test:** External tuning source changes pitch, pitch wheel works

### Phase 2.9: Glissando Controller

**Files:**
- `Source/DSP/GlissandoController.h/.cpp`

**Implementation:**
- [ ] Free mode: SmoothedValue frequency ramp
- [ ] Scale-locked mode: step through scale degrees
- [ ] Speed parameter control
- [ ] Integrate with tuning engine for scale data

**Test:** Free glissando sweeps smoothly, scale-locked steps discretely

### Phase 2.10: Playing Techniques

**Implementation (in PluckExciter and Voice):**
- [ ] Normal: standard excitation
- [ ] Harmonic: filtered to isolate harmonic
- [ ] Muted: heavy damping, short release
- [ ] Près de la table: metallic/bright filter

**Test:** Technique keyswitch changes timbre appropriately

### Phase 2.11: Voice Management

**Implementation:**
- [ ] Voice stealing (quietest/oldest)
- [ ] Soft voice limit with graceful degradation
- [ ] Voice count meter output
- [ ] Quality preset switching

**Test:** 32+ notes polyphony without CPU overload

### Phase 2.12: Optimization Pass

**Implementation:**
- [ ] Profile hot paths (Instruments/perf tools)
- [ ] SIMD for filter processing where applicable
- [ ] Lookup tables for expensive functions
- [ ] Parameter smoothing audit (no clicks)

**Target:** <1% CPU per voice at High quality

**Deliverable:** Complete physical modeling harp engine

---

## Stage 3: GUI Implementation

**Goal:** WebView UI with parameter binding

### Phase 3.1: WebView Setup

**Files:**
- `Source/UI/WebViewEditor.h/.cpp`
- `Resources/ui/index.html`
- `Resources/ui/styles.css`
- `Resources/ui/app.js`

**Implementation:**
- [ ] JUCE WebView component setup
- [ ] Parameter relay system (C++ ↔ JS)
- [ ] Base HTML structure

### Phase 3.2: Main Interface Layout

**UI Sections:**
```
┌─────────────────────────────────────────────────┐
│  OUARICON LYRICA                    [Preset ▼] │
├─────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────┐ │
│  │   STRING    │  │    BODY     │  │ SYMPATH │ │
│  │  Material   │  │    Size     │  │ Amount  │ │
│  │  Brightness │  │  Resonance  │  │         │ │
│  │  Sustain    │  │  Wood Type  │  └─────────┘ │
│  └─────────────┘  └─────────────┘              │
├─────────────────────────────────────────────────┤
│  ┌─────────────────────┐  ┌───────────────────┐│
│  │   PLUCK MECHANICS   │  │    EXPRESSION     ││
│  │  Position  Hardness │  │ Technique  Gliss  ││
│  └─────────────────────┘  └───────────────────┘│
├─────────────────────────────────────────────────┤
│  [Master]  [Voices: 8/32]          [CPU: 12%]  │
└─────────────────────────────────────────────────┘
```

### Phase 3.3: Visual Feedback

**Implementation:**
- [ ] String visualization showing resonance
- [ ] Voice activity meter
- [ ] CPU usage display
- [ ] Material selector with visual icons

### Phase 3.4: Parameter Binding

**Implementation:**
- [ ] All macro parameters bound to UI controls
- [ ] Real-time value updates (both directions)
- [ ] Preset loading/saving
- [ ] Advanced panel for micro parameters

### Phase 3.5: Preset System

**Implementation:**
- [ ] Factory preset collection (8-12 presets)
- [ ] User preset save/load
- [ ] Preset browser UI

**Preset Ideas:**
- "Concert Grand Harp" - Traditional gut strings
- "Celtic Folk" - Nylon, bright
- "Ancient Lyre" - Wire, historical
- "Crystal Dreams" - Crystal, ethereal
- "Energy Strings" - Fantasy, experimental

### Phase 3.6: Polish

**Implementation:**
- [ ] Tooltips for all parameters
- [ ] Keyboard shortcuts
- [ ] Resize handling
- [ ] Accessibility considerations

**Deliverable:** Complete plugin with professional UI

---

## Validation Checkpoints

### After Stage 1
- [ ] Plugin loads in Logic Pro
- [ ] Plugin loads in Ableton Live
- [ ] Plugin loads in REAPER
- [ ] Parameters automate correctly
- [ ] State recall works

### After Stage 2
- [ ] Audio output matches design intent
- [ ] All materials sound distinct
- [ ] Sympathetic resonance audible
- [ ] CPU <50% at 32 voices
- [ ] No artifacts or clicks
- [ ] pluginval passes

### After Stage 3
- [ ] UI displays correctly
- [ ] All parameters controllable
- [ ] Presets load correctly
- [ ] Visual feedback responsive
- [ ] Final pluginval validation

---

## File Structure

```
plugins/OuariconLyrica/
├── CMakeLists.txt
├── OuariconLyrica.jucer (if using Projucer)
├── Source/
│   ├── PluginProcessor.h/.cpp
│   ├── PluginEditor.h/.cpp
│   ├── DSP/
│   │   ├── HarpSynthVoice.h/.cpp
│   │   ├── HarpSynthSound.h/.cpp
│   │   ├── WaveguideString.h/.cpp
│   │   ├── PluckExciter.h/.cpp
│   │   ├── BridgeFilter.h/.cpp
│   │   ├── StiffnessFilter.h/.cpp
│   │   ├── StringMaterial.h/.cpp
│   │   ├── BodyResonance.h/.cpp
│   │   ├── SympatheticResonance.h/.cpp
│   │   ├── TuningEngine.h/.cpp
│   │   └── GlissandoController.h/.cpp
│   └── UI/
│       └── WebViewEditor.h/.cpp
├── Resources/
│   ├── ui/
│   │   ├── index.html
│   │   ├── styles.css
│   │   └── app.js
│   └── ir/
│       └── HarpBody.wav
└── .contracts/
    ├── architecture.md
    └── plan.md
```

---

## Dependencies

### Required
- JUCE 7.x or 8.x
- CMake 3.22+
- C++17 compiler

### Optional
- libMTSClient (MTS-ESP support)
- Surge Tuning Library (Scala files)

---

## Risk Register

| Risk | Probability | Impact | Mitigation |
|------|-------------|--------|------------|
| Waveguide instability | Low | High | Clamp feedback, detect NaN |
| CPU overload | Medium | High | Voice stealing, quality presets |
| Sympathetic aliasing | Medium | Medium | Frequency tolerance tuning |
| IR latency | Low | Medium | Short IR, modal fallback |
| Parameter clicks | Low | Low | SmoothedValue audit |

---

## Success Criteria

1. **Sound Quality:** Harp tones comparable to Pianoteq
2. **Expression:** Meaningful response to velocity, position, material
3. **CPU Efficiency:** <1% per voice at High quality
4. **Polyphony:** 16-32 simultaneous voices
5. **Stability:** pluginval clean pass
6. **Usability:** Intuitive UI, good presets

---

## Ready for Implementation

This plan provides a complete roadmap from shell to finished plugin. Stage 1 creates the foundation, Stage 2 implements all DSP components with incremental testing, and Stage 3 delivers the user interface.

**Next Step:** Execute `/implement OuariconLyrica` to begin Stage 1.
