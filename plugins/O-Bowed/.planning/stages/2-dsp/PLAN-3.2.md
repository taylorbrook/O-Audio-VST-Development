# Phase 3.2: Body Resonator + Stereo Width

**Phase:** 2-dsp / 3.2
**Goal:** Morphable body resonator gives instrument identity, stereo output with width control
**Files to create:** 3 new files, 3 modified files

---

## Task Breakdown

### Task 1: Create BodyResonator DSP Class

**Creates:**
- `Source/DSP/BodyResonator.h`
- `Source/DSP/BodyResonator.cpp`

**Dependencies:** None

#### 1A: `Source/DSP/BodyResonator.h`

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>

class BodyResonator
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Called from processBlock with current APVTS values
    void setMaterial (float material);   // 0.0 - 1.0
    void setSize (float size);           // 0.0 - 1.0

    // Per-sample processing (parallel biquad bank)
    float process (float input);

private:
    static constexpr int NUM_MODES = 8;
    static constexpr int NUM_PRESETS = 4;

    struct ModePreset {
        float freq[NUM_MODES];
        float q[NUM_MODES];
        float gainDb[NUM_MODES];
    };

    static const ModePreset presets[NUM_PRESETS];

    // Filter bank
    std::array<juce::dsp::IIR::Filter<float>, NUM_MODES> bodyModes;

    // Current state
    double currentSampleRate = 44100.0;
    float currentMaterial = -1.0f;   // -1 forces initial update
    float currentSize = -1.0f;

    // Normalization
    float normGain = 1.0f;
    static constexpr float DRY_MIX = 0.4f;
    static constexpr float WET_MIX = 0.6f;

    // Precomputed gain sums for normalization
    float presetGainSums[NUM_PRESETS] {};
    float referenceGainSum = 1.0f;

    void updateCoefficients();
    static float computePresetGainSum (const ModePreset& preset);
};
```

#### 1B: `Source/DSP/BodyResonator.cpp`

**Implementation details:**

- **Static preset data** (from ARCHITECTURE.md):
```cpp
const BodyResonator::ModePreset BodyResonator::presets[NUM_PRESETS] = {
    // Membrane (erhu-like)
    { { 2000.0f, 6000.0f, 10000.0f, 14000.0f, 600.0f, 1200.0f, 3500.0f, 8000.0f },
      { 8.0f, 6.0f, 4.0f, 3.0f, 5.0f, 6.0f, 4.0f, 3.0f },
      { 12.0f, 8.0f, 5.0f, 3.0f, 6.0f, 4.0f, 3.0f, 2.0f } },
    // Wood (violin)
    { { 272.0f, 462.0f, 551.0f, 2500.0f, 1200.0f, 3200.0f, 6000.0f, 800.0f },
      { 12.0f, 10.0f, 10.0f, 3.0f, 5.0f, 4.0f, 2.0f, 8.0f },
      { 10.0f, 14.0f, 12.0f, 8.0f, 4.0f, 3.0f, 2.0f, 6.0f } },
    // Metal
    { { 440.0f, 1123.0f, 1872.0f, 3100.0f, 680.0f, 1560.0f, 2400.0f, 5200.0f },
      { 25.0f, 20.0f, 15.0f, 12.0f, 18.0f, 14.0f, 10.0f, 8.0f },
      { 8.0f, 6.0f, 5.0f, 4.0f, 7.0f, 5.0f, 3.0f, 2.0f } },
    // Glass
    { { 800.0f, 2400.0f, 4800.0f, 7200.0f, 1200.0f, 3600.0f, 6000.0f, 9600.0f },
      { 30.0f, 25.0f, 20.0f, 15.0f, 28.0f, 22.0f, 18.0f, 12.0f },
      { 10.0f, 8.0f, 6.0f, 4.0f, 9.0f, 7.0f, 5.0f, 3.0f } }
};
```

- **`prepare(sampleRate, maxBlockSize)`:**
  - Store sampleRate
  - Create `ProcessSpec { sampleRate, uint32(maxBlockSize), 1 }`
  - For each of 8 bodyModes: `filter.prepare(spec); filter.reset();`
  - Precompute `presetGainSums[i]` for each preset via `computePresetGainSum`
  - Set `referenceGainSum = presetGainSums[1]` (wood preset as reference)
  - Set `currentMaterial = -1.0f; currentSize = -1.0f;` to force initial coefficient update
  
- **`reset()`:**
  - For each filter: `filter.reset();`
  
- **`computePresetGainSum(preset)`:**
  - Sum `Decibels::decibelsToGain(preset.gainDb[i])` for all 8 modes
  - Return sum

- **`setMaterial(material)` / `setSize(size)`:**
  - Epsilon check (0.001f threshold), skip if unchanged
  - Store value
  - Call `updateCoefficients()`

- **`updateCoefficients()`:**
  - Guard: `if (currentMaterial < 0.0f || currentSize < 0.0f) return;`
  - Determine two adjacent presets:
    - `float scaled = currentMaterial * (NUM_PRESETS - 1);`
    - `int idxA = jlimit(0, NUM_PRESETS - 2, (int)scaled);`
    - `int idxB = idxA + 1;`
    - `float t = scaled - (float)idxA;`
  - For each of 8 modes:
    - Log-domain frequency interpolation: `freq = exp(log(freqA) * (1-t) + log(freqB) * t)`
    - Linear Q interpolation: `q = qA * (1-t) + qB * t`
    - Linear gain interpolation (in dB): `gainDb = gainDbA * (1-t) + gainDbB * t`
    - Body size scaling: `freq *= pow(2.0f, (0.5f - currentSize) * 3.0f)`
    - Nyquist clamp: `freq = jlimit(20.0f, (float)(currentSampleRate * 0.45), freq)`
    - Q clamp: `q = std::max(0.1f, q)`
    - Convert gain: `gainLinear = Decibels::decibelsToGain(gainDb)`
    - Apply: `bodyModes[i].coefficients = IIR::Coefficients<float>::makePeakFilter(currentSampleRate, freq, q, gainLinear);`
  - Compute normalization:
    - `float morphedSum = presetGainSums[idxA] * (1-t) + presetGainSums[idxB] * t;`
    - `normGain = referenceGainSum / std::max(0.01f, morphedSum);`

- **`process(input)`:**
  - `float resonant = 0.0f;`
  - `for (auto& filter : bodyModes) resonant += filter.processSample(input);`
  - `resonant *= normGain;`
  - `return input * DRY_MIX + resonant * WET_MIX;`

---

### Task 2: Create StereoWidthProcessor Class

**Creates:**
- `Source/DSP/StereoWidthProcessor.h` (header-only)

**Dependencies:** None (can be done in parallel with Task 1)

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>

class StereoWidthProcessor
{
public:
    void prepare (double sampleRate, int maxBlockSize)
    {
        juce::dsp::ProcessSpec spec {
            sampleRate,
            static_cast<juce::uint32> (maxBlockSize),
            1
        };
        decorrelator.prepare (spec);
        decorrelator.coefficients =
            juce::dsp::IIR::Coefficients<float>::makeAllPass (sampleRate, 800.0f, 0.7f);
        widthSmoothed.reset (sampleRate, 0.02);  // 20ms smoothing
    }

    void reset()
    {
        decorrelator.reset();
        widthSmoothed.reset (0);
    }

    // Process stereo buffer in-place.
    // Input: channel 0 has mono body resonator output, channel 1 is ignored (will be overwritten).
    // Output: decorrelated stereo with width applied.
    void processBlock (juce::AudioBuffer<float>& buffer, float widthFactor)
    {
        if (buffer.getNumChannels() < 2)
            return;

        widthSmoothed.setTargetValue (widthFactor);

        auto* leftData  = buffer.getWritePointer (0);
        auto* rightData = buffer.getWritePointer (1);

        for (int i = 0; i < buffer.getNumSamples(); ++i)
        {
            float mono = leftData[i];

            // Create stereo via allpass decorrelation on R channel
            float left  = mono;
            float right = decorrelator.processSample (mono);

            // Mid-side width processing
            float w = widthSmoothed.getNextValue();
            float mid  = (left + right) * 0.5f;
            float side = (left - right) * 0.5f;
            side *= w;

            leftData[i]  = mid + side;
            rightData[i] = mid - side;
        }
    }

private:
    juce::dsp::IIR::Filter<float> decorrelator;
    juce::SmoothedValue<float> widthSmoothed { 1.0f };
};
```

