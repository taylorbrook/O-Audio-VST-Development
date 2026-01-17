# OuariconLyrica - DSP Architecture

## Overview

**Plugin Type:** Physical Modeling Synthesizer (Synth)
**Core Algorithm:** Bidirectional Digital Waveguide Synthesis
**Target Polyphony:** 16-32 voices (flexible, CPU-dependent)
**CPU Budget:** <1% per voice at High quality, <50% total

OuariconLyrica is a physical modeling harp synthesizer using waveguide synthesis for plucked strings, with sympathetic resonance, convolution-based body modeling, and integration with the Ouaricon microtonal tuning system.

---

## Algorithm Selection Rationale

### Primary: Digital Waveguide Synthesis

**Why Waveguides (not Modal, not Karplus-Strong alone):**

| Approach | Realism | CPU | Flexibility | Decision |
|----------|---------|-----|-------------|----------|
| Karplus-Strong | Medium | Very Low | Low | Too simplistic for harp nuance |
| Modal Synthesis | High | Medium | Medium | Better for bars/bells than strings |
| **Bidirectional Waveguide** | **Very High** | **Low-Medium** | **High** | **Best for plucked strings** |
| Finite Difference | Excellent | Very High | Very High | Overkill, too CPU-intensive |

Waveguides model true wave propagation physics, naturally supporting:
- Pluck position affecting timbre (comb filter effect)
- Damping that evolves over time
- Stiffness via allpass dispersion
- Coupled string resonance

### Secondary: Convolution Body Resonance

Short impulse response (50-100ms) from recorded harp body provides authentic acoustic coloration with minimal CPU overhead (~0.2%).

---

## Signal Flow Architecture

```
MIDI Note-On
    │
    ▼
┌─────────────────┐
│  Tuning Engine  │◄── MTS-ESP / Scala / 12-TET
└────────┬────────┘
         │ (fundamental frequency)
         ▼
┌─────────────────┐
│  Pluck Exciter  │◄── Velocity, Position, Material, Technique
│  (Noise + Env)  │
└────────┬────────┘
         │ (excitation signal)
         ▼
┌─────────────────────────────────────────────┐
│           Waveguide String Model            │
│  ┌─────────────────────────────────────┐    │
│  │     Upper Rail (Right-traveling)    │    │
│  │  ════════════════════════════════►  │    │
│  │         Delay Line + Filter         │    │
│  └─────────────────────────────────────┘    │
│                    ↑↓ (Bridge/Nut reflections)
│  ┌─────────────────────────────────────┐    │
│  │     Lower Rail (Left-traveling)     │    │
│  │  ◄════════════════════════════════  │    │
│  │         Delay Line + Filter         │    │
│  └─────────────────────────────────────┘    │
│                                             │
│  Components:                                │
│  • Bridge Filter (frequency-dependent)      │
│  • Nut Filter (inverted reflection)         │
│  • Loop Damping (material-based)            │
│  • Stiffness Allpass (dispersion)           │
└────────────────┬────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────┐
│       Sympathetic Resonance Engine          │
│  (Couples output with other active voices)  │
│  • Frequency-domain coupling detection      │
│  • Material-based coupling intensity        │
│  • Harmonic relationship weighting          │
└────────────────┬────────────────────────────┘
                 │
                 ▼
┌─────────────────────────────────────────────┐
│      Body Resonance (Convolution IR)        │
│  • 50-100ms harp body impulse response      │
│  • Wet/dry mix (body amount parameter)      │
│  • Optional: Modal fallback if no IR        │
└────────────────┬────────────────────────────┘
                 │
                 ▼
             [Output]
```

---

## Component Specifications

### 1. Tuning Engine

**Purpose:** Converts MIDI note to frequency using microtonal tuning system

**Integration:**
- MTS-ESP client for real-time tuning from external sources
- Scala file loader for .scl/.kbm tuning definitions
- 12-TET fallback when no tuning loaded
- Per-note pitch bend (±48 semitones range)

