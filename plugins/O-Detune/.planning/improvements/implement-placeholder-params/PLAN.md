---
milestone: implement-placeholder-params
domain: dsp
execute_agent: dsp-agent
version_bump: minor
base_version: 1.2.0
target_version: 1.3.0
created: 2026-02-02
---

# Implementation Plan: O-Detune Placeholder Parameters

**Plugin:** O-Detune v1.2.0 → v1.3.0
**Goal:** Implement all 14 placeholder APVTS parameters that are declared but not functional

## Executive Summary

O-Detune has 21 declared parameters but only 7 affect audio output. This plan implements the remaining 14 parameters across 4 implementation waves, organized by dependency and complexity.

## Wave Overview

| Wave | Tasks | Focus | Dependencies |
|------|-------|-------|--------------|
| 1 | 1-4 | Foundation: State variables, parameter smoothing | None |
| 2 | 5-10 | Wobble & Unison engines | Wave 1 |
| 3 | 11-14 | Character section (drive, color, age) | Wave 1 |
| 4 | 15-19 | Output section (width, delay, feedback) | Waves 2-3 |

---

## Wave 1: Foundation (Tasks 1-4)

### Task 1: Add State Variables to Header

**File:** `PluginProcessor.h`

**Changes:**
```cpp
// Add after existing state variables (line ~84):

// LFO state (for multi-waveform support)
float lfoPhase = 0.0f;
float noiseHeldValue = 0.0f;
int noiseLastQuarter = -1;
juce::Random random;

// Age processor state
float filterDriftPhase = 0.0f;

// Voice randomization (refresh periodically)
float voiceRandomOffsets[maxUnisonVoices] = {0};
int randomRefreshCounter = 0;

// Pre-delay lines
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> preDelayL;
juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> preDelayR;
float feedbackStateL = 0.0f;
float feedbackStateR = 0.0f;

// Color filter
juce::dsp::IIR::Filter<float> colorFilterL;
juce::dsp::IIR::Filter<float> colorFilterR;
```

**Verification:** Compiles without errors

**Blocked by:** None

---

### Task 2: Add SmoothedValue Declarations

**File:** `PluginProcessor.h`

**Changes:**
```cpp
// Add after state variables:

// Parameter smoothing (50ms ramp time)
juce::SmoothedValue<float> smoothedBlend;
juce::SmoothedValue<float> smoothedWobbleRate;
juce::SmoothedValue<float> smoothedWobbleDepth;
juce::SmoothedValue<float> smoothedUnisonDetune;
juce::SmoothedValue<float> smoothedDrive;
juce::SmoothedValue<float> smoothedColor;
juce::SmoothedValue<float> smoothedAge;
juce::SmoothedValue<float> smoothedWidth;
juce::SmoothedValue<float> smoothedDelay;
juce::SmoothedValue<float> smoothedFeedback;
juce::SmoothedValue<float> smoothedUnisonSpread;
juce::SmoothedValue<float> smoothedRandomAmt;
```

**Verification:** Compiles without errors

**Blocked by:** None

---

### Task 3: Initialize New DSP Components in prepareToPlay

**File:** `PluginProcessor.cpp`