---

### Task 3: Wire BodyResonator + StereoWidthProcessor to Processor

**Modifies:**
- `Source/PluginProcessor.h`
- `Source/PluginProcessor.cpp`
- `CMakeLists.txt`

**Dependencies:** Tasks 1, 2

#### 3A: Modify `Source/PluginProcessor.h`

Add includes at top (after existing includes):
```cpp
#include "DSP/BodyResonator.h"
#include "DSP/StereoWidthProcessor.h"
```

Add members (after `TuningEngine tuningEngine;`):
```cpp
// Body resonator (processor-level, shared by all voices)
BodyResonator bodyResonator;
StereoWidthProcessor stereoWidthProcessor;
```

#### 3B: Modify `Source/PluginProcessor.cpp`

**prepareToPlay** -- add after the voice preparation loop:
```cpp
bodyResonator.prepare (sampleRate, samplesPerBlock);
stereoWidthProcessor.prepare (sampleRate, samplesPerBlock);
```

**processBlock** -- replace current body after `synthesiser.renderNextBlock`:
```cpp
// Read body and stereo params from APVTS
float material = parameters.getRawParameterValue ("bodyMaterial")->load();
float bodySize = parameters.getRawParameterValue ("bodySize")->load();
float width    = parameters.getRawParameterValue ("width")->load();

// Update body resonator coefficients (checks for changes internally)
bodyResonator.setMaterial (material);
bodyResonator.setSize (bodySize);

// Process body resonator on channel 0 (mono voice sum)
auto* channelData = buffer.getWritePointer (0);
for (int i = 0; i < buffer.getNumSamples(); ++i)
    channelData[i] = bodyResonator.process (channelData[i]);

// Create stereo and apply width
stereoWidthProcessor.processBlock (buffer, width);
```

