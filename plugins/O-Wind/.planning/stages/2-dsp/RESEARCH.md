# Stage 2: DSP Implementation - Research

**Researched:** 2026-04-05
**Domain:** JUCE 8.0.4 DSP for jet-drive waveguide flute physical modeling
**Confidence:** HIGH

## Summary

This research covers the complete JUCE 8.0.4 API surface and implementation patterns needed to build O-Wind's DSP engine -- a jet-drive waveguide flute synthesizer implementing the Verge (1995) model. All JUCE APIs were verified directly from the JUCE 8.0.4 source headers installed at `/Users/taylorbrook/JUCE`.

The architecture is simpler than O-Bowed: one-directional jet exciter (no iterative friction solver), bore waveguide IS the body (no separate body resonator), and the one-sample feedback delay is naturally provided by the jet delay. The O-Bowed codebase provides a proven reference for `DelayLine<Thiran>` usage, `SynthesiserVoice` patterns, and per-sample waveguide processing.

**Primary recommendation:** Follow the O-Bowed split-DSP-component pattern exactly. Start Phase 3.1 with the minimal feedback loop (jet + bore + reflection) to validate self-oscillation before adding expression controls.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- All 4 DSP phases included (3.1-3.4)
- Phase 3.4 scope: Include all (Tier 2 Keefe tone holes + expansion presets) -- user decision
- Voice architecture: `juce::Synthesiser` + `juce::SynthesiserVoice` with manual CC routing
- Oversampling: Per-voice `juce::dsp::Oversampling<float>` instances
- File structure: Split DSP components (JetExciter.h, BoreWaveguide.h, ToneHoleSystem.h, etc.)
- Phase 3.1 priority: Get it oscillating and pitch-tracking first

### Claude's Discretion
- Internal parameter tuning values per instrument preset
- Implementation details within each DSP component
- SmoothedValue ramp times (within 2-5ms spec for tone holes)
- Tier 2 tone hole junction count (6-8 range)

### Deferred Ideas (OUT OF SCOPE)
- None for Stage 2 -- all features included per user decision
</user_constraints>

<phase_requirements>

## Phase Requirements

| ID | Description | Research Support |
|----|-------------|------------------|
| FUNC-01 | Jet-drive excitation model (Verge 1995) with tanh saturation | Section: Jet-Drive Waveguide Physics, Code Examples: tanh nonlinearity |
| FUNC-02 | Bidirectional bore waveguide with Thiran fractional delay | Section: JUCE DelayLine API, Code Examples: bidirectional waveguide |
| FUNC-03 | Tier 1 tone holes: bore-length-switching with 2-5ms crossfade | Section: Tone Hole Systems, Code Examples: SmoothedValue crossfade |
| FUNC-04 | Breath pressure with nonlinear pressure^1.5 curve | Section: Jet-Drive Physics, Code Examples: Bernoulli model |
| FUNC-05 | Embouchure control modulating jet delay ratio (0.3-0.6) | Section: Jet-Drive Physics, JUCE DelayLine Lagrange3rd |
| FUNC-06 | Overblowing via jet velocity increase | Section: Overblowing Mechanism |
| FUNC-07 | Turbulence noise injection scaled by jet velocity squared | Section: Turbulence Noise Generation |
| FUNC-08 | 4 core instrument presets | Section: Instrument Preset System |
| FUNC-09 | Pressure vibrato (NOT pitch vibrato) | Section: Jet-Drive Physics (vibrato LFO) |
| FUNC-10 | DC blocker in waveguide loop | Section: DC Blocker, Code Examples |
| FUNC-11 | Impossible physics: Infinite Sustain, Reversed Jet, Sub-Harmonics | Sections: Sub-Harmonics, Jet Nonlinearity reversed blend |
| FUNC-12 | Tier 2 tone holes: Keefe 3-port scattering | Section: Tone Hole Systems (Tier 2) |
| FUNC-13 | Bore end reflection + radiation filters | Section: JUCE IIR Filter API, Code Examples |
| FUNC-14 | Viscothermal loss filter | Section: JUCE IIR Filter API, bore loss filter |
| DSP-01 | Jet delay with Lagrange3rd interpolation | Section: JUCE DelayLine API |
| DSP-02 | Bore delay with Thiran interpolation | Section: JUCE DelayLine API |
| DSP-03 | 2x oversampling for jet nonlinearity | Section: JUCE Oversampling API |
| DSP-04 | 8-voice max polyphony, 4-voice default | Section: SynthesiserVoice Pattern |
| DSP-05 | Stereo decorrelation (Width) | Section: Stereo Width |
| PERF-01 | Real-time safe processing (no allocations) | Section: CPU Performance |
| PERF-02 | CPU per voice <2.5% at 44.1kHz | Section: CPU Performance |
| PERF-03 | Zero algorithmic latency | Section: Oversampling API (latency reporting) |
| QUAL-01 | No audio artifacts at normal parameter ranges | Section: Common Pitfalls |
| QUAL-02 | Stable oscillation startup and register transitions | Section: Overblowing Mechanism, Common Pitfalls |
| COMPAT-02 | MIDI/MPE support | Section: SynthesiserVoice Pattern |

</phase_requirements>

## Project Constraints (from CLAUDE.md)

- JUCE 8.0.4 only (local install at `/Users/taylorbrook/JUCE`)
- Build with CMake + Ninja: `ninja O-Wind_VST3 O-Wind_AU`
- Must clear AU cache after every build
- No allocations on audio thread
- `setLatencySamples()` in `prepareToPlay()` (NOT override `getLatencySamples()` -- it is non-virtual in JUCE 8)
- Research docs go in `research/`, planning docs in `.planning/`

---

## JUCE 8.0.4 API Findings

### 1. DelayLine API (CRITICAL)

**Source:** `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_DelayLine.h` (verified directly)

```cpp
template <typename SampleType, typename InterpolationType = DelayLineInterpolationTypes::Linear>
class DelayLine
{
public:
    DelayLine();
    explicit DelayLine (int maximumDelayInSamples);

    void setDelay (SampleType newDelayInSamples);
    SampleType getDelay() const;

    void prepare (const ProcessSpec& spec);
    void setMaximumDelayInSamples (int maxDelayInSamples);  // ALLOCATES -- never call on audio thread
    int getMaximumDelayInSamples() const noexcept;
    void reset();

    // Per-sample API for feedback loops:
    void pushSample (int channel, SampleType sample);
    SampleType popSample (int channel,
                          SampleType delayInSamples = -1,
                          bool updateReadPointer = true);
};
```

**Interpolation Types:**

| Type | Use Case | Modulation | Notes |
|------|----------|------------|-------|
| `Thiran` | Bore waveguide (fixed pitch) | NOT suitable for fast modulation | Flat amplitude response, stateful allpass. Minimum fractional delay 0.618 (below this, JUCE internally adds 1 to frac and subtracts 1 from int delay) |
| `Lagrange3rd` | Jet delay (real-time embouchure) | Suitable for real-time modulation | 4-point Lagrange interpolation. Minimum fractional delay: if < 2.0, JUCE adjusts internally |
| `Linear` | Not used | - | Low-pass filtering effect |
| `None` | Not used | - | Integer delay only |