**Changes to prepareToPlay():**
```cpp
// Add after dry/wet mixer preparation (line ~279):

// Initialize pre-delay lines (50ms max)
const int maxPreDelaySamples = static_cast<int>(0.05 * sampleRate);
preDelayL.prepare(spec);
preDelayR.prepare(spec);
preDelayL.setMaximumDelayInSamples(maxPreDelaySamples);
preDelayR.setMaximumDelayInSamples(maxPreDelaySamples);
preDelayL.reset();
preDelayR.reset();

// Initialize color filters
colorFilterL.prepare(spec);
colorFilterR.prepare(spec);
colorFilterL.reset();
colorFilterR.reset();

// Initialize parameter smoothing (50ms ramp)
const double smoothingTime = 0.05;
smoothedBlend.reset(sampleRate, smoothingTime);
smoothedWobbleRate.reset(sampleRate, smoothingTime);
smoothedWobbleDepth.reset(sampleRate, smoothingTime);
smoothedUnisonDetune.reset(sampleRate, smoothingTime);
smoothedDrive.reset(sampleRate, smoothingTime);
smoothedColor.reset(sampleRate, smoothingTime);
smoothedAge.reset(sampleRate, smoothingTime);
smoothedWidth.reset(sampleRate, smoothingTime);
smoothedDelay.reset(sampleRate, smoothingTime);
smoothedFeedback.reset(sampleRate, smoothingTime);
smoothedUnisonSpread.reset(sampleRate, smoothingTime);
smoothedRandomAmt.reset(sampleRate, smoothingTime);

// Reset LFO state
lfoPhase = 0.0f;
noiseHeldValue = 0.0f;
noiseLastQuarter = -1;
filterDriftPhase = 0.0f;
randomRefreshCounter = 0;
feedbackStateL = 0.0f;
feedbackStateR = 0.0f;
```

**Verification:** Plugin loads without crash, prepareToPlay completes

**Blocked by:** Tasks 1, 2

---

### Task 4: Read All Parameters at Block Start

**File:** `PluginProcessor.cpp`

**Changes to processBlock() parameter reading section (line ~310):**
```cpp
// Add after existing parameter reads:

// Wobble engine - additional parameters
auto* wobbleEraParam = parameters.getRawParameterValue("wobble_era");
auto* wobbleShapeParam = parameters.getRawParameterValue("wobble_shape");
auto* wobbleSyncParam = parameters.getRawParameterValue("wobble_sync");
int wobbleEra = static_cast<int>(wobbleEraParam->load());
int wobbleShape = static_cast<int>(wobbleShapeParam->load());
bool wobbleSync = wobbleSyncParam->load() > 0.5f;

// Unison engine - additional parameters
auto* unisonVoicesParam = parameters.getRawParameterValue("unison_voices");
auto* unisonDistParam = parameters.getRawParameterValue("unison_dist");
auto* unisonSpreadParam = parameters.getRawParameterValue("unison_spread");
auto* randomAmtParam = parameters.getRawParameterValue("random_amt");
int unisonVoicesIndex = static_cast<int>(unisonVoicesParam->load());
int unisonDist = static_cast<int>(unisonDistParam->load());
float unisonSpread = unisonSpreadParam->load();
float randomAmt = randomAmtParam->load();

// Map voice index to count: 0→2, 1→3, 2→4, 3→5, 4→7
const int voiceCounts[] = { 2, 3, 4, 5, 7 };
int activeVoices = voiceCounts[unisonVoicesIndex];  // Replace hardcoded 3

// Character section
auto* driveParam = parameters.getRawParameterValue("drive");
auto* colorParam = parameters.getRawParameterValue("color");
auto* ageParam = parameters.getRawParameterValue("age");
float driveValue = driveParam->load();
float colorValue = colorParam->load();
float ageValue = ageParam->load();

// Output section
auto* widthParam = parameters.getRawParameterValue("width");
auto* monoSafeParam = parameters.getRawParameterValue("mono_safe");
auto* delayParam = parameters.getRawParameterValue("delay");
auto* feedbackParam = parameters.getRawParameterValue("feedback");
float widthValue = widthParam->load();
bool monoSafe = monoSafeParam->load() > 0.5f;
float delayMs = delayParam->load();
float feedbackValue = feedbackParam->load();

// Update smoothed values
smoothedBlend.setTargetValue(blendValue);
smoothedWobbleRate.setTargetValue(wobbleRate);
smoothedWobbleDepth.setTargetValue(wobbleDepth);
smoothedUnisonDetune.setTargetValue(unisonDetune);
smoothedDrive.setTargetValue(driveValue);
smoothedColor.setTargetValue(colorValue);
smoothedAge.setTargetValue(ageValue);
smoothedWidth.setTargetValue(widthValue);
smoothedDelay.setTargetValue(delayMs);
smoothedFeedback.setTargetValue(feedbackValue);
smoothedUnisonSpread.setTargetValue(unisonSpread);
smoothedRandomAmt.setTargetValue(randomAmt);
```