**Interface:**
```cpp
class TuningEngine {
public:
    double getFrequency(int midiNote, int midiChannel);
    void setPitchBend(int midiNote, float bendAmount); // ±1.0
    void loadScalaFile(const juce::File& scl, const juce::File& kbm);
    bool connectMTSClient();

    // For scale-locked glissando
    std::vector<double> getScaleFrequencies(int rootNote, int numNotes);
};
```

**CPU Impact:** Negligible (<0.01%)

---

### 2. Pluck Exciter

**Purpose:** Generates excitation signal that triggers string vibration

**Algorithm:**
- Noise burst filtered by pluck position (comb filter)
- ADSR envelope shapes the impulse
- Finger hardness affects spectral content
- Velocity scales amplitude and brightness

**Parameters:**
| Parameter | Range | Default | Effect |
|-----------|-------|---------|--------|
| pluckPosition | 0.0-1.0 | 0.5 | Affects harmonic content (comb filter null) |
| pluckVelocity | 0.0-1.0 | 0.7 | Initial energy |
| fingerHardness | 0.0-1.0 | 0.5 | Soft = filtered, Hard = bright transient |

**Playing Techniques (affects exciter):**
| Technique | Modification |
|-----------|--------------|
| Normal | Standard noise burst |
| Harmonic | Touch at node point, filter to isolate harmonic |
| Muted | Heavy damping, short decay |
| Près de la table | Close to soundboard, metallic/bright filter |

**Interface:**
```cpp
class PluckExciter {
public:
    void trigger(float velocity, float position, float hardness);
    void setTechnique(PlayingTechnique technique);
    float process(); // Returns excitation sample

private:
    juce::ADSR envelope;
    juce::Random noiseSource;
    juce::dsp::IIR::Filter<float> positionFilter;
    PlayingTechnique currentTechnique = PlayingTechnique::Normal;
};
```

**CPU Impact:** ~0.02% per voice

---

### 3. Waveguide String Model

**Purpose:** Core physical model of vibrating string

**Algorithm: Bidirectional Digital Waveguide**

```
Excitation Input
      │
      ▼
   ┌──┴──┐
   │ (+) │ ← Excitation injection point (at pluck position)
   └──┬──┘
      │
  ┌───┴───────────────────────────────────────────┐
  │   ═══════════════════════════════════════►    │
  │   Upper Rail (Delay Line + Interpolation)     │
  │                                               │
  │   ◄═══════════════════════════════════════    │
  │   Lower Rail (Delay Line + Interpolation)     │
  └───────────────────────────────────────────────┘
          │                              │
          ▼                              ▼
    ┌───────────┐                  ┌───────────┐
    │ Nut Filter│                  │Bridge Filt│
    │ (Reflect) │                  │ (Damping) │
    └─────┬─────┘                  └─────┬─────┘
          │                              │
          └──────────────────────────────┘
                         │
                    ┌────┴────┐
                    │Stiffness│
                    │ Allpass │
                    └────┬────┘
                         │
                    [String Output]
```

**Delay Line Specifications:**
- Max delay: 88200 samples (2 seconds at 44.1kHz)
- Interpolation: Lagrange 3rd order (accurate pitch)
- Delay length = sampleRate / frequency

**Filter Specifications:**

| Filter | Type | Purpose | Cutoff Control |
|--------|------|---------|----------------|
| Bridge | 1-pole lowpass | Frequency-dependent reflection | stringBrightness |
| Nut | Inverter + lowpass | Reflection with sign inversion | Fixed |
| Loop Damping | 1-pole lowpass | Energy loss per cycle | stringDamping |
| Stiffness | Allpass cascade | Inharmonicity/dispersion | stringStiffness |

**Interface:**
```cpp
class WaveguideString {
public:
    void prepare(double sampleRate, int maxBlockSize);
    void setFrequency(double frequency);
    void setMaterial(const StringMaterial& material);
    void setPluckPosition(float position);

    float process(float excitation);

private:
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> upperRail;
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> lowerRail;

    juce::dsp::IIR::Filter<float> bridgeFilter;
    juce::dsp::IIR::Filter<float> nutFilter;
    juce::dsp::IIR::Filter<float> loopDamping;
    std::array<juce::dsp::IIR::Filter<float>, 4> stiffnessAllpass;

    double currentFrequency = 440.0;
    float pluckPosition = 0.5f;
    float feedback = 0.995f;
};
```

