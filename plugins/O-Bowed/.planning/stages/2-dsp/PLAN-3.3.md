# Phase 3.3: Multi-String + Sympathetic Coupling

**Phase:** 2-dsp / 3.3
**Goal:** Processor-level drone strings (1-4), passive sympathetic KS waveguides (0-12), body resonator stereo refactor, per-string panning, dynamic voice cap
**Files to create:** 4 new files, 5 modified files

---

## Task Breakdown

### Task 1: Refactor BodyResonator to Stereo

**Modifies:**
- `Source/DSP/BodyResonator.h`
- `Source/DSP/BodyResonator.cpp`

**Dependencies:** None

#### 1A: `Source/DSP/BodyResonator.h` changes

Add a second filter array and new stereo API:

```cpp
// Replace single filter bank:
std::array<juce::dsp::IIR::Filter<float>, NUM_MODES> bodyModes;

// With stereo pair:
std::array<juce::dsp::IIR::Filter<float>, NUM_MODES> bodyModesL;
std::array<juce::dsp::IIR::Filter<float>, NUM_MODES> bodyModesR;
```

Add new method:
```cpp
void processStereo (float& left, float& right);
```

Keep the old `process(float)` as deprecated but functional (calls processStereo internally with mono in/out) to avoid breaking anything during transition. Actually — the old `process()` is only called in PluginProcessor.cpp which we'll modify in Task 5. Remove `process()` entirely and only have `processStereo()`.

#### 1B: `Source/DSP/BodyResonator.cpp` changes

- `prepare()`: Prepare both `bodyModesL` and `bodyModesR` with the ProcessSpec (still numChannels=1 per filter since each filter is mono)
- `reset()`: Reset both arrays
- `updateCoefficients()`: Set the same coefficient pointer on both L and R arrays:
  ```cpp
  auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(...);
  bodyModesL[i].coefficients = coeffs;
  bodyModesR[i].coefficients = coeffs;
  ```
- Replace `process(float)` with `processStereo(float& left, float& right)`:
  ```cpp
  void BodyResonator::processStereo (float& left, float& right)
  {
      float resonantL = 0.0f;
      float resonantR = 0.0f;
      for (int i = 0; i < NUM_MODES; ++i)
      {
          resonantL += bodyModesL[i].processSample (left);
          resonantR += bodyModesR[i].processSample (right);
      }
      resonantL *= normGain;
      resonantR *= normGain;
      left  = left  * DRY_MIX + resonantL * WET_MIX;
      right = right * DRY_MIX + resonantR * WET_MIX;
  }
  ```

**CPU impact:** +8 biquad evaluations per sample (~80 FLOPs). Negligible.

**Why separate filter instances:** IIR filters have state (z1, z2). Each channel needs its own filter state. Shared coefficients + separate state is the standard pattern.

---

### Task 2: Create DroneStringEngine

**Creates:**
- `Source/DSP/DroneStringEngine.h`
- `Source/DSP/DroneStringEngine.cpp`

**Dependencies:** None (uses existing WaveguideString, BowModel, HyperbolicFriction)

#### 2A: `Source/DSP/DroneStringEngine.h`