**Verification:** All parameters read without crash, values logged correctly

**Blocked by:** Tasks 1, 2, 3

---

## Wave 2: Wobble & Unison Engines (Tasks 5-10)

### Task 5: Implement Multi-Waveform LFO (wobble_shape)

**File:** `PluginProcessor.cpp`

**Add helper function before processBlock():**
```cpp
// Multi-waveform LFO generator
float ODetuneAudioProcessor::generateLFO(float phase, int shapeType, float& noiseHeld, int& lastQuarter, juce::Random& rng)
{
    switch (shapeType)
    {
        case 0: // Sine
            return std::sin(phase * juce::MathConstants<float>::twoPi);

        case 1: // Triangle
        {
            float t = phase;
            if (t < 0.25f)
                return t * 4.0f;
            else if (t < 0.75f)
                return 2.0f - (t * 4.0f);
            else
                return -4.0f + (t * 4.0f);
        }

        case 2: // Random (sample-and-hold)
        {
            int quarter = static_cast<int>(phase * 4.0f);
            if (quarter != lastQuarter)
            {
                noiseHeld = rng.nextFloat() * 2.0f - 1.0f;
                lastQuarter = quarter;
            }
            return noiseHeld;
        }

        default:
            return std::sin(phase * juce::MathConstants<float>::twoPi);
    }
}
```

**Add declaration to header:**
```cpp
float generateLFO(float phase, int shapeType, float& noiseHeld, int& lastQuarter, juce::Random& rng);
```

**Modify wobble processing to use manual phase accumulator instead of juce::dsp::Oscillator:**
```cpp
// Replace wobbleLFO.processSample() call with:
float lfoValue = generateLFO(lfoPhase, wobbleShape, noiseHeldValue, noiseLastQuarter, random);

// Advance phase
lfoPhase += effectiveRate / static_cast<float>(currentSampleRate);
if (lfoPhase >= 1.0f) lfoPhase -= 1.0f;
```

**Verification:** Triangle and Random shapes produce audibly different modulation

**Blocked by:** Task 4

---

### Task 6: Implement Tempo Sync (wobble_sync)

**File:** `PluginProcessor.cpp`

**Add before sample loop in processBlock():**
```cpp
// Calculate effective wobble rate (with tempo sync if enabled)
float effectiveRate = wobbleRate;

if (wobbleSync)
{
    double hostBpm = 120.0;  // Fallback

    if (auto* playHead = getPlayHead())
    {
        auto posInfo = playHead->getPosition();
        if (posInfo.hasValue() && posInfo->getBpm().hasValue())
        {
            hostBpm = *posInfo->getBpm();
        }
    }

    double beatsPerSecond = hostBpm / 60.0;

    // Map wobbleRate (0.1-10 Hz) to nearest musical division
    // Divisions: 1/16=0.25, 1/8=0.5, 1/4=1, 1/2=2, 1=4 beats
    const float divisions[] = { 0.25f, 0.333f, 0.5f, 1.0f, 2.0f, 4.0f };
    const int numDivisions = 6;

    // Find closest musical rate
    float targetHz = wobbleRate;
    float closestHz = static_cast<float>(beatsPerSecond / divisions[0]);
    float closestDiff = std::abs(targetHz - closestHz);

    for (int i = 1; i < numDivisions; ++i)
    {
        float hz = static_cast<float>(beatsPerSecond / divisions[i]);
        float diff = std::abs(targetHz - hz);
        if (diff < closestDiff)
        {
            closestHz = hz;
            closestDiff = diff;
        }
    }

    effectiveRate = closestHz;
}
```

**Verification:** Wobble locks to host tempo in Logic Pro/Ableton when sync enabled

