# Phase 3.1: Core Waveguide + Basic Friction

**Phase:** 2-dsp / 3.1
**Goal:** Single bowed string producing sound with core hyperbolic friction model
**Files to create:** 6 new files, 2 modified files

---

## Task Breakdown

### Task 1: Create DSP Module Files (HyperbolicFriction, BowModel, WaveguideString)

**Creates:**
- `Source/DSP/HyperbolicFriction.h`
- `Source/DSP/BowModel.h`
- `Source/DSP/BowModel.cpp`
- `Source/DSP/WaveguideString.h`
- `Source/DSP/WaveguideString.cpp`

**Dependencies:** None

#### 1A: `Source/DSP/HyperbolicFriction.h` (header-only, stateless)

```cpp
#pragma once
#include <cmath>

class HyperbolicFriction
{
public:
    // Compute reflection coefficient from differential velocity and bow force.
    // STK-style memoryless approach: no iteration, O(1) per sample.
    //
    // v_delta = v_bow - v_string_incoming (already computed by caller)
    // F_bow   = current bow force from BowModel envelope
    //
    // Returns rho in [0, ~0.5] -- bounded, always stable.
    float computeReflectionCoefficient(float v_delta, float F_bow) const noexcept
    {
        float absV = std::abs(v_delta);
        float mu = mu_d + (mu_s - mu_d) * v_0 / (v_0 + absV);
        float r = 0.25f * mu * F_bow / R_s;
        float rho = r / (1.0f + r);
        return rho;
    }

    // ROSIN 0.0 = smooth (v_0 = 0.1), ROSIN 1.0 = aggressive (v_0 = 0.01)
    void setRosin(float rosinParam) noexcept
    {
        v_0 = 0.1f * std::exp(-4.6f * rosinParam);
    }

    void setStringImpedance(float impedance) noexcept
    {
        R_s = impedance;
    }

private:
    float mu_s = 0.8f;   // static friction coefficient
    float mu_d = 0.3f;   // dynamic friction coefficient
    float v_0  = 0.05f;  // characteristic velocity (from ROSIN)
    float R_s  = 0.5f;   // string wave impedance (fixed for Phase 3.1)
};
```

#### 1B: `Source/DSP/BowModel.h` / `BowModel.cpp`

**Header** declares:
```cpp
class BowModel
{
public:
    void prepare(double sampleRate);
    void startBow(float velocity);   // called on note-on
    void stopBow();                   // called on note-off (allowTailOff=true)
    void reset();                     // called on hard stop (allowTailOff=false)
    void updateEnvelope();            // called per-sample
    void setBowSpeed(float speed);    // from APVTS
    void setBowPressure(float pressure); // from APVTS

    float getBowVelocity() const noexcept;
    float getBowForce() const noexcept;
    bool isActive() const noexcept;   // true if bow force > tiny threshold

private:
    double sampleRate = 44100.0;

    // Envelope state
    float v_bow = 0.0f;          // current bow velocity
    float F_bow = 0.0f;          // current bow force
    float v_bow_target = 0.0f;   // target from parameter + velocity
    float F_bow_target = 0.0f;   // target from parameter

    // Smoothing coefficients (one-pole)
    float attackCoeff = 0.0f;
    float releaseCoeff = 0.0f;

    // Parameter values
    float bowSpeedParam = 0.2f;
    float bowPressureParam = 0.5f;

    bool bowActive = false;
};
```