**Thiran Internal Details (from source):**
```cpp
// Thiran allpass: output = (delayFrac == 0) ? value1 : value2 + alpha * (value1 - v[channel])
// alpha = (1 - delayFrac) / (1 + delayFrac)
// If delayFrac < 0.618 and delayInt >= 1: delayFrac++; delayInt--;
```

**Lagrange3rd Internal Details (from source):**
```cpp
// 4-point Lagrange interpolation over samples at indices [readPos+delayInt .. readPos+delayInt+3]
// If delayFrac < 2.0 and delayInt >= 1: delayFrac++; delayInt--;
```

**Critical: `setMaximumDelayInSamples()` allocates memory.** Must be called in `prepareToPlay()`, never in `renderNextBlock()`. The constructor `DelayLine(int maximumDelay)` sets initial buffer size.

**Bore waveguide delay sizing:**
- Lowest note C3 (~131 Hz) at 2x oversampled 44.1kHz = 88200 / 131 = ~673 samples per half
- Use 1024 as maximum delay for safety headroom across sample rates up to 96kHz
- At 96kHz * 2x = 192kHz: 192000 / 131 = ~1466 samples -> use 2048 for the constructor

**Thiran minimum delay constraint:**
- Thiran needs minimum ~1.618 samples total delay (0.618 fractional + 1 integer)
- For C7 (2093 Hz) at 88.2kHz internal: 88200 / 2093 = ~42 samples per half -- well above minimum
- No issues at any playable note

### 2. Oversampling API

**Source:** `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_Oversampling.h` (verified directly)

```cpp
template <typename SampleType>
class Oversampling
{
public:
    enum FilterType {
        filterHalfBandFIREquiripple = 0,
        filterHalfBandPolyphaseIIR,
    };

    explicit Oversampling (size_t numChannels = 1);

    Oversampling (size_t numChannels,
                  size_t factor,          // 2^factor oversampling (1 = 2x, 2 = 4x)
                  FilterType type,
                  bool isMaxQuality = true,
                  bool useIntegerLatency = false);

    void initProcessing (size_t maximumNumberOfSamplesBeforeOversampling);
    void reset() noexcept;

    AudioBlock<SampleType> processSamplesUp (const AudioBlock<const SampleType>& inputBlock) noexcept;
    void processSamplesDown (AudioBlock<SampleType>& outputBlock) noexcept;

    SampleType getLatencyInSamples() const noexcept;
    size_t getOversamplingFactor() const noexcept;
};
```

**For 2x polyphase IIR (lowest latency):**
```cpp
juce::dsp::Oversampling<float> oversampling {
    1,                                                      // 1 channel (mono per voice)
    1,                                                      // factor = 1 means 2^1 = 2x
    juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR,
    true,                                                   // max quality
    false                                                   // no integer latency compensation
};
```

**CRITICAL: Block-based API, not sample-by-sample.** `processSamplesUp()` takes an `AudioBlock` and returns an upsampled `AudioBlock`. This means:
1. You cannot upsample a single sample directly
2. Per-voice usage requires buffering the voice's input block, upsampling the entire block, processing the oversampled block sample-by-sample, then downsampling

**Per-voice oversampling pattern:**
```cpp
// In renderNextBlock:
// 1. Generate excitation samples into a small buffer (numSamples)
// 2. Upsample the buffer
auto inputBlock = juce::dsp::AudioBlock<const float>(tempInputBuffer).getSubBlock(0, numSamples);
auto oversampledBlock = oversampling.processSamplesUp(inputBlock);

// 3. Process the oversampled block sample-by-sample (jet + bore + feedback loop)
float* oversampledData = oversampledBlock.getChannelPointer(0);
size_t oversampledNumSamples = oversampledBlock.getNumSamples();

for (size_t i = 0; i < oversampledNumSamples; ++i)
{
    // ... per-sample waveguide processing at 2x rate ...
}

// 4. Downsample back to original rate
auto outputBlock = juce::dsp::AudioBlock<float>(tempOutputBuffer).getSubBlock(0, numSamples);
oversampling.processSamplesDown(outputBlock);
```

**IMPORTANT ARCHITECTURE ISSUE:** The ARCHITECTURE.md shows oversampling wrapping from embouchure summation through end reflection. But the Oversampling API works on blocks, not individual samples. The practical approach is:

1. Compute pre-oversampling signals (breath, noise, vibrato) at native rate into a temp buffer
2. Upsample the excitation signal
3. Run the per-sample feedback loop (jet delay, tanh, DC blocker, bore, loss, reflection) at 2x rate
4. Downsample the output
5. Apply radiation filter and post-processing at native rate

**Alternative (simpler for Phase 3.1):** Skip oversampling entirely in Phase 3.1. The feedback loop runs at native rate. Add oversampling in Phase 3.2. This avoids the block-buffering complexity during initial model validation.

**Latency:**
- `filterHalfBandPolyphaseIIR` at 2x: approximately 2.5-3 samples latency at the input rate
- Report via `setLatencySamples()` in prepareToPlay:
```cpp
setLatencySamples(static_cast<int>(std::ceil(oversampling.getLatencyInSamples())));
```

**Memory per instance:** Small -- AudioBuffer for 1 channel at 2x block size. 8 voices * 1 channel * ~1KB = ~8KB total. Trivial.

### 3. IIR::Filter API

**Source:** `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_IIRFilter.h` (verified directly)

**Two API layers:**

1. **`IIR::ArrayCoefficients<float>`** -- static methods returning `std::array<float, 4>` (first-order) or `std::array<float, 6>` (second-order). Lightweight, no heap allocation.

2. **`IIR::Coefficients<float>`** -- ref-counted objects (inherits `ProcessorState`). Has the same factory methods but returns `Coefficients::Ptr`. Used by `IIR::Filter<float>`.

3. **`IIR::Filter<float>`** -- the processor. Has `processSample(float)` for per-sample use and `process(context)` for block processing.

**Key API for per-sample use in feedback loops:**
```cpp
juce::dsp::IIR::Filter<float> filter;

// Setup (in prepareToPlay):
juce::dsp::ProcessSpec spec { sampleRate, (juce::uint32)maxBlockSize, 1 };
filter.prepare(spec);
filter.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderLowPass(sampleRate, cutoff);

// Per-sample (in renderNextBlock):
float output = filter.processSample(input);
```

**Coefficient update pattern:**
```cpp
// On parameter change (NOT per-sample):
*filter.coefficients = juce::dsp::IIR::Coefficients<float>(
    juce::dsp::IIR::ArrayCoefficients<float>::makeFirstOrderLowPass(sampleRate, newCutoff));
```

**Or using direct coefficient assignment (O-Bowed pattern):**
```cpp
// Custom coefficients like O-Bowed's bridge loss filter:
*filter.coefficients = juce::dsp::IIR::Coefficients<float>(b0, b1, a0, a1);
```

**Required filters per voice:**

| Filter | Type | Factory Method | Purpose |
|--------|------|---------------|---------|
| Bore loss | 2nd-order LP | `makeLowPass(sr, cutoff, Q)` | Viscothermal + tone color |
| End reflection | 1st-order LP | `makeFirstOrderLowPass(sr, cutoff)` | Frequency-dependent reflection |
| Radiation | 1st-order HP | `makeFirstOrderHighPass(sr, cutoff)` | Radiation impedance |
| Noise shaping | 1st-order LP | `makeFirstOrderLowPass(sr, cutoff)` | Turbulence spectrum |
| Tone hole (Tier 2) | 2nd-order | `makeLowPass` / `makeHighPass` | Per-junction scattering (6-8 filters) |