**Blocked by:** Task 5

---

### Task 7: Implement Era Presets (wobble_era)

**File:** `PluginProcessor.cpp`

**Add era preset data:**
```cpp
// Era presets: { depthMultiplier, filterDarkening, driftAmount }
struct EraPreset { float depthMult; float darkness; float drift; };
const EraPreset eraPresets[] = {
    { 1.2f, 0.8f, 0.15f },  // 60s: More depth, darker, more drift
    { 1.0f, 1.0f, 0.08f },  // 70s: Neutral
    { 0.8f, 1.1f, 0.03f },  // 80s: Less depth, brighter, less drift
};
```

**Apply era scaling to wobble depth:**
```cpp
// After reading wobbleDepth:
float scaledWobbleDepth = wobbleDepth * eraPresets[wobbleEra].depthMult;
```

**Verification:** 60s sounds warmer/wobblier, 80s sounds cleaner

**Blocked by:** Task 4

---

### Task 8: Implement Voice Count (unison_voices)

**File:** `PluginProcessor.cpp`

**Already handled in Task 4** - replace hardcoded `activeVoices = 3` with parameter read.

**Verification:** Voice count changes audibly (more chorus with 7 voices)

**Blocked by:** Task 4

---

### Task 9: Implement Voice Distribution (unison_dist)

**File:** `PluginProcessor.cpp`

**Replace fixed linear distribution with selectable algorithm:**
```cpp
// Before unison processing loop:
float voiceDetunes[maxUnisonVoices];
float halfCount = (activeVoices - 1) / 2.0f;

for (int i = 0; i < activeVoices; ++i)
{
    float normalizedPos = (halfCount > 0.0f) ? (i - halfCount) / halfCount : 0.0f;

    switch (unisonDist)
    {
        case 0: // Linear
            voiceDetunes[i] = normalizedPos * (unisonDetune / 2.0f);
            break;

        case 1: // Exponential (more voices near center)
        {
            float sign = (normalizedPos >= 0) ? 1.0f : -1.0f;
            voiceDetunes[i] = sign * std::pow(std::abs(normalizedPos), 2.0f) * (unisonDetune / 2.0f);
            break;
        }

        case 2: // Random
            voiceDetunes[i] = (random.nextFloat() * 2.0f - 1.0f) * (unisonDetune / 2.0f);
            break;
    }
}
```

**Verification:** Exponential sounds tighter in center, Random sounds more chaotic

**Blocked by:** Task 8

---

### Task 10: Implement Spread and Random Amount (unison_spread, random_amt)

**File:** `PluginProcessor.cpp`

**Calculate per-voice pan positions:**
```cpp
// After voice detune calculation:
float voicePanL[maxUnisonVoices];
float voicePanR[maxUnisonVoices];

float spreadNorm = unisonSpread / 100.0f;

for (int i = 0; i < activeVoices; ++i)
{
    float normalizedPos = (halfCount > 0.0f) ? (i - halfCount) / halfCount : 0.0f;
    float pan = normalizedPos * spreadNorm;

    // Constant-power panning
    voicePanL[i] = std::cos((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
    voicePanR[i] = std::sin((pan + 1.0f) * 0.25f * juce::MathConstants<float>::pi);
}

// Add random variation (refresh every 1024 samples to avoid noise)
if (randomRefreshCounter <= 0)
{
    for (int i = 0; i < maxUnisonVoices; ++i)
    {
        voiceRandomOffsets[i] = (random.nextFloat() * 2.0f - 1.0f) * (randomAmt / 100.0f) * 5.0f;
    }
    randomRefreshCounter = 1024;
}
randomRefreshCounter -= numSamples;
```

**Modify unison loop to apply pan gains and random offsets:**
```cpp
// In voice loop:
float effectiveDetune = voiceDetunes[voice] + voiceRandomOffsets[voice];
// ... delay calculation ...
outputDataL[sample] += delayedSample * voicePanL[voice] / static_cast<float>(activeVoices);
outputDataR[sample] += delayedSample * voicePanR[voice] / static_cast<float>(activeVoices);
```