```cpp
#pragma once
#include "WaveguideString.h"
#include "BowModel.h"
#include "HyperbolicFriction.h"
#include <array>

class DroneStringEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Parameter updates (called once per processBlock)
    void setStringCount (int count);              // 1-4
    void setTuning (int stringIndex, float cents); // ±2400 cents from reference
    void setReferencePitch (float hz);            // A4 reference (220-880)
    void setBowSpeed (float speed);
    void setBowPressure (float pressure);
    void setBowPosition (float position);
    void setRosin (float rosin);
    void setBrightness (float brightness);
    void setInfiniteSustain (float sustain);

    // Per-sample stereo output
    void processSample (float& outL, float& outR);

    // Bridge output for sympathetic excitation (mono sum of all drones pre-pan)
    float getLastBridgeOutput() const noexcept { return lastBridgeOutput; }

private:
    static constexpr int MAX_DRONES = 4;
    static constexpr float HALF_PI = 1.5707963f;

    struct DroneString {
        WaveguideString waveguide;
        BowModel bow;
        HyperbolicFriction friction;
        float panL = 0.707f;
        float panR = 0.707f;
        float speedVariation = 1.0f;
        float pressureVariation = 1.0f;
        float targetFrequency = 440.0f;
        bool active = false;
    };

    std::array<DroneString, MAX_DRONES> drones;
    int activeCount = 0;
    int pendingCount = 0;         // for crossfade on count change
    int crossfadeSamples = 0;     // countdown for crossfade
    static constexpr int CROSSFADE_LENGTH = 64;

    float referencePitch = 440.0f;
    float tuningCents[MAX_DRONES] = { 0.0f, 0.0f, 0.0f, 0.0f };

    // Shared bow params (before per-string variation)
    float sharedBowSpeed = 0.2f;
    float sharedBowPressure = 0.5f;

    float lastBridgeOutput = 0.0f;
    double currentSampleRate = 44100.0;

    void updatePanPositions();
    void initVariations();
    void updateDroneFrequency (int index);
    void activateDrone (int index);
    void deactivateDrone (int index);

    // Pan lookup: [stringCount-1][stringIndex] -> pan position (0.0=L, 1.0=R)
    static constexpr float panTable[4][4] = {
        { 0.5f,  0.0f,  0.0f,  0.0f },   // 1 string: center
        { 0.35f, 0.65f, 0.0f,  0.0f },   // 2 strings
        { 0.25f, 0.50f, 0.75f, 0.0f },   // 3 strings
        { 0.20f, 0.40f, 0.60f, 0.80f }   // 4 strings
    };
};
```

#### 2B: `Source/DSP/DroneStringEngine.cpp`

**Key implementation details:**

- **`prepare(sampleRate, maxBlockSize)`:**
  - Store sampleRate
  - For each of 4 DroneString: prepare waveguide, prepare bowModel
  - Call `initVariations()` to seed per-string ±5% random offsets (deterministic from string index)
  - All drones start inactive

- **`initVariations()`:**
  - `juce::Random rng(42)` (deterministic seed)
  - For each drone: `speedVariation = 0.95f + rng.nextFloat() * 0.10f` (range 0.95-1.05)
  - Same for pressureVariation

- **`setStringCount(count)`:**
  - If count == activeCount, return
  - For new strings (beyond current active): `activateDrone(i)`
  - For removed strings (beyond new count): `deactivateDrone(i)`
  - Set crossfadeSamples = CROSSFADE_LENGTH for click-free transition
  - Update pan positions

- **`activateDrone(index)`:**
  - Compute frequency: `drones[index].targetFrequency = referencePitch * std::pow(2.0f, tuningCents[index] / 1200.0f)`
  - `drones[index].waveguide.trigger(frequency)`
  - Start bow: `drones[index].bow.startBow(0.5f)` (constant moderate velocity — drones sustain)
  - Apply bow speed/pressure with per-string variation
  - `drones[index].active = true`

- **`deactivateDrone(index)`:**
  - `drones[index].bow.stopBow()` (natural release)
  - After energy decays, mark `active = false`
  - Actually: for simplicity, call `waveguide.reset(); bow.reset(); active = false;` during crossfade

- **`processSample(outL, outR)`:**
  ```cpp
  float bridgeSum = 0.0f;
  for (int i = 0; i < activeCount; ++i)
  {
      auto& d = drones[i];
      if (!d.active) continue;

      d.bow.updateEnvelope();
      float v_bow = d.bow.getBowVelocity();
      float F_bow = d.bow.getBowForce();
      float sample = d.waveguide.processSample(v_bow, F_bow, d.friction);
      sample = juce::jlimit(-2.0f, 2.0f, sample);

      bridgeSum += sample;
      outL += sample * d.panL;
      outR += sample * d.panR;
  }
  lastBridgeOutput = bridgeSum;
  ```