**Note:** The voice currently writes the same sample to both channels. After body resonator processes channel 0, channel 1 still has the old unprocessed voice output. The `StereoWidthProcessor.processBlock` overwrites channel 1 with the decorrelated signal, so this is correct -- channel 1's old content is discarded.

#### 3C: Update `CMakeLists.txt`

Add to `target_sources`:
```cmake
Source/DSP/BodyResonator.h
Source/DSP/BodyResonator.cpp
Source/DSP/StereoWidthProcessor.h
```

---

## Success Criteria Mapping

| Criterion | Addressed By |
|-----------|-------------|
| BODY_MATERIAL morph: membrane → wood → metal → glass audibly different | Task 1: 4 preset banks with distinct frequency/Q/gain profiles, log-domain frequency interpolation for smooth transitions |
| BODY_SIZE morph: small (bright) → large (deep) frequency shift audible | Task 1: `freq *= pow(2.0, (0.5 - bodySize) * 3.0)` scaling, 3-octave total range |
| Smooth morph without clicks or level jumps | Task 1: parameter interpolation (not raw coefficient lerp), per-preset normalization, epsilon guard on setMaterial/setSize |
| Stereo output with WIDTH parameter working (mono at 0%, stereo at 100%, wide at 200%) | Task 2: allpass decorrelator + M/S width processing with SmoothedValue |
| Body resonator does not cause feedback or instability | Task 1: makePeakFilter guarantees stable coefficients; parallel topology (not series) cannot self-oscillate; Nyquist clamp prevents assertion failures |
| Material=Wood + Size=Small sounds violin-like | Task 1: Wood preset from ARCHITECTURE.md (272, 462, 551 Hz modes = violin A0, B1-, B1+), Size=0 shifts up ~1.5 octaves |
| Material=Membrane + Size=Medium sounds erhu-like | Task 1: Membrane preset (odd-harmonic emphasis at 2000, 6000, 10000 Hz), Size=0.5 = no shift |

---

## Build Verification Points

### After Task 1 + 2 (DSP classes created, before wiring)
**Build check only** -- new files are created but not yet referenced by processor.
```bash
cd /Users/taylorbrook/Dev/VST-development/build && cmake .. -G Ninja && ninja O-Bowed_VST3 2>&1 | tail -20
```
**Expected:** Compiles with zero errors. New DSP files are listed in CMakeLists.txt but their classes aren't called yet.