**Verification:** Spread creates stereo width, random adds organic variation

**Blocked by:** Task 9

---

## Wave 3: Character Section (Tasks 11-14)

### Task 11: Implement Drive (Tube Saturation)

**File:** `PluginProcessor.cpp`

**Add helper function:**
```cpp
float ODetuneAudioProcessor::processDrive(float input, float driveAmount)
{
    if (driveAmount < 1.0f) return input;

    float driveMix = driveAmount / 100.0f;
    float gain = 1.0f + driveMix * 3.0f;  // 1.0 to 4.0
    float driven = input * gain;

    // Tube-style asymmetric soft clipping
    float saturated;
    if (driven >= 0.0f)
        saturated = driven / (1.0f + std::abs(driven));
    else
        saturated = std::tanh(driven * 1.5f) / 1.5f;

    // Add subtle even harmonics
    float x2 = saturated * saturated;
    saturated = saturated + 0.1f * x2 * ((saturated > 0) ? 1.0f : -1.0f);

    return input * (1.0f - driveMix) + saturated * driveMix;
}
```

**Add declaration to header.**

**Insert drive processing after focus filter, before wobble/unison:**
```cpp
// After focus filter processing:
float currentDrive = smoothedDrive.getCurrentValue();
if (currentDrive > 1.0f)
{
    for (int channel = 0; channel < numChannels; ++channel)
    {
        auto* data = buffer.getWritePointer(channel);
        for (int sample = 0; sample < numSamples; ++sample)
        {
            float drive = smoothedDrive.getNextValue();
            data[sample] = processDrive(data[sample], drive);
        }
    }
}
```

**Verification:** Drive adds warmth at low settings, grit at high settings

**Blocked by:** Task 3

---

### Task 12: Implement Color (Tone Shaping)

**File:** `PluginProcessor.cpp`

**Update color filter coefficients and process:**
```cpp
// After drive processing:
float currentColor = smoothedColor.getCurrentValue();
if (std::abs(currentColor) > 1.0f)
{
    // Update filter coefficients (once per block for efficiency)
    if (currentColor < 0)
    {
        // Negative: Low-shelf cut (dark)
        float cutoff = juce::jmap(currentColor, -100.0f, 0.0f, 800.0f, 20000.0f);
        float gainDb = -6.0f * (-currentColor / 100.0f);
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeLowShelf(
            currentSampleRate, cutoff, 0.707f, juce::Decibels::decibelsToGain(gainDb));
        *colorFilterL.coefficients = *coeffs;
        *colorFilterR.coefficients = *coeffs;
    }
    else
    {
        // Positive: High-shelf boost (bright)
        float gainDb = 6.0f * (currentColor / 100.0f);
        auto coeffs = juce::dsp::IIR::Coefficients<float>::makeHighShelf(
            currentSampleRate, 3000.0f, 0.707f, juce::Decibels::decibelsToGain(gainDb));
        *colorFilterL.coefficients = *coeffs;
        *colorFilterR.coefficients = *coeffs;
    }

    // Process
    for (int sample = 0; sample < numSamples; ++sample)
    {
        if (numChannels >= 1)
            buffer.getWritePointer(0)[sample] = colorFilterL.processSample(buffer.getWritePointer(0)[sample]);
        if (numChannels >= 2)
            buffer.getWritePointer(1)[sample] = colorFilterR.processSample(buffer.getWritePointer(1)[sample]);
    }
}
```

**Verification:** Negative color = darker/woolly, positive = brighter/present

**Blocked by:** Task 3

---

### Task 13: Implement Age (Degradation)

**File:** `PluginProcessor.cpp`