**CPU Impact:** ~0.3% per voice

---

### 4. String Material System

**Purpose:** Defines timbral characteristics based on physical material

**Material Definitions:**

| Material | Damping | Brightness | Stiffness | Coupling | Character |
|----------|---------|-----------|-----------|----------|-----------|
| Gut | 0.40 | 3000 Hz | 0.10 | 0.80 | Warm, mellow, blended |
| Nylon | 0.25 | 5000 Hz | 0.20 | 0.50 | Balanced, modern |
| Wire | 0.10 | 8000 Hz | 0.05 | 0.20 | Bright, punchy, defined |
| Carbon | 0.08 | 10000 Hz | 0.30 | 0.15 | Bright, sustained, piercing |
| Metal Alloy | 0.05 | 12000 Hz | 0.15 | 0.10 | Metallic, ethereal, ringing |
| Glass | 0.20 | 9000 Hz | 0.60 | 0.70 | Crystalline, bell-like |
| Crystal | 0.03 | 14000 Hz | 0.50 | 0.85 | Pure, sustained, magical |
| Energy | Variable | Variable | Variable | Variable | Impossible, synthetic |

**Interface:**
```cpp
struct StringMaterial {
    float dampingCoeff;        // 0.0-1.0 (higher = faster decay)
    float brightnessCutoff;    // Hz for damping filter
    float stiffnessAmount;     // Allpass dispersion amount 0.0-1.0
    float sympatheticCoupling; // How much resonates with other strings
    float noiseContent;        // Attack noise amount
    juce::String name;

    static StringMaterial fromType(MaterialType type);
    StringMaterial interpolate(const StringMaterial& other, float t);
};
```

**CPU Impact:** Negligible (lookup only)

---

### 5. Sympathetic Resonance Engine

**Purpose:** Models acoustic coupling between strings

**Algorithm:**
1. Track all active voices and their fundamental frequencies
2. For each voice, identify harmonically related frequencies:
   - Unison (1/1)
   - Octave (1/2, 2/1)
   - Fifth (2/3, 3/2)
   - Third (4/5, 5/4)
3. Compute coupling intensity based on:
   - Frequency proximity (within tolerance)
   - Material coupling coefficient
   - User intensity parameter
4. Add damped version of related strings' outputs

**Coupling Formula:**
```
coupledOutput = sum(relatedStrings * frequencyMatch * materialCoupling * intensityParam * decay)
```

**Interface:**
```cpp
class SympatheticResonanceEngine {
public:
    void registerVoice(int voiceId, double frequency, const StringMaterial& material);
    void unregisterVoice(int voiceId);

    void setIntensity(float intensity); // 0.0-1.0

    // Called each sample for each active voice
    float computeSympatheticContribution(int voiceId, float voiceOutput);

private:
    struct VoiceInfo {
        double frequency;
        float materialCoupling;
        juce::dsp::DelayLine<float> resonatorDelay;
        juce::dsp::IIR::Filter<float> resonatorFilter;
    };

    std::map<int, VoiceInfo> activeVoices;
    float intensity = 0.3f;

    float computeCouplingStrength(double freq1, double freq2);
};
```

**CPU Impact:** ~0.1-0.2% per voice (depends on voice count)

---

### 6. Body Resonance Module

**Purpose:** Adds acoustic character of harp soundboard

**Primary Method: Convolution**
- Embed pre-recorded impulse response in plugin binary
- 50-100ms length (2205-4410 samples at 44.1kHz)
- Minimal latency, authentic sound

**Fallback: Modal Synthesis**
- 5 resonant bandpass filters modeling body modes
- Frequencies: 300, 400, 600, 900, 1200 Hz (concert harp)
- User-adjustable body size scales frequencies