**Implementation details for `BowModel.cpp`:**
- `prepare(sampleRate)`: store sampleRate, compute default release coefficient for 30ms decay (`releaseCoeff = std::exp(-1.0 / (0.030 * sampleRate))`)
- `startBow(velocity)`: map MIDI velocity (0-1 float from JUCE) to bow speed multiplier using `0.2f + 0.8f * velocity`. Set `v_bow_target = bowSpeedParam * multiplier`. Set `F_bow_target = bowPressureParam`. Compute `attackCoeff` from velocity-dependent attack time: `attackTime = 0.050 - 0.045 * velocity` (50ms at vel=0, 5ms at vel=1), `attackCoeff = 1.0f - std::exp(-1.0f / (attackTime * sampleRate))`. Set `bowActive = true`.
- `stopBow()`: set `bowActive = false` (release ramp begins in updateEnvelope)
- `reset()`: zero out v_bow, F_bow, bowActive=false
- `updateEnvelope()`: if bowActive, one-pole smooth toward targets: `v_bow += (v_bow_target - v_bow) * attackCoeff`, same for F_bow. If !bowActive, exponential decay: `v_bow *= releaseCoeff`, `F_bow *= releaseCoeff`.
- `isActive()`: return `bowActive || (std::abs(v_bow) > 1e-7f)`
- `setBowSpeed/setBowPressure`: store param value AND update target if bowActive (so real-time parameter changes take effect during sustain)

#### 1C: `Source/DSP/WaveguideString.h` / `WaveguideString.cpp`

**Header** declares:
```cpp
#include <juce_dsp/juce_dsp.h>

class WaveguideString
{
public:
    void prepare(double sampleRate, int maxBlockSize);
    void trigger(float frequency);      // set delay lengths for new note
    void reset();                        // clear delay lines + filter state

    // Core per-sample processing: takes bow signals, returns output sample.
    // Internally runs steps 2-8 of the algorithm from RESEARCH.md.
    float processSample(float v_bow, float F_bow,
                        const class HyperbolicFriction& friction);

    bool isActive() const noexcept;     // true if energy above threshold

    // Parameter setters (called once per block from voice)
    void setBowPosition(float beta);          // 0.02 - 0.30
    void setBrightness(float cutoffHz);       // 20 - 20000
    void setInfiniteSustain(float amount);    // 0.0 - 1.0

private:
    void updateDelayLengths();
    void updateBridgeFilterCoeffs();

    double sampleRate = 44100.0;
    float currentFrequency = 440.0f;
    float bowPosition = 0.12f;
    float brightnessHz = 8000.0f;
    float infiniteSustain = 0.0f;

    // Delay lines: Thiran interpolation for fractional delay accuracy
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> bridgeDelay { 4410 };
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> neckDelay { 4410 };

    // Bridge loss filter (custom one-pole: H(z) = g*(1-p) / (1 - p*z^-1))
    juce::dsp::IIR::Filter<float> bridgeLossFilter;

    // Cached filter parameters for thread-safe update
    float loopGain = 0.995f;
    float filterPole = 0.0f;

    // Energy tracking for voice cleanup
    float energyEstimate = 0.0f;

    // Flag for pending filter coefficient update
    bool filterDirty = true;
};
```

**Implementation details for `WaveguideString.cpp`:**

- `prepare(sampleRate, maxBlockSize)`:
  - Store sampleRate
  - `int maxDelay = static_cast<int>(sampleRate / 20.0) + 100` (lowest note ~20Hz)
  - Create `ProcessSpec { sampleRate, uint32(maxBlockSize), 1 }`
  - `bridgeDelay.prepare(spec); bridgeDelay.setMaximumDelayInSamples(maxDelay);`
  - Same for neckDelay
  - `bridgeLossFilter.prepare(spec);`
  - `filterDirty = true;`

- `trigger(frequency)`:
  - Store `currentFrequency = frequency`
  - Call `reset()` to clear delay lines (clean start, no artifacts from previous note)
  - Call `updateDelayLengths()` and `updateBridgeFilterCoeffs()`

- `reset()`:
  - `bridgeDelay.reset(); neckDelay.reset(); bridgeLossFilter.reset();`
  - `energyEstimate = 0.0f;`

- `updateDelayLengths()`:
  - `float totalDelay = sampleRate / currentFrequency;`
  - Filter group delay compensation: `float filterGroupDelay = sampleRate / (2.0f * pi * brightnessHz);`
  - `float compensatedDelay = totalDelay - filterGroupDelay;`
  - Split at bow position: `float bridgeSamples = compensatedDelay * bowPosition;`
  - `float neckSamples = compensatedDelay * (1.0f - bowPosition);`
  - Clamp minimum: `bridgeSamples = std::max(2.0f, bridgeSamples);`
  - `neckSamples = std::max(2.0f, neckSamples);`
  - `bridgeDelay.setDelay(bridgeSamples); neckDelay.setDelay(neckSamples);`