Note: Actually, CMakeLists.txt must be updated first for the new files to compile. So Task 3C (CMake update) should be done together with Tasks 1+2 for the build check.

### After Task 3 (full wiring complete)
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
4. Open in DAW, play MIDI notes, verify all 7 test criteria

---

## Risk Areas

### 1. Level Jump During Material Morph
**Cause:** Different presets have different total energy; normalization formula doesn't account for frequency-dependent interaction.
**Detection:** Sweep BODY_MATERIAL from 0 to 1 while holding a note -- listen for volume jumps at preset boundaries (0.33, 0.66).
**Fix:** If normalization is insufficient, switch to a fixed divisor (e.g., 4.0) tuned by ear. Or add SmoothedValue on normGain.

### 2. Nyquist Assertion at Extreme Size + Glass Preset
**Cause:** Glass mode 8 at 9600 Hz * 2.83x (size=0) = 27168 Hz > Nyquist.
**Detection:** Set Material=1.0 (glass), Size=0.0 (small) -- if crash/assert, the clamp isn't working.
**Fix:** The Nyquist clamp `jlimit(20.0f, sampleRate * 0.45f, freq)` handles this. Verify it's applied AFTER size scaling.

### 3. makePeakFilter gainFactor Confusion
**Cause:** ARCHITECTURE.md specifies gains in dB but `makePeakFilter` expects LINEAR gain factor.
**Detection:** Body sounds wrong -- either barely audible (forgot to convert) or massively over-boosted.
**Fix:** Always convert: `gainLinear = Decibels::decibelsToGain(gainDb)`. +12 dB → 3.98, not 12.0.

### 4. Phase Cancellation from Decorrelator
**Cause:** Allpass creates frequency-dependent phase difference. When summed to mono (e.g., mono monitoring), some frequencies may partially cancel.
**Detection:** Set WIDTH=0 (mono) and compare to Phase 3.1 output. Should sound identical (M/S at width=0 collapses to mono correctly).
**Fix:** This is inherent to decorrelation but at WIDTH=0, the M/S math produces `L = R = mid`, so the decorrelator's phase difference is irrelevant. Only at WIDTH > 0 does stereo appear.

### 5. Body Resonator Adds Unwanted Latency
**Cause:** IIR filters have no algorithmic latency (causal). This is a non-risk.
**Detection:** N/A
**Fix:** N/A -- parallel biquads are zero-latency.

### 6. CPU Spike on Coefficient Update
**Cause:** 8 × makePeakFilter calls involve trig functions.
**Detection:** Monitor CPU during rapid Material/Size automation.
**Fix:** Epsilon guards in setMaterial/setSize prevent updates on negligible changes. If still problematic, rate-limit updates to once per block (already the case since params are read once per processBlock call).

---

## Implementation Order Summary

```
Task 1: Create BodyResonator class
  1A. Source/DSP/BodyResonator.h        (~55 lines)
  1B. Source/DSP/BodyResonator.cpp      (~130 lines)

Task 2: Create StereoWidthProcessor class (parallel with Task 1)
  2A. Source/DSP/StereoWidthProcessor.h  (~55 lines, header-only)

Task 3: Wire to processor + update CMake
  3A. Modify PluginProcessor.h           (add 2 includes + 2 members)
  3B. Modify PluginProcessor.cpp         (add prepare calls + processBlock body)
  3C. Update CMakeLists.txt              (add 3 source files)
  [BUILD + INSTALL + DAW TEST]
```

---

## Reference Files

- **Research:** `plugins/O-Bowed/.planning/stages/2-dsp/RESEARCH-3.2.md` (makePeakFilter API, morph algorithm, decorrelation, thread safety)
- **Architecture contract:** `plugins/O-Bowed/.planning/research/ARCHITECTURE.md` (body resonator spec, preset data, stereo width spec)
- **Phase 3.1 plan:** `plugins/O-Bowed/.planning/stages/2-dsp/PLAN.md` (existing voice + waveguide architecture)
- **O-Prism stereo width:** `plugins/O-Prism/Source/PluginProcessor.cpp:634-656` (proven M/S width pattern with SmoothedValue)
- **Existing processor:** `plugins/O-Bowed/Source/PluginProcessor.h/.cpp` (current processBlock, APVTS with bodyMaterial/bodySize/width params)