**CRITICAL: `snapToZero()` after per-sample processing.** The JUCE header notes: "you might need the function snapToZero after a few calls to avoid potential denormalisation issues." Call periodically (every N samples) or rely on `juce::ScopedNoDenormals` in processBlock.

### 4. SmoothedValue API

**Source:** `/Users/taylorbrook/JUCE/modules/juce_audio_basics/utilities/juce_SmoothedValue.h` (verified directly)

```cpp
template <typename FloatType, typename SmoothingType = ValueSmoothingTypes::Linear>
class SmoothedValue
{
public:
    SmoothedValue();
    SmoothedValue(FloatType initialValue);

    void reset(double sampleRate, double rampLengthInSeconds);
    void reset(int numSteps);

    void setTargetValue(FloatType newValue);
    void setCurrentAndTargetValue(FloatType newValue);  // immediate, no ramp

    FloatType getNextValue();         // advances one step
    FloatType getCurrentValue() const;
    FloatType getTargetValue() const;
    bool isSmoothing() const;
};
```

**Usage for bore delay crossfade (Tier 1 tone holes):**
```cpp
juce::SmoothedValue<float> boreDelay;

// In prepareToPlay:
boreDelay.reset(sampleRate, 0.003);  // 3ms ramp

// On note change:
float newDelay = calculateBoreDelay(midiNote, internalSampleRate);
boreDelay.setTargetValue(newDelay);

// Per-sample in oversampled loop:
float currentDelay = boreDelay.getNextValue();
float halfDelay = currentDelay * 0.5f;
// ... use halfDelay for bore forward/backward delay lines
```

**IMPORTANT: SmoothedValue operates at whatever rate you call `getNextValue()`.** If the waveguide runs at 2x oversampled rate, the SmoothedValue ramp will be 2x as many steps. Either:
- Initialize with `reset(oversampledSampleRate, rampSeconds)` -- simplest
- Or call at native rate and hold the value for 2 oversampled iterations

### 5. SynthesiserVoice Pattern (from O-Bowed reference)

**O-Bowed reference:** `/Users/taylorbrook/Dev/VST-development/plugins/O-Bowed/Source/BowedStringVoice.h/.cpp`

The proven pattern for this project:

```cpp
class FluteSynthVoice : public juce::SynthesiserVoice
{
public:
    explicit FluteSynthVoice(juce::AudioProcessorValueTreeState* apvts);

    bool canPlaySound(juce::SynthesiserSound*) override;
    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound*, int pitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newValue) override;
    void controllerMoved(int controllerNumber, int newValue) override;

    void prepareToPlay(double sampleRate, int maxBlockSize);
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                         int startSample, int numSamples) override;

private:
    void updateParametersFromAPVTS();  // reads atomic params once per block

    juce::AudioProcessorValueTreeState* parameters = nullptr;

    // DSP components
    JetExciter jetExciter;
    BoreWaveguide boreWaveguide;
    DCBlocker dcBlocker;
    ToneHoleSystem toneHoles;
    // ... etc
};
```

**Key patterns from O-Bowed:**
1. Constructor takes `APVTS*` pointer
2. `prepareToPlay()` is custom (not a SynthesiserVoice override) -- called manually from processor
3. `updateParametersFromAPVTS()` uses `parameters->getRawParameterValue("id")->load()` -- atomic, lock-free
4. Read params ONCE per block, NOT per sample
5. Per-sample loop: `while (--numSamples >= 0) { ... ++startSample; }`
6. Voice cleanup: `clearCurrentNote()` when excitation released AND energy decayed
7. Hard-clip output: `juce::jlimit(-2.0f, 2.0f, sample)`
8. Write to all channels: `outputBuffer.addSample(ch, startSample, sample)`

**Voice addition in processor (in prepareToPlay):**
```cpp
synthesiser.clearVoices();
for (int i = 0; i < 8; ++i)  // 8 max voices
{
    auto* voice = new FluteSynthVoice(&parameters);
    voice->prepareToPlay(sampleRate, samplesPerBlock);
    synthesiser.addVoice(voice);
}
synthesiser.setCurrentPlaybackSampleRate(sampleRate);
```

---

## Algorithm Implementation Details

### Jet-Drive Waveguide Physics

**Complete per-sample algorithm (Phase 3.1 minimal):**

```cpp
// State from previous sample:
float boreFeedback;  // stored after end reflection

// Step 1: Breath model
float breathPressure = breathParam * breathEnvelope;
float jetVelocity = std::pow(breathPressure, 1.5f);  // Bernoulli nonlinear curve

// Step 2: Embouchure summation
float excitation = jetVelocity + boreFeedback * jetReflectionParam;

// Step 3: Jet delay (Lagrange3rd)
jetDelay.pushSample(0, excitation);
float jetDelayLength = embouchureParam * boreDelayTotal;  // 0.3-0.6 range
float jetOutput = jetDelay.popSample(0, jetDelayLength);

// Step 4: Jet nonlinearity
float jetGain = 2.0f;  // per-preset, range 1.0-3.0
float labiumOutput = std::tanh(jetGain * jetOutput);

// Step 5: DC blocker
float dcOut = labiumOutput - dcBlocker.xPrev + 0.995f * dcBlocker.yPrev;
dcBlocker.xPrev = labiumOutput;
dcBlocker.yPrev = dcOut;

// Step 6: Bore waveguide (bidirectional)
float halfDelay = boreDelayTotal * 0.5f;
boreFwd.pushSample(0, dcOut);                    // inject into forward wave
float pPlus = boreFwd.popSample(0, halfDelay);   // forward wave at open end

// Step 7: Bore loss filter (inside loop)
float pPlusFiltered = boreLossFilter.processSample(pPlus);

// Step 8: End reflection (sign-inverted lowpass)
float reflected = -endReflectionParam * endReflectionFilter.processSample(pPlusFiltered);
boreBwd.pushSample(0, reflected);                // reflected backward wave
float pMinus = boreBwd.popSample(0, halfDelay);  // backward wave at embouchure end

// Step 9: Store feedback for next sample
boreFeedback = pMinus;

// Step 10: Radiation filter (output tap)
float voiceOutput = radiationFilter.processSample(pPlusFiltered);
```

### Overblowing Mechanism

Physics-based register transition requires no explicit logic. The jet-to-bore delay ratio determines which harmonic mode the feedback loop locks onto:

| Jet Ratio (embouchure * boreDelay) | Expected Register | Physics |
|--------------------------------------|-------------------|---------|
| ~0.45-0.50 of bore delay | Fundamental (1st harmonic) | Full round-trip phase alignment |
| ~0.30-0.35 of bore delay | 2nd harmonic (octave) | Half-wavelength phase alignment |
| ~0.20-0.25 of bore delay | 3rd harmonic (12th) | Third-wavelength alignment |

**Hysteresis via SmoothedValue:** The embouchure parameter should ramp (2-5ms) to prevent abrupt ratio changes that cause unstable register flickering. The tanh nonlinearity provides natural amplitude limiting during transitions.