- `updateBridgeFilterCoeffs()`:
  - Compute loop gain from INFINITE_SUSTAIN: `float g = 0.990f + 0.010f * infiniteSustain;`
  - Compute pole from brightness: `float p = std::exp(-2.0f * pi * brightnessHz / sampleRate);`
  - Create custom one-pole coefficients: `b0 = g*(1-p), b1 = 0, a0 = 1, a1 = -p`
  - `auto coeffs = new juce::dsp::IIR::Coefficients<float>(g * (1.0f - p), 0.0f, 1.0f, -p);`
  - `*bridgeLossFilter.coefficients = *coeffs;`
  - `filterDirty = false;`

- `processSample(v_bow, F_bow, friction)`:
  - If `filterDirty`, call `updateBridgeFilterCoeffs()` (thread-safe: only runs on audio thread)
  - Step 2: `float bridgeReflection = -bridgeLossFilter.processSample(bridgeDelay.popSample(0));`
  - `float nutReflection = -neckDelay.popSample(0);` (sign inversion = hard boundary)
  - Step 3: `float v_string_incoming = bridgeReflection + nutReflection;`
  - Step 4: `float v_delta = v_bow - v_string_incoming;`
  - Step 5: `float rho = friction.computeReflectionCoefficient(v_delta, F_bow);`
  - Step 6: `float newVelocity = v_delta * rho;`
  - Step 7: `bridgeDelay.pushSample(0, bridgeReflection + newVelocity);`
  - `neckDelay.pushSample(0, nutReflection + newVelocity);`
  - Step 8: output = `bridgeReflection + newVelocity` (bridge end signal)
  - Energy tracking: `energyEstimate = 0.999f * energyEstimate + 0.001f * std::abs(output);`
  - Denormal flush: `if (std::abs(output) < 1e-15f) output = 0.0f;`
  - Return output

- `setBowPosition(beta)`: store, call `updateDelayLengths()`
- `setBrightness(cutoffHz)`: store, set `filterDirty = true`, call `updateDelayLengths()` (because filter group delay changes)
- `setInfiniteSustain(amount)`: store, set `filterDirty = true`
- `isActive()`: return `energyEstimate > 1e-7f`

---

### Task 2: Create BowedStringVoice and Wire to Processor

**Creates:**
- `Source/BowedStringVoice.h`
- `Source/BowedStringVoice.cpp`

**Modifies:**
- `Source/PluginProcessor.h`
- `Source/PluginProcessor.cpp`
- `CMakeLists.txt`

**Dependencies:** Task 1 (all DSP modules must exist)

#### 2A: `Source/BowedStringVoice.h` / `BowedStringVoice.cpp`

**Header** declares:
```cpp
#pragma once
#include <juce_audio_basics/juce_audio_basics.h>
#include <juce_audio_processors/juce_audio_processors.h>
#include "DSP/WaveguideString.h"
#include "DSP/BowModel.h"
#include "DSP/HyperbolicFriction.h"

class BowedStringVoice : public juce::SynthesiserVoice
{
public:
    BowedStringVoice(juce::AudioProcessorValueTreeState* apvts);

    bool canPlaySound(juce::SynthesiserSound* sound) override;
    void startNote(int midiNoteNumber, float velocity,
                   juce::SynthesiserSound* sound, int currentPitchWheelPosition) override;
    void stopNote(float velocity, bool allowTailOff) override;
    void pitchWheelMoved(int newPitchWheelValue) override;
    void controllerMoved(int controllerNumber, int newControllerValue) override;
    void prepareToPlay(double sampleRate, int maxBlockSize);
    void renderNextBlock(juce::AudioBuffer<float>& outputBuffer,
                         int startSample, int numSamples) override;

private:
    void updateParametersFromAPVTS();

    juce::AudioProcessorValueTreeState* parameters = nullptr;

    WaveguideString waveguideString;
    BowModel bowModel;
    HyperbolicFriction frictionModel;

    float outputGainLinear = 1.0f;
    float currentFrequency = 440.0f;
};
```

