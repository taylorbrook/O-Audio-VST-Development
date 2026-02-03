# Research: Implement Placeholder Parameters

**Date:** 2026-02-02
**Plugin:** O-Detune v1.2.0 -> v1.3.0
**Goal:** Implement all 14 placeholder parameters that are declared but not functional

---

## 1. Current Architecture Summary

### 1.1 Parameters: Implemented vs. Placeholder

**IMPLEMENTED (7 parameters - functional in processBlock):**

| Parameter | Type | Range | How Used |
|-----------|------|-------|----------|
| `blend` | Float | 0-1 | Wobble/Unison crossfade (line 314) |
| `wobble_rate` | Float | 0.1-10 Hz | LFO frequency (line 319-320, 341) |
| `wobble_depth` | Float | 0-100 cents | Pitch deviation (line 320, 393-394) |
| `unison_detune` | Float | 0-50 cents | Voice pitch spread (line 324, 432-435) |
| `focus_low` | Float | 20-500 Hz | Highpass cutoff (line 330, 351) |
| `focus_high` | Float | 1k-20k Hz | Lowpass cutoff (line 332, 352) |
| `mix` | Float | 0-100% | Wet/dry blend (line 336-337, 498-499) |

**PLACEHOLDER (14 parameters - declared but NOT used):**

| Parameter | Type | Range | Purpose |
|-----------|------|-------|---------|
| `wobble_era` | Choice | 60s/70s/80s | Era character preset |
| `wobble_shape` | Choice | Sine/Triangle/Random | LFO waveform |
| `wobble_sync` | Bool | On/Off | Tempo sync |
| `unison_voices` | Choice | 2/3/4/5/7 | Active voice count |
| `unison_dist` | Choice | Linear/Exp/Random | Voice pitch distribution |
| `unison_spread` | Float | 0-100% | Stereo panning width |
| `random_amt` | Float | 0-100% | Per-voice variation |
| `drive` | Float | 0-100% | Tube saturation |
| `color` | Float | -100 to +100 | Tone shaping |
| `age` | Float | 0-100% | Degradation effects |
| `width` | Float | 0-200% | Stereo width |
| `mono_safe` | Bool | On/Off | Mono compatibility |
| `delay` | Float | 0-50ms | Pre-delay |
| `feedback` | Float | 0-80% | Delay recirculation |

### 1.2 Current DSP Architecture

```
Input Buffer
    |
    v
[Focus Filter] -----> focusHighPass + focusLowPass (IIR filters)
    |
    +---> [Wobble Engine] ---> wobbleDelayL/R (Lagrange3rd interpolation)
    |                          wobbleLFO (sine wave only)
    |
    +---> [Unison Engine] ---> unisonDelaysL/R[7] (fixed 3 voices)
    |                          Linear distribution only
    |
    v
[Blend Crossfade] ---> blend parameter (0=wobble, 1=unison)
    |
    v
[DryWetMixer] ---> mixValue
    |
    v
Output Buffer
```

**Key Implementation Details:**

- Sample rate stored in `currentSampleRate` (double)
- ProcessSpec stored in `spec` for DSP component initialization
- Pre-allocated buffers: `wobbleBuffer`, `unisonBuffer`
- Center delay: 50ms fixed (`centerDelayMs`)
- Latency: ~2400 samples @ 48kHz
- Delay lines use `juce::dsp::DelayLine<float, Lagrange3rd>`

### 1.3 Parameter Access Pattern

Parameters are read atomically at the start of processBlock:

```cpp
auto* blendParam = parameters.getRawParameterValue("blend");
float blendValue = blendParam->load();
```

**Current Issues:**
- No parameter smoothing (potential zipper noise)
- Voice count hardcoded to 3 (ignores `unison_voices`)
- Only sine LFO (ignores `wobble_shape`)
- No tempo sync support

---

## 2. Implementation Approach Per Parameter Group

### 2.1 Wobble Engine Enhancements (4 parameters)