**Add age processing after color filter:**
```cpp
// Age processing
float currentAge = smoothedAge.getCurrentValue();
if (currentAge > 1.0f)
{
    float ageMix = currentAge / 100.0f;

    // 1. Hiss: Low-level broadband noise
    float hissLevel = ageMix * 0.02f;  // Max -34dB noise floor

    for (int sample = 0; sample < numSamples; ++sample)
    {
        float noiseL = (random.nextFloat() * 2.0f - 1.0f) * hissLevel;
        float noiseR = (random.nextFloat() * 2.0f - 1.0f) * hissLevel;

        if (numChannels >= 1)
            buffer.getWritePointer(0)[sample] += noiseL;
        if (numChannels >= 2)
            buffer.getWritePointer(1)[sample] += noiseR;
    }

    // 2. Filter drift: Modulate color filter cutoff (affects next block)
    filterDriftPhase += 0.3f * numSamples / static_cast<float>(currentSampleRate);
    if (filterDriftPhase >= 1.0f) filterDriftPhase -= 1.0f;

    // Drift modulation applied to color filter in Task 12 (add interaction)
}
```

**Modify Task 12 to incorporate age drift:**
```cpp
// Add drift modulation to color filter cutoff:
float driftMod = std::sin(filterDriftPhase * juce::MathConstants<float>::twoPi) * (currentAge / 100.0f) * 0.2f;
// Apply driftMod to cutoff frequency calculation
```

**Verification:** Age adds hiss and subtle filter movement

**Blocked by:** Tasks 11, 12

---

### Task 14: Integrate Era with Age

**File:** `PluginProcessor.cpp`

**Apply era drift multiplier to age effects:**
```cpp
float eraDrift = eraPresets[wobbleEra].drift;
float effectiveAgeDrift = ageMix * eraDrift;
// Use effectiveAgeDrift instead of ageMix for drift calculations
```

**Verification:** 60s era has more age drift, 80s has less

**Blocked by:** Tasks 7, 13

---

## Wave 4: Output Section (Tasks 15-19)

### Task 15: Implement Width (Stereo Width)

**File:** `PluginProcessor.cpp`

**Add helper function:**
```cpp
void ODetuneAudioProcessor::processWidth(float& left, float& right, float widthPercent)
{
    const float sqrtHalf = 0.70710678f;

    // Encode to M/S
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

**Add declaration to header.**

**Apply after dry/wet mix:**
```cpp
// After dryWetMixer.mixWetSamples(buffer):
float currentWidth = smoothedWidth.getCurrentValue();
if (std::abs(currentWidth - 100.0f) > 1.0f)  // Not default
{
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float width = smoothedWidth.getNextValue();
        if (numChannels >= 2)
        {
            float left = buffer.getWritePointer(0)[sample];
            float right = buffer.getWritePointer(1)[sample];
            processWidth(left, right, width);
            buffer.getWritePointer(0)[sample] = left;
            buffer.getWritePointer(1)[sample] = right;
        }
    }
}
```

**Verification:** Width 0% = mono, 100% = normal, 200% = extra-wide

**Blocked by:** Task 3

---

### Task 16: Implement Mono Safe

**File:** `PluginProcessor.cpp`

**Add helper function:**
```cpp
void ODetuneAudioProcessor::processMonoSafe(float& left, float& right)
{
    const float sqrtHalf = 0.70710678f;

    float mid = (left + right) * sqrtHalf;
    float side = (left - right) * sqrtHalf;

    // Limit side to prevent phase cancellation on mono sum
    float maxSide = std::abs(mid) * 0.5f + 0.001f;  // Small epsilon to avoid div/0
    side = std::clamp(side, -maxSide, maxSide);

    left = (mid + side) * sqrtHalf;
    right = (mid - side) * sqrtHalf;
}
```

**Apply after width processing:**
```cpp
if (monoSafe && numChannels >= 2)
{
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float left = buffer.getWritePointer(0)[sample];
        float right = buffer.getWritePointer(1)[sample];
        processMonoSafe(left, right);
        buffer.getWritePointer(0)[sample] = left;
        buffer.getWritePointer(1)[sample] = right;
    }
}
```

**Verification:** Mono sum has no phase cancellation artifacts

**Blocked by:** Task 15

---

### Task 17: Implement Pre-Delay

**File:** `PluginProcessor.cpp`

**Add pre-delay processing at start of wet path (after focus filter, before drive):**
```cpp
// Pre-delay processing
float currentDelay = smoothedDelay.getCurrentValue();
float currentFeedback = smoothedFeedback.getCurrentValue() / 100.0f;