**Self-oscillation conditions:**
- Nonzero breath pressure (jet velocity > 0)
- Feedback gain (jetReflection) > ~0.3
- Loop gain > loss (end reflection + bore loss must not overdamp)
- Default params (breathPressure=0.5, jetReflection=0.5, endReflection=0.5) should self-oscillate

### Turbulence Noise Generation

```cpp
// In JetExciter:
juce::Random noiseSource;
juce::dsp::IIR::Filter<float> noiseFilter;  // 1-pole lowpass

// Per sample:
float whiteNoise = noiseSource.nextFloat() * 2.0f - 1.0f;  // [-1, 1]
float filteredNoise = noiseFilter.processSample(whiteNoise);

// Quadratic scaling with jet velocity:
float noiseGain = breathNoiseParam * jetVelocity * jetVelocity;
float turbulenceOut = filteredNoise * noiseGain;

// Inject at embouchure summation:
float excitation = jetVelocity + turbulenceOut + boreFeedback * jetReflectionParam;
```

**Noise filter cutoff:** Proportional to jet velocity. Higher breath = higher cutoff (more HF content). Range ~1000-6000 Hz. Update cutoff on parameter change, not per-sample.

### Sub-Harmonics Generator

**Frequency-halving via asymmetric clipping in feedback path:**

```cpp
// Inside waveguide loop, pre-radiation:
if (subHarmonicsParam > 0.0f)
{
    // Asymmetric soft-clip: different gain for positive vs negative half-cycles
    float asymmetric = (pPlusFiltered >= 0.0f)
        ? pPlusFiltered
        : pPlusFiltered * 0.5f;  // reduce negative half-cycle

    // Blend with original
    pPlusFiltered = pPlusFiltered + subHarmonicsParam * (asymmetric - pPlusFiltered);
}
```

The asymmetric clipping creates even harmonics and a period-doubling effect. At low blend levels (0.1-0.3), this adds subtle sub-octave warmth. At high levels, dominant sub-octave.

### Reversed Jet Nonlinearity

```cpp
// REVERSED_JET blends normal and inverted tanh:
float normalTanh = std::tanh(jetGain * jetOutput);
float invertedTanh = -std::tanh(-jetGain * jetOutput);  // = tanh(jetGain * jetOutput) for odd function
// Wait -- tanh IS odd: tanh(-x) = -tanh(x), so -tanh(-x) = tanh(x).
// The "reversed jet" needs a different approach.

// Correct implementation: phase-invert the jet signal before nonlinearity
float effectiveJet = juce::jmap(reversedJetParam, jetOutput, -jetOutput);
float labiumOutput = std::tanh(jetGain * effectiveJet);
```

**Note:** Since tanh is an odd function, `-tanh(-x) = tanh(x)`. The "reversed jet" effect must invert the jet signal BEFORE the nonlinearity, changing the phase relationship between jet and bore. This creates different resonance modes and timbral effects.

### Bidirectional Bore Waveguide Detail

Two `DelayLine<float, Thiran>` instances per voice:

```cpp
// Members:
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> boreFwd { 2048 };
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> boreBwd { 2048 };

// Prepare:
juce::dsp::ProcessSpec spec { internalSampleRate, (juce::uint32)maxBlockSize * 2, 1 };
boreFwd.prepare(spec);
boreFwd.setMaximumDelayInSamples(2048);
boreBwd.prepare(spec);
boreBwd.setMaximumDelayInSamples(2048);

// Delay calculation:
float f0 = tuningEngine->getFrequency(midiNote);
float totalDelay = static_cast<float>(internalSampleRate) / f0;
// Subtract filter group delay compensation (same pattern as O-Bowed):
float compensatedDelay = totalDelay - filterGroupDelay;
compensatedDelay = std::max(4.0f, compensatedDelay);
float halfDelay = compensatedDelay * 0.5f;
halfDelay = std::max(2.0f, halfDelay);  // Thiran needs >= ~1.618 samples
```

### Tone Hole Systems

**Tier 1 (Bore Length Switching):**
```cpp
// Lookup table: MIDI note -> bore delay at internal sample rate
std::array<float, 128> boreDelayTable;

void calculateBoreDelayTable(double internalSampleRate, TuningEngine* tuning)
{
    for (int note = 0; note < 128; ++note)
    {
        float freq = tuning->getFrequency(note);
        boreDelayTable[note] = static_cast<float>(internalSampleRate) / freq;
    }
}

// On note change:
juce::SmoothedValue<float> boreDelaySmoothed;
boreDelaySmoothed.reset(internalSampleRate, 0.003);  // 3ms crossfade
boreDelaySmoothed.setTargetValue(boreDelayTable[newMidiNote]);
```

**Tier 2 (Keefe 3-Port Scattering Junction):**

Each junction has 3 ports: upstream bore, downstream bore, tone hole (side branch).

```cpp
struct ToneHoleJunction
{
    juce::dsp::IIR::Filter<float> openFilter;   // 2nd-order for open state
    juce::dsp::IIR::Filter<float> closedFilter; // 2nd-order for closed state
    float openAmount = 0.0f;  // 0 = closed, 1 = open (continuous for half-holing)

    // Scattering coefficients
    float reflectionCoeff;    // R = (Z_hole - Z_bore) / (Z_hole + Z_bore)
    float transmissionCoeff;  // T = 1 + R

    void processSample(float& pPlusIn, float& pMinusIn,
                       float& pPlusOut, float& pMinusOut)
    {
        // 3-port scattering:
        // Interpolate between open and closed filter states
        float openResp = openFilter.processSample(pPlusIn) * openAmount;
        float closedResp = closedFilter.processSample(pPlusIn) * (1.0f - openAmount);
        float holeResponse = openResp + closedResp;

        // Scattered waves:
        pPlusOut = pPlusIn * transmissionCoeff + holeResponse * reflectionCoeff;
        pMinusOut = pMinusIn * transmissionCoeff + holeResponse * reflectionCoeff;
    }
};

// 6-8 junctions inline with bore:
std::array<ToneHoleJunction, 8> toneHoles;
```

**CPU estimate for Tier 2:** ~8 multiply-adds per junction (filter eval + scattering) * 8 junctions = ~64 ops per sample. At 2x oversampling: ~128 ops per output sample.

### Instrument Preset System