#### `wobble_shape` (Choice: Sine/Triangle/Random)

**Implementation Approach:** Replace fixed sine LFO with multi-waveform generator.

**Reference:** O-Tremolo generateWaveform() pattern (PluginProcessor.cpp:378-439)

```cpp
float generateWobbleLFO(float phase, int shapeType) {
    switch (shapeType) {
        case 0: // Sine
            return std::sin(phase * 2.0f * juce::MathConstants<float>::pi);

        case 1: // Triangle
            if (phase < 0.25f) return phase * 4.0f;
            else if (phase < 0.75f) return 2.0f - (phase * 4.0f);
            else return -4.0f + (phase * 4.0f);

        case 2: // Random (sample-and-hold)
            // Sample new random value at phase zero crossings
            return noiseHeldValue;
    }
}
```

**State Required:**
- `float lfoPhase` (0-1, existing)
- `float noiseHeldValue` (for random S&H)
- `int noiseLastQuarter` (for random sampling)
- `juce::Random random` (RNG instance)

**Complexity:** Low
**Domain:** DSP only

#### `wobble_sync` (Bool: On/Off)

**Implementation Approach:** Read host BPM via AudioPlayHead, convert rate Hz to musical divisions.

**Reference:** O-Tremolo (PluginProcessor.cpp:177-242), O-Polystutter (PluginProcessor.cpp:1372-1373)

```cpp
// In processBlock:
float effectiveRate = wobbleRate;

if (wobbleSyncEnabled) {
    if (auto* playHead = getPlayHead()) {
        if (auto posInfo = playHead->getPosition()) {
            if (posInfo->getBpm().hasValue()) {
                double bpm = *posInfo->getBpm();
                double beatsPerSecond = bpm / 60.0;

                // Map wobbleRate (0.1-10Hz) to nearest musical division
                float closestDivision = findClosestMusicalDivision(wobbleRate, beatsPerSecond);
                effectiveRate = static_cast<float>(beatsPerSecond / closestDivision);
            }
        }
    }
    // Fallback: 120 BPM if host unavailable
    if (!positionAvailable) effectiveRate = wobbleRate;
}
```

**Musical Divisions (from O-Tremolo):**

| Division | Beat Multiplier |
|----------|-----------------|
| 1/1 | 4.0 |
| 1/2 | 2.0 |
| 1/4 | 1.0 |
| 1/8 | 0.5 |
| 1/16 | 0.25 |
| 1/8T (triplet) | 0.333 |

**Complexity:** Medium
**Domain:** DSP only

#### `wobble_era` (Choice: 60s/70s/80s)

**Implementation Approach:** Era presets affect LFO behavior and filter characteristics. From market research:
- **60s:** Slower, more pronounced wow, darker filtering
- **70s:** Balanced, classic tape character
- **80s:** Cleaner, less pronounced modulation

```cpp
struct EraPreset {
    float lfoDepthMultiplier;  // Scale the user's depth setting
    float filterDarkening;     // High-frequency rolloff amount
    float driftAmount;         // Random pitch drift (adds to age)
};

const EraPreset eraPresets[3] = {
    { 1.2f, 0.8f, 0.15f },  // 60s: More depth, darker, more drift
    { 1.0f, 1.0f, 0.08f },  // 70s: Neutral
    { 0.8f, 1.1f, 0.03f },  // 80s: Less depth, brighter, less drift
};
```

**Complexity:** Low
**Domain:** DSP only

### 2.2 Unison Engine Enhancements (4 parameters)

#### `unison_voices` (Choice: 2/3/4/5/7)

**Implementation Approach:** Replace hardcoded `activeVoices = 3` with parameter read.

```cpp
// Choice parameter returns index 0-4, map to voice count
const int voiceCounts[] = { 2, 3, 4, 5, 7 };
auto* voicesParam = parameters.getRawParameterValue("unison_voices");
int voiceIndex = static_cast<int>(voicesParam->load());
int activeVoices = voiceCounts[voiceIndex];
```