**Body Parameters:**
| Parameter | Range | Default | Effect |
|-----------|-------|---------|--------|
| soundboardSize | 0.0-1.0 | 0.5 | Scales resonance frequencies |
| woodType | Choice | Spruce | Selects IR or mode preset |
| bodyResonance | 0.0-1.0 | 0.6 | Wet/dry mix |

**Interface:**
```cpp
class BodyResonance {
public:
    void prepare(double sampleRate, int maxBlockSize);
    void loadImpulseResponse(const void* data, size_t size);
    void setBodyParameters(float size, WoodType type, float amount);

    float process(float input);

private:
    juce::dsp::Convolution convolution;

    // Fallback modal synthesis
    std::array<juce::dsp::IIR::Filter<float>, 5> bodyModes;
    std::array<float, 5> modeAmplitudes;

    float bodyAmount = 0.6f;
    bool useConvolution = true;
};
```

**CPU Impact:** ~0.1% (convolution) or ~0.05% (modal)

---

### 7. Glissando Controller

**Purpose:** Implements smooth and scale-locked pitch sweeps

**Modes:**
| Mode | Behavior |
|------|----------|
| Off | Normal discrete notes |
| Free | Continuous pitch sweep between notes |
| Scale-Locked | Steps through scale degrees from tuning system |

**Interface:**
```cpp
class GlissandoController {
public:
    void setMode(GlissandoMode mode);
    void setScale(const std::vector<double>& scaleFrequencies);
    void setSpeed(float speed); // Notes per second for scale-locked

    void startGlissando(double startFreq, double endFreq);
    double getNextFrequency(); // Per-sample query
    bool isActive() const;

private:
    GlissandoMode mode = GlissandoMode::Off;
    juce::SmoothedValue<double> frequencyRamp;
    std::vector<double> scale;
    int currentScaleDegree = 0;
    float speed = 10.0f; // notes per second
};
```

**CPU Impact:** Negligible

---

## Voice Architecture

### Voice State Machine

```
┌─────────────┐
│    Idle     │ ◄─────────────────┐
└──────┬──────┘                   │
       │ Note On                  │
       ▼                          │
┌─────────────┐                   │
│   Attack    │ (Exciter active)  │
└──────┬──────┘                   │
       │ Exciter done             │
       ▼                          │
┌─────────────┐                   │
│   Sustain   │ (Waveguide ring)  │
└──────┬──────┘                   │
       │ Note Off / Mute          │
       ▼                          │
┌─────────────┐                   │
│   Release   │ (Fast decay)      │
└──────┬──────┘                   │
       │ Amplitude < threshold    │
       ▼                          │
       └──────────────────────────┘
```

### Voice Class

```cpp
class HarpVoice : public juce::SynthesiserVoice {
public:
    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNote, float velocity,
                   juce::SynthesiserSound* sound,
                   int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newValue) override;
    void controllerMoved(int controllerNumber, int newValue) override;
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                        int startSample, int numSamples) override;

private:
    PluckExciter exciter;
    WaveguideString string;
    GlissandoController glissando;

    double currentFrequency = 440.0;
    float currentVelocity = 0.0f;
    bool isPlaying = false;

    // Reference to shared sympathetic engine (in processor)
    SympatheticResonanceEngine* sympatheticEngine = nullptr;
};
```

---

## Parameter Hierarchy

### Macro Parameters (User-facing)

| Parameter | Range | Description |
|-----------|-------|-------------|
| Master Volume | -inf to +6 dB | Output level |
| String Material | Choice | Gut/Nylon/Wire/etc |
| Body Size | 0-100% | Soundboard resonance |
| Brightness | 0-100% | Overall tone |
| Sustain | 0-100% | Decay time |
| Sympathetic | 0-100% | Resonance intensity |
| Glissando Mode | Off/Free/Scale | Pitch behavior |
| Tuning Source | 12-TET/MTS-ESP/Scala | Tuning system |

### Micro Parameters (Advanced tab)