- **`updatePanPositions()`:**
  - Read from panTable based on activeCount
  - Convert to equal-power: `panL = std::cos(pan * HALF_PI)`, `panR = std::sin(pan * HALF_PI)`

- **`setTuning(index, cents)`:**
  - Store cents. If drone is active, retrigger waveguide at new frequency (smooth via SmoothedValue on delay length — already handled by WaveguideString's Thiran interpolation)

- **Bow parameter setters (setBowSpeed, etc.):**
  - Store shared value
  - For each active drone, apply with per-string variation:
    ```cpp
    drones[i].bow.setBowSpeed(sharedBowSpeed * drones[i].speedVariation);
    drones[i].bow.setBowPressure(sharedBowPressure * drones[i].pressureVariation);
    ```
  - Position, rosin, brightness, infiniteSustain: apply directly to each drone's waveguide/friction (no variation — these are timbre controls, not bowing dynamics)

---

### Task 3: Create SympatheticStringEngine

**Creates:**
- `Source/DSP/SympatheticStringEngine.h`
- `Source/DSP/SympatheticStringEngine.cpp`

**Dependencies:** None

#### 3A: `Source/DSP/SympatheticStringEngine.h`

```cpp
#pragma once
#include <juce_dsp/juce_dsp.h>
#include <array>
#include <vector>

class SympatheticStringEngine
{
public:
    void prepare (double sampleRate, int maxBlockSize);
    void reset();

    // Configuration
    void setCount (int count);            // 0-12 active strings
    void setAmount (float amount);        // 0.0-1.0 excitation level

    // Call when active notes change — recomputes sympathetic tunings
    void updateTunings (const float* fundamentals, int numFundamentals);

    // Per-sample processing: excitation in, stereo out
    struct StereoSample { float left; float right; };
    StereoSample processSample (float excitation);

private:
    static constexpr int MAX_SYMPATHETICS = 12;

    struct SympatheticString {
        juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Thiran> delay { 4410 };
        float lossCoeff = 0.999f;
        float filterState = 0.0f;
        float frequency = 0.0f;
        float energyEstimate = 0.0f;
        float panL = 0.707f;
        float panR = 0.707f;
        bool active = false;
    };

    std::array<SympatheticString, MAX_SYMPATHETICS> strings;
    int activeCount = 0;
    float amount = 0.0f;
    double currentSampleRate = 44100.0;

    // Crossfade state for retune
    static constexpr int RETUNE_CROSSFADE = 64;
    int retuneCrossfade = 0;

    void tuneString (int index, float frequency);
    void computeHarmonics (const float* fundamentals, int numFundamentals,
                           float* outFreqs, int maxOut, int& numOut);
};
```

#### 3B: `Source/DSP/SympatheticStringEngine.cpp`

**Key implementation details:**

- **`prepare(sampleRate, maxBlockSize)`:**
  - Store sampleRate
  - ProcessSpec with numChannels=1
  - For each of 12 strings: `delay.prepare(spec); delay.setMaximumDelayInSamples(int(sampleRate/20) + 100);`
  - Reset all filter states and energy estimates

- **`setCount(count)`:**
  - Clamp 0-12
  - If reducing: deactivate excess strings (clear delay + reset energy)
  - If increasing: leave strings inactive until `updateTunings` assigns frequencies
  - Store activeCount

- **`setAmount(amount)`:**
  - Store. This scales the excitation level in processSample.

- **`updateTunings(fundamentals, numFundamentals)`:**
  - Call `computeHarmonics` to generate candidate frequencies
  - Assign top `activeCount` harmonics to strings
  - For each string getting a new frequency: `tuneString(i, freq)`
  - Set `retuneCrossfade = RETUNE_CROSSFADE`

- **`computeHarmonics(fundamentals, numFundamentals, outFreqs, maxOut, numOut)`:**
  - For each fundamental, generate harmonics 1x, 2x, 3x... up to Nyquist
  - Collect all candidates, sort by harmonic order (lower = stronger coupling)
  - Deduplicate within 5 cents: `abs(log2(f1/f2) * 1200) < 5`
  - Return up to maxOut unique frequencies

- **`tuneString(index, frequency)`:**
  - `float delaySamples = currentSampleRate / frequency;`
  - `strings[index].delay.setDelay(delaySamples);`
  - Set loss coefficient for natural decay: `lossCoeff = 0.9995f` (longer than bowed strings — sympathetics ring)
  - Compute pan position: `float pan = (activeCount > 1) ? (float)index / (float)(activeCount - 1) : 0.5f;`
  - Equal-power: `panL = cos(pan * halfPi)`, `panR = sin(pan * halfPi)`
  - `active = true`

- **`processSample(excitation)`:**
  ```cpp
  StereoSample out { 0.0f, 0.0f };
  float scaledExcitation = excitation * amount;

  for (int i = 0; i < activeCount; ++i)
  {
      auto& s = strings[i];
      if (!s.active) continue;

      // Gating: skip if no energy and no excitation
      if (s.energyEstimate < 1e-7f && std::abs(scaledExcitation) < 1e-7f)
          continue;

      float delayed = s.delay.popSample(0);
      // One-pole loss filter
      s.filterState = s.lossCoeff * s.filterState + (1.0f - s.lossCoeff) * delayed;
      // Feed excitation + filtered feedback
      s.delay.pushSample(0, s.filterState + scaledExcitation);

      float sample = s.filterState;
      s.energyEstimate = 0.999f * s.energyEstimate + 0.001f * std::abs(sample);

      out.left  += sample * s.panL;
      out.right += sample * s.panR;
  }
  return out;
  ```

---

### Task 4: Add Per-Voice Panning to BowedStringVoice

**Modifies:**
- `Source/BowedStringVoice.h`
- `Source/BowedStringVoice.cpp`

**Dependencies:** None

#### 4A: `Source/BowedStringVoice.h` changes

Add pan members:
```cpp
// Per-voice stereo panning (set from processor based on string count)
float panL = 0.707f;
float panR = 0.707f;
```

Add public setter:
```cpp
void setPan (float left, float right) { panL = left; panR = right; }
```

#### 4B: `Source/BowedStringVoice.cpp` changes

In `renderNextBlock`, replace the all-channel write loop:
```cpp
// Current:
for (int ch = outputBuffer.getNumChannels(); --ch >= 0;)
    outputBuffer.addSample (ch, startSample, sample);

// New:
outputBuffer.addSample (0, startSample, sample * panL);
if (outputBuffer.getNumChannels() >= 2)
    outputBuffer.addSample (1, startSample, sample * panR);
```

**Note:** When STRING_COUNT=1, all voices remain centered (panL=panR=0.707). When STRING_COUNT>1, voices get subtle ±10% random pan variation from center. This is set from the processor (Task 5), not computed by the voice itself.

---

### Task 5: Integrate Everything in PluginProcessor

**Modifies:**
- `Source/PluginProcessor.h`
- `Source/PluginProcessor.cpp`

**Dependencies:** Tasks 1, 2, 3, 4

#### 5A: `Source/PluginProcessor.h` changes

Add includes:
```cpp
#include "DSP/DroneStringEngine.h"
#include "DSP/SympatheticStringEngine.h"
```

Add members:
```cpp
DroneStringEngine droneEngine;
SympatheticStringEngine sympatheticEngine;
```

#### 5B: `Source/PluginProcessor.cpp` changes

**`prepareToPlay`:** Add after existing prepare calls:
```cpp
droneEngine.prepare (sampleRate, samplesPerBlock);
sympatheticEngine.prepare (sampleRate, samplesPerBlock);
```

**`processBlock`:** Complete rewrite of the post-synthesiser section.

New signal flow (per RESEARCH-3.3.md section 6):

```cpp
// 1. Read all params
int stringCount = (int) parameters.getRawParameterValue("stringCount")->load();
float tuning1 = parameters.getRawParameterValue("stringTuning1")->load();
float tuning2 = parameters.getRawParameterValue("stringTuning2")->load();
float tuning3 = parameters.getRawParameterValue("stringTuning3")->load();
float tuning4 = parameters.getRawParameterValue("stringTuning4")->load();
float sympatheticAmount = parameters.getRawParameterValue("sympatheticAmount")->load();
int sympatheticCount = (int) parameters.getRawParameterValue("sympatheticCount")->load();
float material = parameters.getRawParameterValue("bodyMaterial")->load();
float bodySize = parameters.getRawParameterValue("bodySize")->load();
float width = parameters.getRawParameterValue("width")->load();
float bowSpeed = parameters.getRawParameterValue("bowSpeed")->load();
float bowPressure = parameters.getRawParameterValue("bowPressure")->load();
float bowPos = parameters.getRawParameterValue("bowPosition")->load();
float rosin = parameters.getRawParameterValue("rosin")->load();
float brightness = parameters.getRawParameterValue("brightness")->load();
float infSustain = parameters.getRawParameterValue("infiniteSustain")->load();
float refPitch = parameters.getRawParameterValue("referencePitch")->load();

// 2. Dynamic voice cap
int maxPolyphony = 8;
switch (stringCount) {
    case 1: maxPolyphony = 8; break;
    case 2: maxPolyphony = 6; break;
    case 3: maxPolyphony = 5; break;
    case 4: maxPolyphony = 4; break;
}
// Enforce: stop oldest excess voices
int activeVoices = 0;
for (int i = 0; i < synthesiser.getNumVoices(); ++i)
    if (synthesiser.getVoice(i)->isVoiceActive()) activeVoices++;
if (activeVoices > maxPolyphony)
{
    for (int i = 0; i < synthesiser.getNumVoices() && activeVoices > maxPolyphony; ++i)
    {
        if (synthesiser.getVoice(i)->isVoiceActive())
        {
            synthesiser.getVoice(i)->stopNote(0.0f, false);
            --activeVoices;
        }
    }
}

// 3. Set voice panning (subtle variation when multi-string)
for (int i = 0; i < synthesiser.getNumVoices(); ++i)
{
    if (auto* voice = dynamic_cast<BowedStringVoice*>(synthesiser.getVoice(i)))
    {
        if (stringCount <= 1)
            voice->setPan(0.707f, 0.707f);  // center
        else
            voice->setPan(0.707f, 0.707f);  // voices stay center; drones do the panning
    }
}

// 4. Render polyphonic voices
synthesiser.renderNextBlock(buffer, midiMessages, 0, buffer.getNumSamples());

// 5. Update and render drone engine
droneEngine.setStringCount(stringCount);
droneEngine.setTuning(0, tuning1);
droneEngine.setTuning(1, tuning2);
droneEngine.setTuning(2, tuning3);
droneEngine.setTuning(3, tuning4);
droneEngine.setReferencePitch(refPitch);
droneEngine.setBowSpeed(bowSpeed);
droneEngine.setBowPressure(bowPressure);
droneEngine.setBowPosition(bowPos);
droneEngine.setRosin(rosin);
droneEngine.setBrightness(brightness);
droneEngine.setInfiniteSustain(infSustain);

// 6. Update body resonator
bodyResonator.setMaterial(material);
bodyResonator.setSize(bodySize);

// 7. Update sympathetic engine
sympatheticEngine.setCount(sympatheticCount);
sympatheticEngine.setAmount(sympatheticAmount);

// Collect fundamentals from active voices for sympathetic tuning
float fundamentals[12];
int numFundamentals = 0;
for (int i = 0; i < synthesiser.getNumVoices() && numFundamentals < 12; ++i)
{
    if (auto* voice = dynamic_cast<BowedStringVoice*>(synthesiser.getVoice(i)))
    {
        if (voice->isVoiceActive())
            fundamentals[numFundamentals++] = voice->getCurrentFrequency();
    }
}
// Add drone fundamentals
for (int i = 0; i < stringCount && numFundamentals < 12; ++i)
    fundamentals[numFundamentals++] = refPitch * std::pow(2.0f, tuningCents[i] / 1200.0f);
sympatheticEngine.updateTunings(fundamentals, numFundamentals);

// 8. Per-sample processing loop: drones + body stereo + sympathetics
auto* leftData = buffer.getWritePointer(0);
auto* rightData = buffer.getWritePointer(1);

for (int i = 0; i < buffer.getNumSamples(); ++i)
{
    // Add drone output to stereo buffer
    float droneL = 0.0f, droneR = 0.0f;
    droneEngine.processSample(droneL, droneR);
    leftData[i] += droneL;
    rightData[i] += droneR;

    // Capture pre-body bridge sum for sympathetic excitation
    float preBodyMono = (leftData[i] + rightData[i]) * 0.5f;

    // Body resonator (stereo)
    bodyResonator.processStereo(leftData[i], rightData[i]);

    // Post-body mono for sympathetic excitation
    float postBodyMono = (leftData[i] + rightData[i]) * 0.5f;

    // Sympathetic excitation: 50/50 pre/post body
    float excitation = (preBodyMono + postBodyMono) * 0.5f;
    auto symp = sympatheticEngine.processSample(excitation);
    leftData[i] += symp.left;
    rightData[i] += symp.right;
}

// 9. Stereo width
stereoWidthProcessor.processBlock(buffer, width);
```

**Important change to StereoWidthProcessor:** Currently it reads from channel 0 as mono and creates decorrelated stereo. With the new stereo signal flow, channel 1 already has meaningful content from per-string panning. The StereoWidthProcessor needs to be updated to work with true stereo input (M/S encode existing stereo, scale side, decode). This is a small refactor of StereoWidthProcessor.

#### 5C: Expose `getCurrentFrequency()` in BowedStringVoice

Add to `BowedStringVoice.h`:
```cpp
float getCurrentFrequency() const noexcept { return currentFrequency; }
```

This is needed so PluginProcessor can collect fundamentals for sympathetic tuning.

---

### Task 6: Refactor StereoWidthProcessor for True Stereo Input

**Modifies:**
- `Source/DSP/StereoWidthProcessor.h`

**Dependencies:** Task 5 (conceptual — can be coded in parallel)

The current StereoWidthProcessor assumes mono input on channel 0 and creates stereo via allpass decorrelation. With Phase 3.3, the buffer already has meaningful stereo from per-string panning.

**New behavior:**
- Input is already stereo (from voice panning + drone panning + body stereo)
- M/S encode the stereo input: `mid = (L+R)/2`, `side = (L-R)/2`
- Scale side by width parameter
- Decode back: `L = mid + side`, `R = mid - side`
- Remove allpass decorrelator (no longer needed — stereo comes from physical panning)

```cpp
void processBlock (juce::AudioBuffer<float>& buffer, float widthFactor)
{
    if (buffer.getNumChannels() < 2)
        return;

    widthSmoothed.setTargetValue (widthFactor);

    auto* leftData  = buffer.getWritePointer (0);
    auto* rightData = buffer.getWritePointer (1);

    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float w = widthSmoothed.getNextValue();
        float mid  = (leftData[i] + rightData[i]) * 0.5f;
        float side = (leftData[i] - rightData[i]) * 0.5f;
        side *= w;

        leftData[i]  = mid + side;
        rightData[i] = mid - side;
    }
}
```

**Note:** At width=0, output is mono. At width=1, stereo is preserved as-is. At width=2, stereo is exaggerated. Same behavior as before, just operating on real stereo input now.

The allpass decorrelator member can be removed. `prepare()` simplifies to just `widthSmoothed.reset(sampleRate, 0.02)`.

---

### Task 7: Update CMakeLists.txt

**Modifies:**
- `CMakeLists.txt`

**Dependencies:** Tasks 2, 3

Add new source files to `target_sources`:
```cmake
Source/DSP/DroneStringEngine.h
Source/DSP/DroneStringEngine.cpp
Source/DSP/SympatheticStringEngine.h
Source/DSP/SympatheticStringEngine.cpp
```

---

## Implementation Order

```
Task 1: BodyResonator stereo refactor (header + cpp)
Task 2: DroneStringEngine (new, 2 files)         } can be done in parallel
Task 3: SympatheticStringEngine (new, 2 files)    } with Tasks 1, 2
Task 4: BowedStringVoice per-voice pan (minor mod)
  [BUILD CHECK — new files compile, voice still works]

Task 5: PluginProcessor integration (major rewrite of processBlock)
Task 6: StereoWidthProcessor stereo refactor
Task 7: CMakeLists.txt update
  [BUILD + INSTALL + DAW TEST]
```

---

## Success Criteria Mapping

| Criterion | Addressed By |
|-----------|-------------|
| STRING_COUNT=1: single string works correctly | Task 5: drone engine with count=1, centered pan. Existing voice path unchanged. |
| STRING_COUNT=2-4: multiple drones at different pitches | Task 2: DroneStringEngine owns 1-4 WaveguideString instances, tuned via cents offset |
| Per-string tuning offsets shift pitch correctly | Task 2: `referencePitch * pow(2, cents/1200)` |
| Drones panned across stereo field | Task 2: panTable with equal-power panning |
| Sympathetic strings add resonance when SYMPATHETIC_AMOUNT > 0 | Task 3: KS waveguides excited by bridge sum, scaled by amount |
| Sympathetic strings tuned to active note harmonics | Task 3: computeHarmonics generates overtone series, assigns to strings |
| CPU within budget (~19% max config) | Task 2+3: KS waveguides ~0.15% each, drone reuses existing classes. Dynamic voice cap (Task 5). |
| Gating: silent sympathetics don't consume CPU | Task 3: energyEstimate threshold gating (skip pop/push when below 1e-7) |
| Body resonator works in stereo | Task 1: shared coefficients, separate filter instances per channel |
| No clicks on stringCount change | Task 2: 64-sample crossfade on drone activate/deactivate |
| No clicks on sympathetic retune | Task 3: 64-sample crossfade on retune |
| Dynamic voice cap prevents CPU overrun | Task 5: maxPolyphony table, stop oldest excess voices |

---

## Build Verification Points

### After Tasks 1-4 (new classes + voice mod, before integration)
```bash
cd /Users/taylorbrook/Dev/VST-development/build && cmake .. -G Ninja && ninja O-Bowed_VST3 2>&1 | tail -20
```
**Expected:** Compiles. New DSP files are listed but DroneStringEngine/SympatheticStringEngine aren't called yet from processor. BodyResonator has new API but processor hasn't switched to it yet.

### After Tasks 5-7 (full integration)
1. Build:
```bash
cd /Users/taylorbrook/Dev/VST-development/build && ninja O-Bowed_VST3 O-Bowed_AU
```
2. Install:
```bash
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-Bowed-dev.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-Bowed-dev.component
cp -R build/plugins/O-Bowed/O-Bowed_artefacts/Release/VST3/O-Bowed-dev.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-Bowed/O-Bowed_artefacts/Release/AU/O-Bowed-dev.component ~/Library/Audio/Plug-Ins/Components/
```
3. Pluginval:
```bash
/Applications/pluginval.app/Contents/MacOS/pluginval --validate-in-process --strictness-level 5 --validate "build/plugins/O-Bowed/O-Bowed_artefacts/Release/VST3/O-Bowed-dev.vst3" 2>&1 | tail -5
```
4. DAW test: load, play MIDI, sweep STRING_COUNT 1→4, verify drone pitches, sympathetics, stereo imaging

---

## Risk Areas

### 1. Drone Click on STRING_COUNT Change
**Cause:** WaveguideString has energy, suddenly stopped/started.
**Detection:** Change STRING_COUNT while plugin is active — listen for click.
**Fix:** 64-sample crossfade on deactivation (fade old drone to zero) and activation (fade new drone from zero).

### 2. Sympathetic Feedback Buildup
**Cause:** Excitation continuously feeds into KS delay loops, energy accumulates.
**Detection:** Play sustained note with SYMPATHETIC_AMOUNT=1.0 for 30+ seconds — listen for growing noise.
**Fix:** Loss coefficient 0.9995 ensures natural decay. Soft-clip sympathetic output at ±0.5f. Coupling coefficient (amount * 0.01) keeps excitation small.

### 3. Body Resonator Stereo Crosstalk
**Cause:** Accidentally sharing filter state between L/R.
**Detection:** Pan a single drone hard left — right channel should have body-colored silence (no bleed from left filter state).
**Fix:** Separate filter instances per channel (the implementation plan uses bodyModesL/bodyModesR arrays).

### 4. StereoWidthProcessor Regression
**Cause:** Removing allpass decorrelator changes the stereo character vs Phase 3.2.
**Detection:** With STRING_COUNT=1, stereo should still work (voices write to both channels equally, M/S width still applies).
**Fix:** When input is mono-identical (L==R), `side=(L-R)/2=0`, width has no effect → true mono. This is correct behavior for STRING_COUNT=1. The allpass was the old stereo source; now physical panning provides it. Document this intentional behavior change.

### 5. CPU Spike at Max Configuration
**Cause:** 4 drones + 4 voices + 12 sympathetics all active.
**Detection:** Monitor CPU in DAW with max config.
**Fix:** Dynamic voice cap limits polyphonic voices. Sympathetic gating skips silent strings. Budget analysis shows ~19% which is within 25% target.

### 6. Sympathetic Tuning with Multiple Voices
**Cause:** With 3 polyphonic voices + 4 drones = 7 fundamentals, many harmonics overlap.
**Detection:** Play a chord, verify sympathetics don't all cluster on the same frequency.
**Fix:** Deduplication within 5 cents in computeHarmonics prevents wasteful overlap.

---

## Parameters Connected After Phase 3.3

**Newly connected (7):** stringCount, stringTuning1, stringTuning2, stringTuning3, stringTuning4, sympatheticAmount, sympatheticCount

**Total connected:** 17/22

**Remaining (5):** reversedFriction, subHarmonics, referencePitch (partially — used by drones but not by tuning engine), tuningSystem, bowNoise

---

## Reference Files

- **Research:** `plugins/O-Bowed/.planning/stages/2-dsp/RESEARCH-3.3.md` (full drone/sympathetic architecture)
- **Context:** `plugins/O-Bowed/.planning/stages/2-dsp/CONTEXT-3.3.md` (discuss decisions)
- **Architecture:** `plugins/O-Bowed/.planning/research/ARCHITECTURE.md` (immutable spec)
- **Phase 3.1 plan:** `plugins/O-Bowed/.planning/stages/2-dsp/PLAN.md` (WaveguideString API)
- **Phase 3.2 plan:** `plugins/O-Bowed/.planning/stages/2-dsp/PLAN-3.2.md` (BodyResonator/StereoWidthProcessor API)
- **Current source:** `plugins/O-Bowed/Source/` (all implementation files)