**Thread Safety:** Voice count changes must be handled carefully:
- Delay lines already allocated for 7 voices (maxUnisonVoices = 7)
- Just change loop bounds, no reallocation needed

**Complexity:** Low
**Domain:** DSP only

#### `unison_dist` (Choice: Linear/Exp/Random)

**Implementation Approach:** Replace fixed linear distribution with selectable algorithm.

**Reference:** O-Bells calculateUnisonDetunes() pattern, Market research section 4.4

```cpp
void calculateVoiceDetunes(int voiceCount, float totalDetune, int distMode, float* detunes) {
    float halfCount = (voiceCount - 1) / 2.0f;

    for (int i = 0; i < voiceCount; ++i) {
        float normalizedPos = (i - halfCount) / halfCount;  // -1 to +1

        switch (distMode) {
            case 0: // Linear
                detunes[i] = normalizedPos * (totalDetune / 2.0f);
                break;

            case 1: // Exponential (more voices near center)
                float sign = (normalizedPos >= 0) ? 1.0f : -1.0f;
                detunes[i] = sign * std::pow(std::abs(normalizedPos), 2.0f) * (totalDetune / 2.0f);
                break;

            case 2: // Random
                detunes[i] = (random.nextFloat() * 2.0f - 1.0f) * (totalDetune / 2.0f);
                break;
        }
    }
}
```

**Complexity:** Low
**Domain:** DSP only

#### `unison_spread` (Float: 0-100%)

**Implementation Approach:** Calculate per-voice pan positions based on spread amount.

```cpp
void calculateVoicePans(int voiceCount, float spread, float* panL, float* panR) {
    float halfCount = (voiceCount - 1) / 2.0f;
    float spreadNorm = spread / 100.0f;  // 0-1

    for (int i = 0; i < voiceCount; ++i) {
        float normalizedPos = (i - halfCount) / halfCount;  // -1 to +1
        float pan = normalizedPos * spreadNorm;  // Scale by spread

        // Constant-power panning
        panL[i] = std::cos((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
        panR[i] = std::sin((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
    }
}
```

**Processing Change:** Apply pan gains per voice in unison loop

**Complexity:** Low
**Domain:** DSP only

#### `random_amt` (Float: 0-100%)

**Implementation Approach:** Add per-voice random variation to detune and timing.

```cpp
// Per voice, add random offset
float randomOffset = (random.nextFloat() * 2.0f - 1.0f) * randomAmt * 5.0f;  // Up to +/-5 cents
float effectiveDetune = voiceDetunes[voice] + randomOffset;

// Optional: Per-voice delay jitter for timing variation
float delayJitter = (random.nextFloat() * 2.0f - 1.0f) * randomAmt * 0.002f * sampleRate;  // Up to +/-2ms
```

**State Required:** Random offsets should be recalculated periodically (not every sample) to avoid noise

**Complexity:** Low
**Domain:** DSP only

### 2.3 Character Section (3 parameters)

#### `drive` (Float: 0-100%)

**Implementation Approach:** Tube-style saturation using tanh waveshaping.

**Reference:** O-AnalogSaturation processTubeSample() (PluginProcessor.cpp:515-563)

```cpp
float processDrive(float input, float driveAmount) {
    if (driveAmount < 0.1f) return input;

    float driveMix = driveAmount / 100.0f;  // 0-1

    // Drive gain: 1.0 to 4.0
    float gain = 1.0f + driveMix * 3.0f;
    float driven = input * gain;

    // Tube-style asymmetric soft clipping
    float saturated;
    if (driven >= 0.0f) {
        saturated = driven / (1.0f + std::abs(driven));  // Soft knee
    } else {
        saturated = std::tanh(driven * 1.5f) / 1.5f;  // Harder negative clip
    }

    // Add subtle even harmonics
    float x2 = saturated * saturated;
    saturated = saturated + 0.1f * x2 * ((saturated > 0) ? 1.0f : -1.0f);

    // Mix
    return input * (1.0f - driveMix) + saturated * driveMix;
}
```