```cpp
struct InstrumentPreset
{
    const char* name;
    float jetGain;            // tanh gain factor
    float noiseLevel;         // turbulence noise multiplier
    float noiseCutoffBase;    // base cutoff for noise shaping filter
    float radiationCutoff;    // radiation highpass cutoff
    float boreLossCutoff;     // bore loss lowpass cutoff
    float boreLossQ;          // bore loss filter Q
    float endReflCutoff;      // end reflection lowpass cutoff
    float embouchureMin;      // min jet ratio
    float embouchureMax;      // max jet ratio
    float defaultBreath;      // default breath pressure
    float attackTimeMs;       // minimum attack ramp
};

static constexpr InstrumentPreset presets[] = {
    // Core 4 (Phase 3.3)
    { "Concert Flute",    2.0f, 0.15f, 4000.0f, 300.0f, 8000.0f, 0.707f, 3000.0f, 0.35f, 0.55f, 0.5f,  5.0f },
    { "Shakuhachi",       1.5f, 0.35f, 3000.0f, 200.0f, 5000.0f, 0.500f, 2000.0f, 0.30f, 0.60f, 0.4f, 15.0f },
    { "Bansuri",          1.8f, 0.25f, 3500.0f, 250.0f, 6000.0f, 0.600f, 2500.0f, 0.35f, 0.55f, 0.5f, 10.0f },
    { "Native Am. Flute", 1.3f, 0.20f, 2500.0f, 180.0f, 4000.0f, 0.450f, 1500.0f, 0.40f, 0.50f, 0.3f, 20.0f },

    // Expansion 4 (Phase 3.4)
    { "Recorder",         2.5f, 0.08f, 5000.0f, 400.0f, 10000.0f, 0.707f, 4000.0f, 0.42f, 0.48f, 0.6f,  3.0f },
    { "Pan Flute",        1.0f, 0.40f, 2000.0f, 350.0f, 6000.0f,  0.500f, 2500.0f, 0.38f, 0.52f, 0.4f, 12.0f },
    { "Piccolo",          2.2f, 0.12f, 5000.0f, 500.0f, 12000.0f, 0.707f, 5000.0f, 0.32f, 0.50f, 0.5f,  4.0f },
    { "Ocarina",          1.6f, 0.05f, 3000.0f, 250.0f, 5000.0f,  0.600f, 2000.0f, 0.45f, 0.50f, 0.5f,  8.0f },
};
```

**Preset switching:** Update all filter coefficients and internal params immediately (no crossfade needed for internal coefficients -- they only affect the spectral shape, not signal amplitude).

### Stereo Width (Post-Voice)

**Applied in processor's processBlock, not per-voice:**

```cpp
// After synthesiser.renderNextBlock(buffer, midiMessages, 0, numSamples):
if (buffer.getNumChannels() >= 2)
{
    float width = widthParam;  // 0.0 = mono, 1.0 = natural, 2.0 = wide
    float* left = buffer.getWritePointer(0);
    float* right = buffer.getWritePointer(1);

    for (int i = 0; i < numSamples; ++i)
    {
        float mid = (left[i] + right[i]) * 0.5f;
        float side = (left[i] - right[i]) * 0.5f;
        left[i]  = mid + side * width;
        right[i] = mid - side * width;
    }
}
```

**Note:** Mono voices write the same sample to both channels (`outputBuffer.addSample` for each channel in the voice loop). For stereo width to work, we need per-voice decorrelation BEFORE summation. Options:
1. **Per-voice slight delay offset** (1-3 samples random per voice, applied to one channel) -- cheapest
2. **Per-voice allpass decorrelation** on the right channel

Recommend: Voice writes to channel 0 only. Processor copies channel 0 to channel 1, then applies stereo width with a tiny per-voice random delay as decorrelation seed.

Actually, simplest proven approach: Each voice writes mono to both channels. After all voices sum, apply a short allpass on the right channel for decorrelation, then mid-side width control:

```cpp
// Decorrelation allpass (Schroeder, ~5 samples delay):
juce::dsp::IIR::Filter<float> decorrelationAllpass;
// In prepareToPlay:
decorrelationAllpass.coefficients = juce::dsp::IIR::Coefficients<float>::makeFirstOrderAllPass(
    sampleRate, 2000.0f);

// In processBlock after voice render:
float* right = buffer.getWritePointer(1);
for (int i = 0; i < numSamples; ++i)
    right[i] = decorrelationAllpass.processSample(right[i]);
// Then apply mid-side width
```

---

## Per-Voice Oversampling Pattern (Phase 3.2+)

**Challenge:** JUCE's `Oversampling` is block-based but we need per-sample feedback processing.

**Solution:** Buffer the voice's block, upsample it, process sample-by-sample at 2x, downsample.

```cpp
class FluteSynthVoice : public juce::SynthesiserVoice
{
    // Oversampling (per-voice, 1 channel, 2x, IIR halfband)
    juce::dsp::Oversampling<float> oversampling { 1, 1,
        juce::dsp::Oversampling<float>::filterHalfBandPolyphaseIIR, true };

    // Temp buffers for oversampling
    juce::AudioBuffer<float> excitationBuffer;  // native rate
    juce::AudioBuffer<float> outputBuffer;      // native rate

    void prepareToPlay(double sampleRate, int maxBlockSize)
    {
        oversampling.initProcessing(static_cast<size_t>(maxBlockSize));
        oversampling.reset();
        excitationBuffer.setSize(1, maxBlockSize);
        outputBuffer.setSize(1, maxBlockSize);

        double internalRate = sampleRate * 2.0;
        // Prepare all DSP at internal (oversampled) rate:
        // ...
    }

    void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override
    {
        updateParametersFromAPVTS();

        // Phase 1: Generate excitation at native rate
        for (int i = 0; i < numSamples; ++i)
        {
            breathEnvelope.advance();
            float breath = breathPressure * breathEnvelope.getValue();
            float jetVel = std::pow(breath, 1.5f);
            float noise = generateNoise(jetVel);
            float excitation = jetVel + noise + boreFeedbackStored * jetReflection;
            excitationBuffer.setSample(0, i, excitation);
        }

        // Phase 2: Upsample excitation
        auto inputBlock = juce::dsp::AudioBlock<const float>(excitationBuffer).getSubBlock(0, numSamples);
        auto osBlock = oversampling.processSamplesUp(inputBlock);
        float* osData = osBlock.getChannelPointer(0);
        size_t osNumSamples = osBlock.getNumSamples();

        // Phase 3: Per-sample waveguide at 2x rate
        for (size_t i = 0; i < osNumSamples; ++i)
        {
            float exc = osData[i];
            // Jet delay, tanh, DC blocker, bore waveguide, loss, reflection
            float out = processWaveguide(exc);
            osData[i] = out;
        }

        // Phase 4: Downsample to output buffer
        auto outBlock = juce::dsp::AudioBlock<float>(outputBuffer).getSubBlock(0, numSamples);
        oversampling.processSamplesDown(outBlock);

        // Phase 5: Write to output
        for (int i = 0; i < numSamples; ++i)
        {
            float sample = outputBuffer.getSample(0, i);
            sample = juce::jlimit(-2.0f, 2.0f, sample);
            for (int ch = buffer.getNumChannels(); --ch >= 0;)
                buffer.addSample(ch, startSample + i, sample * outputGainLinear);
        }
    }
};
```

**CRITICAL ISSUE with this pattern:** The bore feedback from the previous sample needs to feed back into the NEXT block's excitation. Since excitation is pre-computed at native rate before upsampling, the feedback is delayed by one block. For Phase 3.1 (no oversampling), this is not an issue -- everything is per-sample in a single loop.

**Better pattern for oversampled feedback (Phase 3.2):** Compute everything at 2x rate inside the oversampled block. Generate excitation per-sample INSIDE the oversampled loop, not before upsampling:

```cpp
void renderNextBlock(juce::AudioBuffer<float>& buffer, int startSample, int numSamples) override
{
    updateParametersFromAPVTS();

    // Fill excitation buffer with a simple ramp/constant (just for upsampling structure)
    // Actually: use a dummy upsample of zeros, then overwrite per-sample
    excitationBuffer.clear(0, numSamples);
    auto inputBlock = juce::dsp::AudioBlock<const float>(excitationBuffer).getSubBlock(0, numSamples);
    auto osBlock = oversampling.processSamplesUp(inputBlock);
    float* osData = osBlock.getChannelPointer(0);
    size_t osNum = osBlock.getNumSamples();

    // Per-sample at 2x rate: compute everything inside
    for (size_t i = 0; i < osNum; ++i)
    {
        // Advance envelope at oversampled rate (or at half rate with counter)
        float breath = getBreathValue();
        float jetVel = std::pow(breath, 1.5f);
        float noise = generateNoise(jetVel);
        float excitation = jetVel + noise + boreFeedbackStored * jetReflection;

        // Waveguide processing
        jetDelay.pushSample(0, excitation);
        float jetOut = jetDelay.popSample(0, jetDelayLength);
        float labium = std::tanh(jetGain * jetOut);
        float dc = dcBlocker.process(labium);

        boreFwd.pushSample(0, dc);
        float pPlus = boreFwd.popSample(0, halfBoreDelay);
        float pFiltered = boreLossFilter.processSample(pPlus);
        float reflected = -endRefl * endReflFilter.processSample(pFiltered);
        boreBwd.pushSample(0, reflected);
        boreFeedbackStored = boreBwd.popSample(0, halfBoreDelay);

        osData[i] = radiationFilter.processSample(pFiltered);
    }

    // Downsample
    auto outBlock = juce::dsp::AudioBlock<float>(outputBuffer).getSubBlock(0, numSamples);
    oversampling.processSamplesDown(outBlock);

    // Write to output
    for (int i = 0; i < numSamples; ++i)
    {
        float sample = outputBuffer.getSample(0, i);
        sample = juce::jlimit(-2.0f, 2.0f, sample);
        for (int ch = buffer.getNumChannels(); --ch >= 0;)
            buffer.addSample(ch, startSample + i, sample * outputGainLinear);
    }
}
```

**This is the correct oversampled pattern.** Upsample a dummy/zero input, overwrite the oversampled buffer with the per-sample waveguide output, then downsample. The upsampling of zeros means the Oversampling object's internal filters get fed zeros -- the actual content is generated at 2x rate directly. The downsampling filter then properly anti-aliases the output.

**Wait -- this is incorrect.** Upsampling zeros and then overwriting defeats the purpose of the anti-aliasing filter. The correct approach:

**Correct pattern: Process the ENTIRE voice at 2x rate, upsample only the breath/CC input signals, downsample only the voice output.**

Actually, the simplest correct approach for a feedback system is to skip `juce::dsp::Oversampling` entirely and implement manual 2x processing:

1. For each output sample, run the waveguide loop TWICE at the internal rate
2. Average or filter the two output samples to get one output sample
3. Use a simple one-pole lowpass as the anti-aliasing filter on output

This avoids the block-API mismatch entirely. However, it sacrifices the quality of JUCE's polyphase halfband filter.

**Recommended approach for O-Wind:**

For Phase 3.1: No oversampling. Everything at native rate. Validate the model.

For Phase 3.2: Use JUCE Oversampling properly. The technique is:
1. Upsample the breath/excitation signal (block-based)
2. Run the waveguide loop over the upsampled block sample-by-sample
3. The bore feedback is naturally handled because we process sample-by-sample WITHIN the upsampled block
4. Downsample the output (block-based)

But the feedback from the bore needs to feed back into the excitation. Within a single upsampled block, this works per-sample. Between blocks, the stored `boreFeedbackStored` carries over correctly because it is per-voice state.

The issue is: the excitation signal is upsampled BEFORE we know the feedback. Solution: upsample a "control signal" (breath envelope, noise modulation) and compute the actual excitation (including feedback) at the oversampled rate per-sample:

```cpp
// Correct Phase 3.2 pattern:
// 1. Fill controlBuffer with per-native-sample control values
//    (breath pressure, noise random values, vibrato phase)
for (int i = 0; i < numSamples; ++i)
    controlBuffer.setSample(0, i, getBreathPressure());

// 2. Upsample control signal
auto ctrlBlock = juce::dsp::AudioBlock<const float>(controlBuffer).getSubBlock(0, numSamples);
auto osCtrl = oversampling.processSamplesUp(ctrlBlock);

// 3. Process waveguide at 2x, computing excitation per-sample with feedback
float* ctrl = osCtrl.getChannelPointer(0);
for (size_t i = 0; i < osCtrl.getNumSamples(); ++i)
{
    float breath = ctrl[i];
    float jetVel = std::pow(std::abs(breath), 1.5f);
    float noise = generateNoise(jetVel);
    float excitation = jetVel + noise + boreFeedback * jetRefl;
    // ... full waveguide ...
    ctrl[i] = voiceOutput;  // overwrite in-place
}

// 4. Downsample output
auto outBlock = juce::dsp::AudioBlock<float>(outputBuffer).getSubBlock(0, numSamples);
oversampling.processSamplesDown(outBlock);
```

This is clean and correct. The control signal (breath) is upsampled for smooth interpolation at 2x rate. Feedback is computed per-sample at the oversampled rate. Output is properly downsampled with anti-aliasing.

---

## Module Reuse Opportunities

| Module | Source | Reusable? | Notes |
|--------|--------|-----------|-------|
| TuningEngine | `modules/tuning/scala-tuning-engine` | YES | Already linked in Stage 1. Pointer passed to voice. |
| WaveguideString (O-Bowed) | `plugins/O-Bowed/Source/DSP/` | PATTERN ONLY | Same `DelayLine<Thiran>` push/pop pattern. Different topology (bow split vs. full bore). |
| BowModel (O-Bowed) | `plugins/O-Bowed/Source/DSP/` | NO | Completely different excitation physics. |
| StereoWidth | N/A | NEW | O-Bowed has no stereo width DSP module yet. Create new, share pattern later. |
| SubHarmonics | N/A | NEW | O-Bowed has the APVTS parameter but no DSP implementation yet. |
| DCBlocker | N/A | NEW | Trivial (2 state vars). Could be shared across projects. |

---

## Common Pitfalls

### Pitfall 1: Oversampling API Mismatch with Feedback Loops
**What goes wrong:** Attempting to use JUCE Oversampling sample-by-sample, or upsampling the complete excitation (including feedback) as a block -- feedback is delayed by one block.
**Why it happens:** JUCE Oversampling is block-based. Feedback loops need per-sample computation.
**How to avoid:** Upsample only the CONTROL signal (breath, noise seed). Compute excitation WITH feedback per-sample INSIDE the oversampled block.
**Warning signs:** Pitch instability, oscillation failure, or buzzy artifacts at block boundaries.

### Pitfall 2: Thiran Delay Line State After Reset
**What goes wrong:** After `reset()`, Thiran's internal state variable `v[channel]` is cleared. The first few samples may produce transient artifacts.
**Why it happens:** Thiran allpass is stateful (stores previous output). Reset clears this state.
**How to avoid:** Accept the brief transient (inaudible). Or avoid `reset()` on note change -- just update delay length. O-Bowed calls `reset()` in `trigger()` which is fine.
**Warning signs:** Clicks at note onset.

### Pitfall 3: `setMaximumDelayInSamples()` Allocates Memory
**What goes wrong:** Calling `setMaximumDelayInSamples()` on the audio thread causes allocation.
**Why it happens:** It resizes the internal `AudioBuffer`.
**How to avoid:** Only call in `prepareToPlay()`. Size the constructor argument large enough for all sample rates.
**Warning signs:** Occasional audio dropouts, priority inversion.