**Implementation details for `BowedStringVoice.cpp`:**

- Constructor: store APVTS pointer
- `canPlaySound`: return `dynamic_cast<BowedStringSound*>(sound) != nullptr`
- `startNote(midiNote, velocity, sound, pitchWheel)`:
  - `currentFrequency = juce::MidiMessage::getMidiNoteInHertz(midiNote);`
  - `waveguideString.trigger(currentFrequency);`
  - `bowModel.startBow(velocity);`
- `stopNote(velocity, allowTailOff)`:
  - If allowTailOff: `bowModel.stopBow();` (voice stays active until energy decays)
  - Else: `clearCurrentNote(); waveguideString.reset(); bowModel.reset();`
- `pitchWheelMoved`: ignore for Phase 3.1 (no portamento yet)
- `controllerMoved`: ignore for Phase 3.1 (no MPE routing yet)
- `prepareToPlay(sampleRate, maxBlockSize)`:
  - `waveguideString.prepare(sampleRate, maxBlockSize);`
  - `bowModel.prepare(sampleRate);`
- `renderNextBlock(outputBuffer, startSample, numSamples)`:
  - `updateParametersFromAPVTS();`
  - Check if voice should be cleared: `if (!bowModel.isActive() && !waveguideString.isActive()) { clearCurrentNote(); return; }`
  - Per-sample loop: `while (--numSamples >= 0)`:
    - `bowModel.updateEnvelope();`
    - `float v_bow = bowModel.getBowVelocity();`
    - `float F_bow = bowModel.getBowForce();`
    - `float sample = waveguideString.processSample(v_bow, F_bow, frictionModel);`
    - `sample *= outputGainLinear;`
    - Write to all channels: `for (int ch = outputBuffer.getNumChannels(); --ch >= 0;) outputBuffer.addSample(ch, startSample, sample);`
    - `++startSample;`

- `updateParametersFromAPVTS()`:
  - Read Phase 3.1 active parameters atomically:
    - `bowSpeed` -> `bowModel.setBowSpeed(val)`
    - `bowPressure` -> `bowModel.setBowPressure(val)`
    - `bowPosition` -> `waveguideString.setBowPosition(val)`
    - `rosin` -> `frictionModel.setRosin(val)`
    - `brightness` -> `waveguideString.setBrightness(val)`
    - `infiniteSustain` -> `waveguideString.setInfiniteSustain(val)`
    - `outputLevel` -> `outputGainLinear = juce::Decibels::decibelsToGain(val)`
  - Parameters NOT wired in Phase 3.1 (exist in APVTS but have no effect): bodyMaterial, bodySize, stringCount, stringTuning1-4, sympatheticAmount, sympatheticCount, width, reversedFriction, subHarmonics, referencePitch, tuningSystem

#### 2B: Modify `Source/PluginProcessor.h`

Add include at top:
```cpp
#include "BowedStringVoice.h"
```

No other header changes needed -- synthesiser member already exists.

#### 2C: Modify `Source/PluginProcessor.cpp`

**Constructor** -- after `synthesiser.addSound(new BowedStringSound());`, add:
```cpp
// Add 8 polyphonic voices
for (int i = 0; i < 8; ++i)
    synthesiser.addVoice(new BowedStringVoice(&parameters));
```

**prepareToPlay** -- after `synthesiser.setCurrentPlaybackSampleRate(sampleRate);`, add:
```cpp
for (int i = 0; i < synthesiser.getNumVoices(); ++i)
{
    if (auto* voice = dynamic_cast<BowedStringVoice*>(synthesiser.getVoice(i)))
        voice->prepareToPlay(sampleRate, samplesPerBlock);
}
```

**processBlock** -- replace `buffer.clear()` with:
```cpp
// Clear all output channels
for (auto i = getTotalNumInputChannels(); i < getTotalNumOutputChannels(); ++i)
    buffer.clear(i, 0, buffer.getNumSamples());

// Render all active voices
synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());
```