**Processing Position:** After focus filter, before wobble/unison

**Complexity:** Low
**Domain:** DSP only

#### `color` (Float: -100 to +100)

**Implementation Approach:** Analog-modeled tone control using shelving filters.

```cpp
// In prepareToPlay: Initialize color filter
juce::dsp::IIR::Filter<float> colorFilterL, colorFilterR;

// In processBlock:
float colorValue = colorParam->load();  // -100 to +100

if (colorValue < 0) {
    // Negative: Low-pass shelving (dark)
    float cutoff = juce::jmap(colorValue, -100.0f, 0.0f, 800.0f, 20000.0f);
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
        sampleRate, cutoff, 0.707f, juce::Decibels::decibelsToGain(-6.0f * (-colorValue / 100.0f))
    );
    *colorFilterL.coefficients = *coeffs;
} else if (colorValue > 0) {
    // Positive: High-shelf boost (bright)
    auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
        sampleRate, 3000.0f, 0.707f, juce::Decibels::decibelsToGain(6.0f * (colorValue / 100.0f))
    );
    *colorFilterL.coefficients = *coeffs;
}
```

**Complexity:** Medium (filter coefficient updates)
**Domain:** DSP only

#### `age` (Float: 0-100%)

**Implementation Approach:** Combined degradation effects from CONTEXT.md spec.

```cpp
struct AgeProcessor {
    juce::Random noiseGen;
    float filterDriftPhase = 0.0f;

    void process(float& sampleL, float& sampleR, float ageAmount, float sampleRate) {
        float ageMix = ageAmount / 100.0f;

        // 1. Hiss: Low-level broadband noise
        float hissLevel = ageMix * 0.02f;  // Max -34dB noise floor
        sampleL += (noiseGen.nextFloat() * 2.0f - 1.0f) * hissLevel;
        sampleR += (noiseGen.nextFloat() * 2.0f - 1.0f) * hissLevel;

        // 2. Filter drift: Modulate color filter cutoff
        // (Returned as drift amount for color filter to apply)
        filterDriftPhase += 0.3f / sampleRate;  // 0.3 Hz drift
        if (filterDriftPhase >= 1.0f) filterDriftPhase -= 1.0f;

        // 3. Wow/flutter enhancement handled via wobble LFO
    }

    float getFilterDrift(float ageAmount) {
        float ageMix = ageAmount / 100.0f;
        return std::sin(filterDriftPhase * 2.0f * juce::MathConstants<float>::pi)
               * ageMix * 0.2f;  // +/- 20% filter modulation
    }
};
```

**Complexity:** Medium
**Domain:** DSP only

### 2.4 Output Section (4 parameters)

#### `width` (Float: 0-200%)

**Implementation Approach:** Mid/Side stereo width control.

**Reference:** O-MultiBandCompressor M/S processing (PluginProcessor.cpp:478-493)

```cpp
void processWidth(float& left, float& right, float widthPercent) {
    // Encode to M/S
    const float sqrtHalf = 0.70710678f;
    float mid = (left + right) * sqrtHalf;
    float side = (left - right) * sqrtHalf;

    // Scale side by width (0=mono, 1=normal, 2=extra-wide)
    float widthScale = widthPercent / 100.0f;
    side *= widthScale;

    // Decode back to L/R
    left = (mid + side) * sqrtHalf;
    right = (mid - side) * sqrtHalf;
}
```

**Processing Position:** Final output stage, after dry/wet mix

**Complexity:** Low
**Domain:** DSP only

#### `mono_safe` (Bool: On/Off)

**Implementation Approach:** Ensure mono compatibility by limiting side content.

```cpp
void processMonoSafe(float& left, float& right, bool enabled) {
    if (!enabled) return;

    const float sqrtHalf = 0.70710678f;
    float mid = (left + right) * sqrtHalf;
    float side = (left - right) * sqrtHalf;

    // Limit side to prevent phase cancellation on mono sum
    // Soft limit at 0.5 of mid level
    float maxSide = std::abs(mid) * 0.5f;
    side = std::clamp(side, -maxSide, maxSide);

    left = (mid + side) * sqrtHalf;
    right = (mid - side) * sqrtHalf;
}
```