if (currentDelay > 0.1f || currentFeedback > 0.01f)
{
    for (int sample = 0; sample < numSamples; ++sample)
    {
        float delayMs = smoothedDelay.getNextValue();
        float feedback = smoothedFeedback.getNextValue() / 100.0f;
        float delaySamples = (delayMs / 1000.0f) * static_cast<float>(currentSampleRate);

        if (numChannels >= 1)
        {
            auto* dataL = buffer.getWritePointer(0);
            preDelayL.setDelay(delaySamples);
            float delayedL = preDelayL.popSample(0);
            float toDelayL = dataL[sample] + delayedL * feedback;
            preDelayL.pushSample(0, toDelayL);
            dataL[sample] = delayedL;
        }

        if (numChannels >= 2)
        {
            auto* dataR = buffer.getWritePointer(1);
            preDelayR.setDelay(delaySamples);
            float delayedR = preDelayR.popSample(0);
            float toDelayR = dataR[sample] + delayedR * feedback;
            preDelayR.pushSample(0, toDelayR);
            dataR[sample] = delayedR;
        }
    }
}
```

**Verification:** Pre-delay creates spatial depth, non-zero adds latency to wet path

**Blocked by:** Task 3

---

### Task 18: Implement Feedback (Already in Task 17)

Feedback is implemented as part of Task 17's pre-delay loop.

**Verification:** Feedback creates resonant echoes, max 80% is safe (no runaway)

**Blocked by:** Task 17

---

### Task 19: Final Integration and Processing Order

**File:** `PluginProcessor.cpp`

**Ensure correct processing order:**
```
1. Capture dry signal (DryWetMixer)
2. Focus Filter (existing)
3. Pre-Delay + Feedback (new - Task 17)
4. Drive (new - Task 11)
5. Color + Age drift (new - Tasks 12, 13)
6. Age hiss (new - Task 13)
7. Wobble Engine with multi-waveform LFO, sync, era (modified)
8. Unison Engine with voices, dist, spread, random (modified)
9. Blend crossfade (existing)
10. Dry/Wet mix (existing)
11. Width (new - Task 15)
12. Mono Safe (new - Task 16)
```

**Refactor processBlock() to follow this order.**

**Verification:** All 14 parameters audibly affect output, no artifacts

**Blocked by:** All previous tasks

---

## Acceptance Criteria Checklist

- [ ] All 14 placeholder parameters audibly affect output
- [ ] No zipper noise during parameter automation
- [ ] No clicks/pops on parameter changes
- [ ] Tempo sync works in Logic Pro and Ableton Live
- [ ] CPU usage increase < 10% vs v1.2.0
- [ ] pluginval passes all tests
- [ ] Preset save/restore works for all parameters
- [ ] VST3 and AU build successfully
- [ ] Backup created before implementation

---

## Risk Mitigation

| Risk | Mitigation |
|------|------------|
| Zipper noise | SmoothedValue for all continuous parameters |
| CPU spike | Process character/age effects conditionally (skip if at 0) |
| Tempo sync crash | Null-check playHead and position info |
| Feedback runaway | Hard-limit feedback to 80% in parameter range |
| Filter coefficient update cost | Update once per block, not per sample |

---

## Notes

- Domain is **DSP** - all changes are in PluginProcessor.cpp/h
- Execute agent: **dsp-agent**
- Version bump: **MINOR** (1.2.0 → 1.3.0) - new features, no breaking changes
- Estimated total: 19 tasks across 4 waves