Remove the `juce::ignoreUnused(midiMessages);` line since midiMessages is now used.

#### 2D: Update `CMakeLists.txt`

Add the new source files to `target_sources`. The block should become:
```cmake
target_sources(O-Bowed
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/BowedStringSound.h
        Source/BowedStringVoice.h
        Source/BowedStringVoice.cpp
        Source/DSP/WaveguideString.h
        Source/DSP/WaveguideString.cpp
        Source/DSP/BowModel.h
        Source/DSP/BowModel.cpp
        Source/DSP/HyperbolicFriction.h
        # Tuning module files (referenced from shared module)
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningEngine.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/ScaleGenerator.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/EmbeddedTunings.cpp
        ${CMAKE_SOURCE_DIR}/modules/tuning/scala-tuning-engine/cpp/TuningExporter.cpp
)
```

---

## Success Criteria Mapping

| Criterion | Addressed By |
|-----------|-------------|
| Plugin loads in DAW as instrument (not effect) | Already true from Stage 1 (IS_SYNTH TRUE, output-only buses) -- no change needed |
| MIDI note-on produces audible bowed string tone | Task 2: BowedStringVoice.startNote -> waveguide.trigger + bowModel.startBow, processBlock calls synthesiser.renderNextBlock |
| BOW_SPEED affects volume | Task 1B: BowModel maps bowSpeedParam to v_bow_target, which scales excitation amplitude |
| BOW_PRESSURE affects tone quality | Task 1B+1A: BowModel provides F_bow to friction; higher F_bow -> larger rho -> more energy injection -> richer harmonics |
| BOW_POSITION changes timbre | Task 1C: WaveguideString splits delay at bowPosition; near bridge (low beta) = ponticello, far = tasto |
| ROSIN changes friction aggressiveness | Task 1A: HyperbolicFriction.setRosin maps param to v_0 characteristic velocity |
| BRIGHTNESS controls high-frequency content | Task 1C: WaveguideString bridge loss filter pole from brightnessHz; lower cutoff = warmer tone |
| Note-off produces natural decay | Task 1B+2A: BowModel.stopBow starts release ramp, voice stays active until waveguide energy decays below threshold |
| No clicks or pops during parameter changes | Task 1C: filterDirty flag defers coefficient update to audio thread; delay lengths update smoothly; one-pole envelope smoothing in BowModel |
| Sustained tone is stable | Task 1A: rho bounded in [0, 0.5]; Task 1C: loopGain < 1.0 at infiniteSustain=0; energy cannot grow unbounded |

---

## Build Verification Points

### After Task 1 (DSP modules created)
**Build check only** -- no functional test possible yet since voices aren't wired.
```bash
cd /Users/taylorbrook/Dev/VST-development/build && cmake .. -G Ninja && ninja O-Bowed_VST3 2>&1 | tail -20
```
**Expected:** Compiles with zero errors. Warnings acceptable but no errors in new DSP files.

### After Task 2 (full wiring complete)
**Full functional test:**
1. Build:
```bash
cd /Users/taylorbrook/Dev/VST-development/build && ninja O-Bowed_VST3 O-Bowed_AU
```
2. Install to system (per CLAUDE.md protocol):
```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Bowed.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Bowed.component
cp -R build/plugins/O-Bowed/O-Bowed_artefacts/Release/VST3/O-Bowed.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-Bowed/O-Bowed_artefacts/Release/AU/O-Bowed.component ~/Library/Audio/Plug-Ins/Components/
```
3. Verify AU registration: `auval -a | grep -i bowed`
4. Open in DAW, play MIDI notes, verify all 10 test criteria

---

## Risk Areas

### 1. Silent Output (No Sound)
**Cause:** v_bow or F_bow stays at zero, or waveguide delay lines never get seeded.
**Detection:** Play a MIDI note -- if silence, add a `DBG()` in renderNextBlock to print v_bow, F_bow, and output sample values for the first 100 samples.
**Fix:** Verify BowModel.startBow is receiving nonzero velocity. Verify APVTS parameter IDs match exactly ("bowSpeed" not "bow_speed"). Verify synthesiser.renderNextBlock is being called (not buffer.clear).