**Complexity:** Low
**Domain:** DSP only

#### `delay` (Float: 0-50ms)

**Implementation Approach:** Pre-delay before processing using existing delay infrastructure.

**Reference:** delay-effects-comprehensive-guide.md

```cpp
// In prepareToPlay: Add pre-delay lines
juce::dsp::DelayLine<float> preDelayL, preDelayR;
preDelayL.setMaximumDelayInSamples(static_cast<int>(0.05 * sampleRate));  // 50ms max

// In processBlock:
float delayMs = delayParam->load();
float delaySamples = (delayMs / 1000.0f) * static_cast<float>(currentSampleRate);

// Use SmoothedValue to avoid clicks
smoothedDelay.setTargetValue(delaySamples);

for (int sample = 0; sample < numSamples; ++sample) {
    float currentDelay = smoothedDelay.getNextValue();
    preDelayL.setDelay(currentDelay);
    // ... process
}
```

**Complexity:** Medium
**Domain:** DSP only

#### `feedback` (Float: 0-80%)

**Implementation Approach:** Recirculate delayed signal back into delay input.

```cpp
// Feedback coefficient (0-0.8)
float feedbackGain = feedbackParam->load() / 100.0f;

// In sample loop:
float delayedL = preDelayL.popSample(0, delaySamples);
float toDelayL = inputL + delayedL * feedbackGain;
preDelayL.pushSample(0, toDelayL);
```

**Risk:** Feedback > 1.0 causes runaway. Hard-limit to 0.8 in parameter range.

**Complexity:** Low
**Domain:** DSP only

---

## 3. JUCE API Recommendations

### 3.1 Parameter Smoothing (Zipper Noise Prevention)

**Use `juce::SmoothedValue` for all continuous parameters:**

```cpp
// In class declaration:
juce::SmoothedValue<float> smoothedBlend;
juce::SmoothedValue<float> smoothedWobbleRate;
juce::SmoothedValue<float> smoothedDrive;
// etc.

// In prepareToPlay:
smoothedBlend.reset(sampleRate, 0.05);  // 50ms smoothing time

// In processBlock:
smoothedBlend.setTargetValue(blendValue);
// Then use: smoothedBlend.getNextValue() per sample
```

**Reference:** delay-effects-comprehensive-guide.md lines 853-859, physical-modelling-synthesis-complete-guide.md lines 1101-1105

### 3.2 AudioPlayHead for Tempo Sync

**JUCE 7+ API (from BREAKING_CHANGES.md):**

```cpp
// Must use getPosition() not getCurrentPosition()
if (auto* playHead = getPlayHead()) {
    auto posInfo = playHead->getPosition();  // Returns Optional<PositionInfo>

    if (posInfo.hasValue()) {
        if (posInfo->getBpm().hasValue()) {
            double bpm = *posInfo->getBpm();
        }
        if (posInfo->getTimeSignature().hasValue()) {
            auto timeSig = *posInfo->getTimeSignature();
        }
    }
}
```

**Safety:** Always check `hasValue()` before dereferencing optionals.

### 3.3 Delay Line Interpolation

**Current:** Using `Lagrange3rd` interpolation (good for pitch modulation).

**For pre-delay (no modulation):** Could use `Linear` for lower CPU, but `Lagrange3rd` is fine.

### 3.4 Oscillator Waveforms

**For LFO with multiple waveforms, avoid reinitialization:**

Instead of using `juce::dsp::Oscillator::initialise()` per waveform, implement waveform generation manually using phase accumulator (as shown in O-Tremolo pattern).

---

## 4. Risk Areas and Mitigations

### 4.1 Real-Time Safety Concerns