| Parameter | Range | Maps To |
|-----------|-------|---------|
| String Tension | 0-100% | Loop filter feedback |
| String Gauge | 0-100% | Damping characteristics |
| String Length | 0-100% | Delay line scaling |
| Pluck Position | 0-100% | Excitation comb filter |
| Finger Hardness | 0-100% | Excitation filter |
| Stiffness | 0-100% | Allpass dispersion |
| Body Resonance | 0-100% | IR wet/dry |
| Wood Type | Choice | IR selection |

### Technical Parameters (Hidden/Internal)

- delayLineSamples (calculated from frequency)
- bridgeFilterCoeffs
- loopDampingCoeffs
- stiffnessAllpassCoeffs

---

## Performance Targets

### CPU Budget per Voice

| Component | Draft | Standard | High |
|-----------|-------|----------|------|
| Tuning Engine | 0.01% | 0.01% | 0.01% |
| Pluck Exciter | 0.02% | 0.02% | 0.02% |
| Waveguide | 0.15% | 0.25% | 0.30% |
| Stiffness Allpass | 0.00% | 0.05% | 0.10% |
| Sympathetic | 0.00% | 0.10% | 0.20% |
| Body (Modal) | 0.03% | 0.03% | - |
| Body (IR) | - | - | 0.10% |
| **Total/Voice** | **0.21%** | **0.46%** | **0.73%** |

### Voice Count Targets

| Quality | Max Voices | Total CPU |
|---------|------------|-----------|
| Draft | 64+ | <15% |
| Standard | 32 | <15% |
| High | 16 | <12% |

### Latency

| Component | Latency |
|-----------|---------|
| Waveguide | 0 samples |
| Body IR | 50-100 samples |
| Internal | <2ms total |

---

## JUCE Components Used

### DSP Modules
- `juce::dsp::DelayLine` - Waveguide delay lines
- `juce::dsp::IIR::Filter` - All filtering
- `juce::dsp::Convolution` - Body IR
- `juce::dsp::ProcessSpec` - Prepare context

### Synthesizer Framework
- `juce::Synthesiser` - Voice management
- `juce::SynthesiserVoice` - Per-voice processing
- `juce::SynthesiserSound` - Sound template
- `juce::ADSR` - Exciter envelope

### Parameters
- `juce::AudioProcessorValueTreeState` - Parameter management
- `juce::SmoothedValue` - Click-free parameter changes
- `juce::NormalisableRange` - Parameter scaling

---

## Implementation Dependencies

### External Libraries
- **libMTSClient** - MTS-ESP tuning integration
- **Surge Tuning Library** - Scala file parsing (optional)

### Internal Ouaricon Modules
- **Tuning Module** - Shared microtonal infrastructure
- **FX Module System** - Post-processing effects (if applicable)

### Embedded Resources
- Harp body impulse responses (BinaryData)
- Default preset collection

---

## Risk Mitigation

### Potential Issues

| Risk | Likelihood | Mitigation |
|------|------------|------------|
| CPU overload at high polyphony | Medium | Voice stealing, quality presets |
| Waveguide instability | Low | Clamp feedback, detect NaN |
| IR latency too high | Low | Use 50ms IR, offer modal fallback |
| Sympathetic resonance artifacts | Medium | Damping, frequency tolerance tuning |
| Parameter clicks | Low | SmoothedValue everywhere |

### Fallback Strategies

1. If convolution IR problematic → Use modal body synthesis
2. If sympathetic too CPU-heavy → Offer "Simple" mode (disabled)
3. If waveguide unstable → Fall back to Karplus-Strong
4. If MTS-ESP unavailable → 12-TET with manual tuning table

---

## Testing Requirements

### Audio Quality Tests
- Pitch accuracy across all notes (within 0.5 Hz)
- Smooth decay (exponential, no artifacts)
- No clicks on parameter changes
- No DC offset in output

### CPU Performance Tests
- Single voice: <1% at High quality
- 16 voices: <12% at High quality
- 32 voices: <25% at Standard quality
- No dropouts at 256 sample buffer

### Stability Tests
- No NaN/Inf output under any parameters
- Graceful handling of extreme values
- No memory leaks over extended sessions

---

## Revision History

| Version | Date | Changes |
|---------|------|---------|
| 1.0 | 2026-01-16 | Initial architecture based on research |