### 2. Runaway / Clipping
**Cause:** loopGain >= 1.0 (energy grows each cycle), or rho too large.
**Detection:** Output clips to +/-1.0 or DAW meters hit red immediately.
**Fix:** Verify `g = 0.990 + 0.010 * infiniteSustain` never exceeds 1.0 when infiniteSustain is at max. Verify rho is bounded by `r/(1+r)` which is always < 1.0. Add a safety hard-clip: `sample = juce::jlimit(-2.0f, 2.0f, sample)` after output gain.

### 3. Pitch Inaccuracy
**Cause:** Filter group delay not compensated, or compensated incorrectly.
**Detection:** Play A4 (MIDI 69), measure output frequency with tuner -- should be 440Hz +/- 2 cents.
**Fix:** Double-check `filterGroupDelay = sampleRate / (2*pi*cutoffHz)` formula. Verify it is subtracted from totalDelay BEFORE splitting at bow position.

### 4. Clicks on Parameter Changes
**Cause:** IIR filter coefficients updated mid-buffer from wrong thread, or delay line length jumps.
**Detection:** Sweep BRIGHTNESS or BOW_POSITION while holding a note -- listen for clicks.
**Fix:** The `filterDirty` flag ensures coefficients are updated on audio thread only. If delay length changes click, add one-pole smoothing to the delay length (but for Phase 3.1 this is unlikely since Thiran handles moderate changes).

### 5. Denormal CPU Spikes
**Cause:** Voice not cleared after note-off + decay.
**Detection:** Play and release many notes, watch CPU meter climb.
**Fix:** `ScopedNoDenormals` is already in processBlock. The energy-based `isActive()` check in WaveguideString with threshold 1e-7f should trigger voice cleanup. If not, lower threshold or add explicit denormal flush in the per-sample loop (already included in WaveguideString.processSample spec above).

### 6. IIR Coefficient Thread Safety
**Cause:** Updating `bridgeLossFilter.coefficients` while audio thread is reading them.
**Detection:** Random glitches or crashes during parameter automation.
**Fix:** The `filterDirty` flag pattern ensures all coefficient updates happen inside `processSample` (on the audio thread). Parameter setters only set `filterDirty = true`. This is the O-Lyrica v1.7.10 pattern.

---

## Implementation Order Summary

```
Task 1: Create all DSP module files
  1A. Source/DSP/HyperbolicFriction.h     (header-only, ~40 lines)
  1B. Source/DSP/BowModel.h + .cpp        (~60 + ~100 lines)
  1C. Source/DSP/WaveguideString.h + .cpp  (~60 + ~150 lines)
  [BUILD CHECK]

Task 2: Create voice + wire processor
  2A. Source/BowedStringVoice.h + .cpp     (~40 + ~100 lines)
  2B. Modify PluginProcessor.h             (add 1 include)
  2C. Modify PluginProcessor.cpp           (add voice creation + renderNextBlock)
  2D. Update CMakeLists.txt                (add 6 source files)
  [BUILD + INSTALL + DAW TEST]
```

**Estimated Claude execution time:** 30-45 minutes total (Task 1: ~20 min, Task 2: ~15 min)

---

## Reference Files

- **Algorithm spec:** `plugins/O-Bowed/.planning/stages/2-dsp/RESEARCH.md` (per-sample algorithm, JUCE API patterns, pitfalls)
- **Architecture contract:** `plugins/O-Bowed/.planning/research/ARCHITECTURE.md` (immutable DSP architecture)
- **O-Lyrica reference:** `plugins/O-Lyrica/Source/DSP/WaveguideString.h/.cpp` and `plugins/O-Lyrica/Source/HarpSynthVoice.h/.cpp` (proven patterns for delay line usage, APVTS reading, voice lifecycle)
- **Existing processor:** `plugins/O-Bowed/Source/PluginProcessor.h/.cpp` (current state with synthesiser member, APVTS, all 21 parameters)