| Risk | Mitigation |
|------|------------|
| Parameter smoothing allocation | Pre-allocate SmoothedValue in prepareToPlay |
| Voice count changes mid-block | Delay lines pre-allocated for 7 voices |
| Random number generation | juce::Random is lock-free and thread-safe |
| Filter coefficient updates | Update coefficients once per block, not per sample |

### 4.2 CPU Performance

**Estimated increase vs. v1.2.0:**

| Feature | CPU Impact |
|---------|------------|
| Extra LFO waveforms | Negligible |
| Tempo sync lookups | Negligible (once per block) |
| Saturation (tanh) | Low (~1-2%) |
| Color filter | Low (~1-2%) |
| Age (noise + drift) | Low (~1-2%) |
| Width M/S | Low (~1%) |
| Pre-delay | Medium (~2-3%) |

**Total estimated increase:** ~8-10% (within <10% requirement)

### 4.3 Parameter Dependencies

```
wobble_sync --> wobble_rate (sync modifies effective rate)
wobble_era --> wobble_depth (era scales depth)
age --> color (age modulates color filter cutoff)
width --> mono_safe (mono_safe limits width effect)
delay --> feedback (feedback requires delay)
```

### 4.4 State Management

**New state variables needed:**

```cpp
// LFO state
float lfoPhase = 0.0f;
float noiseHeldValue = 0.0f;
int noiseLastQuarter = -1;
juce::Random random;

// Age state
float filterDriftPhase = 0.0f;

// Voice randomization (refresh periodically, not per-sample)
float voiceRandomOffsets[7] = {0};
int randomRefreshCounter = 0;

// Smoothed parameters
juce::SmoothedValue<float> smoothedBlend;
juce::SmoothedValue<float> smoothedWobbleRate;
// ... etc.
```

---

## 5. Complexity Assessment Summary

| Parameter Group | Complexity | Domain | Estimated Effort |
|-----------------|------------|--------|------------------|
| Wobble Shape | Low | DSP | 0.5 days |
| Wobble Sync | Medium | DSP | 1 day |
| Wobble Era | Low | DSP | 0.5 days |
| Voice Count | Low | DSP | 0.25 days |
| Voice Distribution | Low | DSP | 0.5 days |
| Voice Spread | Low | DSP | 0.5 days |
| Random Amount | Low | DSP | 0.5 days |
| Drive | Low | DSP | 0.5 days |
| Color | Medium | DSP | 1 day |
| Age | Medium | DSP | 1 day |
| Width | Low | DSP | 0.25 days |
| Mono Safe | Low | DSP | 0.25 days |
| Pre-Delay | Medium | DSP | 1 day |
| Feedback | Low | DSP | 0.25 days |

**Total estimated effort:** ~8 developer days

---

## 6. Recommended Implementation Order

**Phase 1: Low-hanging fruit (affects audio immediately)**
1. `unison_voices` - Trivial, just change loop bounds
2. `wobble_shape` - Reuse O-Tremolo pattern
3. `unison_spread` - Simple pan calculation
4. `drive` - Reuse O-AnalogSaturation pattern

**Phase 2: Medium complexity**
5. `unison_dist` - Voice distribution algorithms
6. `color` - Shelving filter implementation
7. `width` - M/S processing
8. `mono_safe` - Side limiting

**Phase 3: Integration features**
9. `wobble_sync` - AudioPlayHead integration
10. `wobble_era` - Preset system
11. `age` - Combined effects
12. `random_amt` - Per-voice variation

**Phase 4: Delay system**
13. `delay` - Pre-delay line
14. `feedback` - Recirculation

---

## 7. Testing Checklist

- [ ] All 14 parameters audibly affect output
- [ ] No zipper noise during automation
- [ ] No clicks/pops on parameter changes
- [ ] Tempo sync works with Logic Pro, Ableton
- [ ] Tempo sync gracefully handles offline bounce (no host)
- [ ] CPU usage < 10% increase
- [ ] pluginval passes
- [ ] Presets save/restore correctly
- [ ] AU and VST3 build successfully