### Pitfall 4: DC Accumulation Without DC Blocker
**What goes wrong:** The tanh nonlinearity and feedback loop accumulate DC offset over time, eventually saturating the output.
**Why it happens:** Asymmetric nonlinearity + feedback = DC drift. Even symmetric tanh can accumulate DC from numerical precision.
**How to avoid:** Always include DC blocker after jet nonlinearity. The `y[n] = x[n] - x[n-1] + 0.995 * y[n-1]` formula is proven.
**Warning signs:** Output slowly drifts to one rail, signal becomes distorted.

### Pitfall 5: Filter Coefficient Updates on Audio Thread
**What goes wrong:** Creating new `Coefficients::Ptr` objects in the audio loop causes heap allocation.
**Why it happens:** `Coefficients` inherits `ReferenceCountedObject`.
**How to avoid:** Use the `*filter.coefficients = Coefficients(...)` pattern (assignment, not pointer creation). Or pre-allocate coefficients and assign raw values. O-Bowed uses direct coefficient construction: `*bridgeLossFilter.coefficients = juce::dsp::IIR::Coefficients<float>(b0, b1, a0, a1);`
**Warning signs:** Audio dropouts during parameter automation.

### Pitfall 6: Denormals in Feedback Loop
**What goes wrong:** Tiny floating-point values (denormals) in the waveguide loop cause massive CPU spikes on some processors.
**Why it happens:** Decaying signals approach denormal range. Feedback keeps them alive.
**How to avoid:** `juce::ScopedNoDenormals` at the top of `processBlock()`. Also add explicit flush: `if (std::abs(sample) < 1e-15f) sample = 0.0f;` (O-Bowed pattern).
**Warning signs:** CPU meter spikes during note decay.

### Pitfall 7: Oversampling Latency Not Reported
**What goes wrong:** Oversampled voices have ~2.5 sample latency but host does not compensate, causing phase issues with parallel tracks.
**Why it happens:** Forgetting to call `setLatencySamples()`.
**How to avoid:** In `prepareToPlay()`: `setLatencySamples(static_cast<int>(std::ceil(oversampling.getLatencyInSamples())));`
**Warning signs:** Phase cancellation when mixing with other tracks.

### Pitfall 8: Self-Oscillation Failure
**What goes wrong:** Model produces no sound despite nonzero breath pressure.
**Why it happens:** Feedback gain too low, bore loss too high, or end reflection too low -- loop gain < 1.
**How to avoid:** Ensure default parameter combination produces sound. Test: jetReflection=0.5, endReflection=0.5, boreLoss cutoff > 4000Hz, breath > 0.2. Add noise burst seeding at note-on if needed (1ms broadband noise).
**Warning signs:** Silence or very quiet output at reasonable breath levels.

### Pitfall 9: SmoothedValue at Wrong Sample Rate
**What goes wrong:** Bore delay crossfade takes too long or too short during oversampled processing.
**Why it happens:** SmoothedValue initialized at native rate but advanced at oversampled rate.
**How to avoid:** Initialize `boreDelay.reset(internalSampleRate, 0.003)` using the oversampled rate if `getNextValue()` is called at that rate.
**Warning signs:** Clicks during fast note changes (too short) or sluggish pitch response (too long).

### Pitfall 10: getLatencySamples() is NOT Virtual
**What goes wrong:** Attempting to override `getLatencySamples()` -- it compiles but the override is never called.
**Why it happens:** `AudioProcessor::getLatencySamples()` is non-virtual in JUCE 8.
**How to avoid:** Use `setLatencySamples(N)` in `prepareToPlay()`. The stored value is returned by the non-virtual getter.

---

## CPU Performance Considerations

**Target:** <2.5% per voice (Tier 1), <3.5% per voice (Tier 2 tone holes)

**Per-sample operation count (Tier 1, no oversampling):**

| Operation | Cost (approx ops) |
|-----------|-------------------|
| Breath model (pow, multiply) | ~10 |
| Noise generation (Random + LP filter) | ~8 |
| Embouchure summation | ~3 |
| Jet delay push/pop (Lagrange3rd) | ~15 |
| std::tanh | ~8 |
| DC blocker | ~4 |
| Bore fwd push/pop (Thiran) | ~10 |
| Bore bwd push/pop (Thiran) | ~10 |
| Bore loss filter (processSample) | ~8 |
| End reflection filter + multiply | ~8 |
| Radiation filter | ~6 |
| **Total per sample** | **~90** |

**With 2x oversampling:** ~180 ops per output sample (inner loop), plus ~20 for upsample/downsample filter overhead = ~200 ops/sample.

**With Tier 2 tone holes (8 junctions):** Add ~128 ops per output sample = ~328 ops/sample total.

**At 44.1kHz on modern Apple Silicon:**
- ~90 ops * 44100 = 3.97M ops/sec per voice (Tier 1, no OS)
- ~200 ops * 44100 = 8.82M ops/sec per voice (Tier 1, 2x OS)
- ~328 ops * 44100 = 14.47M ops/sec per voice (Tier 2, 2x OS)
- M1 delivers ~50 GFLOPS single-thread = well within budget

**Optimization strategies:**
1. `juce::ScopedNoDenormals` in processBlock (already present)
2. Read APVTS once per block, not per sample (O-Bowed pattern)
3. Use `SmoothedValue` for parameter interpolation (avoid branching)
4. `std::tanh` is fine (~8 ops). Only switch to `LookupTableTransform` if profiling shows it as bottleneck
5. Pre-calculate bore delay table in `prepareToPlay`, not per-note
6. Avoid virtual calls in inner loop -- DSP components should be direct members, not pointers

---

## Architecture Patterns

### Recommended Project Structure

```
Source/
  PluginProcessor.h/cpp       # Existing -- add voice setup, processBlock routing
  PluginEditor.h/cpp           # Existing -- no changes in Stage 2
  FluteSynthSound.h            # Existing -- trivial sound class
  FluteSynthVoice.h/cpp        # NEW -- SynthesiserVoice, orchestrates DSP chain
  DSP/
    JetExciter.h               # Breath model, Bernoulli, turbulence noise, vibrato LFO
    JetNonlinearity.h          # tanh saturation, reversed jet blend
    BoreWaveguide.h            # Bidirectional Thiran delay lines, bore loss, end reflection, radiation
    DCBlocker.h                # Inline DC blocking filter (2 state vars)
    ToneHoleSystem.h           # Tier 1 bore-length switching + Tier 2 Keefe scattering
    SubHarmonics.h             # Nonlinear feedback sub-octave generator
    StereoWidth.h              # Mid-side decorrelation (applied in processor, not voice)
    InstrumentPresets.h        # constexpr parameter sets for 8 instruments
```

### Anti-Patterns to Avoid

- **ProcessorChain for waveguide:** Cannot use JUCE's ProcessorChain because feedback loop requires per-sample access to intermediate signals. Must be manual per-sample loop.
- **Per-sample APVTS reads:** Atomic loads are cheap but not free. Read once per block, store in local variables.
- **Coefficient Ptr creation in audio loop:** Use assignment to existing coefficients, not factory methods that return Ptr.
- **Oversampling per-sample calls:** JUCE Oversampling has no single-sample API. Must use block-based up/down.
- **Shared Oversampling instance:** Each voice needs its own Oversampling instance (they have internal state/buffers).

---

## Phase-by-Phase Implementation Recommendations

### Phase 3.1: Minimal Oscillating Model
- **NO oversampling** -- everything at native rate, simplest possible feedback loop
- Implement: FluteSynthVoice, JetExciter (basic breath + Bernoulli), BoreWaveguide (bidirectional Thiran), DCBlocker, basic radiation/reflection filters
- Use ToneHoleSystem Tier 1 for pitch (just bore delay lookup)
- 6 params active: breathPressure, embouchure, toneColor, jetReflection, endReflection, outputLevel
- Monophonic (1 voice) for debugging
- **Success criterion:** Self-oscillation, correct pitch tracking C4-C7, responds to breath/embouchure

### Phase 3.2: Expression + Oversampling
- Add JetExciter enhancements: turbulence noise, vibrato LFO, CC2/aftertouch/CC74 routing
- Add per-voice 2x oversampling (upsample control signal, process waveguide at 2x)
- Increase to 4-voice polyphony (default), 8-voice max
- SmoothedValue for parameter smoothing
- **Success criterion:** Breathy, expressive sound. Clean polyphony. No aliasing artifacts.

### Phase 3.3: Impossible Physics + Presets
- Add JetNonlinearity reversed jet blend
- Add SubHarmonics generator
- Add InfiniteSustain (bore loss gain -> 1.0)
- Add StereoWidth (processor-level)
- Create 4 core instrument presets (Concert Flute, Shakuhachi, Bansuri, NAF)
- **Success criterion:** Each preset sounds distinct. Impossible physics params add creative range.

### Phase 3.4: Tier 2 + Expansion + MPE + Tuning
- Implement ToneHoleSystem Tier 2 (Keefe 3-port scattering)
- Add half-holing, cross-fingering support
- Create 4 expansion presets (Recorder, Pan Flute, Piccolo, Ocarina)
- Full MPE implementation (per-note aftertouch, slide, pitch bend)
- TuningEngine integration for Scala/TUN/MTS-ESP
- **Success criterion:** Tier 2 adds audible realism. All 8 presets playable. MPE expressive.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Fractional delay interpolation | Custom delay buffer + interpolation | `juce::dsp::DelayLine<Thiran/Lagrange3rd>` | Thiran allpass is mathematically precise, Lagrange proven for modulation |
| Anti-alias filtering for oversampling | Manual polyphase filter | `juce::dsp::Oversampling<float>` | Polyphase halfband IIR is complex to implement correctly |
| IIR filter design | Bilinear transform by hand | `juce::dsp::IIR::ArrayCoefficients` factory methods | Coefficient calculation is error-prone, JUCE handles it |
| Parameter smoothing | Manual linear interpolation | `juce::SmoothedValue<float>` | Handles edge cases (already-at-target, reset, sample-accurate) |
| Voice allocation | Custom voice stealing | `juce::Synthesiser` | Handles note-on/off, voice stealing, MIDI routing |
| Random noise | `rand()` / custom PRNG | `juce::Random` | Thread-safe, good distribution |
| dB to linear conversion | `pow(10, dB/20)` | `juce::Decibels::decibelsToGain()` | Standard, handles edge cases |

**Key insight:** The only custom DSP is the jet physics (excitation model, nonlinearity, DC blocker) and the tone hole scattering junctions. Everything else uses JUCE primitives.

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| STK cubic `x - x^3` | tanh saturation | Architecture decision | More stable, better harmonics |
| Global oversampling | Per-voice oversampling | JUCE 8 pattern | Isolates feedback per voice |
| `getLatencySamples()` override | `setLatencySamples()` | JUCE 8 | Non-virtual getter, must use setter |
| Raw coefficient arrays | `IIR::ArrayCoefficients` factory | JUCE 7+ | Type-safe, no manual bilinear |
| Manual MIDI parsing | `Synthesiser` + `SynthesiserVoice` | Long-standing | Handles voice allocation automatically |

---

## Open Questions

1. **Oversampled feedback loop: upsample control vs. process all at 2x?**
   - What we know: Upsampling the control signal and processing the waveguide at 2x within the upsampled block is correct
   - What's unclear: Whether the Oversampling object's internal filter state causes artifacts when we overwrite the upsampled buffer with waveguide output
   - Recommendation: Test both approaches in Phase 3.2. Fallback: manual 2x (run loop twice per sample, simple LP on output)

2. **Native American Flute dual-chamber modeling**
   - What we know: NAF has a slow air chamber (SAC) before the main bore
   - What's unclear: Whether a short delay stub adequately models the SAC
   - Recommendation: Start with a simple bore-parameter preset. Add SAC delay stub if needed.

3. **Tier 2 stability during rapid half-holing**
   - What we know: Interpolating between open/closed filter coefficients can produce unstable intermediate states
   - What's unclear: How bad this is in practice
   - Recommendation: Interpolate frequency/Q parameters, then recompute coefficients (not raw coefficient lerp). Test with fast half-hole sweeps.

---

## Environment Availability

Step 2.6: SKIPPED (no external dependencies -- code/config-only changes using existing JUCE 8.0.4 install and CMake build system)

---

## Sources

### Primary (HIGH confidence)
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_DelayLine.h` -- full API and interpolation implementations verified line-by-line
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_Oversampling.h` -- constructor, processSamplesUp/Down, latency API
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_Oversampling.cpp` -- block-based processing implementation
- `/Users/taylorbrook/JUCE/modules/juce_dsp/processors/juce_IIRFilter.h` -- ArrayCoefficients, Coefficients, Filter APIs
- `/Users/taylorbrook/JUCE/modules/juce_audio_basics/utilities/juce_SmoothedValue.h` -- reset, setTargetValue, getNextValue
- `/Users/taylorbrook/JUCE/modules/juce_audio_basics/synthesisers/juce_Synthesiser.h` -- SynthesiserVoice::renderNextBlock signature
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bowed/Source/` -- BowedStringVoice.h/cpp, WaveguideString.h/cpp (proven patterns)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Wind/.planning/research/ARCHITECTURE.md` -- immutable DSP contract

### Secondary (MEDIUM confidence)
- `/Users/taylorbrook/Dev/VST-development/research/flute-waveguide-juce8-implementation.md` -- prior research on JUCE flute implementation
- Instrument preset values (jet gain, noise levels, cutoffs) -- initial estimates requiring experimentation

### Tertiary (LOW confidence)
- Sub-harmonics asymmetric clipping approach -- needs validation that it produces musically useful sub-octaves without destabilizing the waveguide
- Tier 2 scattering junction coefficient values -- need empirical tuning per instrument

---

## Metadata

**Confidence breakdown:**
- JUCE API surface: HIGH -- verified directly from source headers
- Waveguide physics / algorithm: HIGH -- proven architecture, 30+ years of references
- Per-voice oversampling pattern: MEDIUM -- JUCE API is block-based, per-sample feedback integration needs validation
- Instrument preset values: LOW -- initial estimates, require experimentation
- Tier 2 tone hole stability: MEDIUM -- algorithm is well-documented, implementation details need testing

**Research date:** 2026-04-05
**Valid until:** 2026-06-05 (stable JUCE 8.0.4 APIs, no expected changes)
