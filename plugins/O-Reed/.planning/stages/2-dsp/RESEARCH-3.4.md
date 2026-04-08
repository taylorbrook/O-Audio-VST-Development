# Phase 3.4 Research: Impossible Physics + Dual Bore

**Date:** 2026-04-05
**Stage:** 2 (DSP)
**Phase:** 3.4
**Status:** COMPLETE

---

## 1. Infinite Sustain

### Current State
- `BoreWaveguide::updateParams()` line 136: viscFilter gain `g = 0.995f` (fixed)
- Bell reflection filter: allpass with cutoff-dependent coefficient
- Combined bore loop gain is always < 1.0, so energy decays naturally

### Implementation Approach

**Viscothermal loss reduction:**
```cpp
float g_base = 0.995f;
float g_eff = g_base + infiniteSustain * (1.0f - g_base);  // 0.995 -> 1.0
```

**Bell loss reduction:**
The bell filter produces reflected wave `p_reflected = -bellFiltered`. To reduce bell loss, interpolate the bell cutoff toward infinity (full reflection):
```cpp
// bellCutoff normally 800-6000 Hz. Push toward sr/2 for full reflection.
float sustainedBellCutoff = bellCutoff + infiniteSustain * (sr * 0.499f - bellCutoff);
```
At `infiniteSustain=1.0`: bell coefficient `a -> 0` (total reflection), visc gain `g -> 1.0` = lossless bore loop.

**Stability concern:** Lossless bore + active reed excitation = energy accumulation. The existing tanh soft-clip at output (ReedWindVoice.cpp:409) is the safety net. No additional limiting needed since reed model has natural energy-limiting behavior (reed closure clamps flow).

### Regression Safety
At `infiniteSustain=0.0` (default): `g_eff = 0.995f`, bell cutoff unchanged. Identical to Phase 3.3.

---

## 2. Reverse Bore

### Current State
- `BoreWaveguide::updateParams()` lines 115-120: scale factors computed from `segCenters[i]`
- `segCenters = {0.05, 0.20, 0.40, 0.625, 0.875}` (reed to bell ordering)
- Conical scaling: `r_at_seg = r_in + segCenters[i] * L * tan(halfAngle)`

### Implementation Approach

Interpolate segment centers between normal and reversed:
```cpp
constexpr float normalCenters[5]   = { 0.05f, 0.20f, 0.40f, 0.625f, 0.875f };
constexpr float reversedCenters[5] = { 0.95f, 0.80f, 0.60f, 0.375f, 0.125f };

for (int i = 0; i < 5; ++i)
{
    float center = normalCenters[i] + reverseBore * (reversedCenters[i] - normalCenters[i]);
    float r_at_seg = r_in + center * L * std::tan(halfAngle);
    targetScaleForward[i]  = r_in / r_at_seg;
    targetScaleBackward[i] = r_at_seg / r_in;
}
```

At `reverseBore=0`: normal bore. At `reverseBore=1`: bore narrows toward bell (hichiriki-like).

**Only audible when `boreCharacter > 0`**: Cylindrical bore has halfAngle=0, all scale factors = 1.0 regardless of reverse.

### New Parameter
- `updateParams()` signature: add `float reverseBore` parameter
- Smooth transition via existing `smoothCoeff` (50ms target/current interpolation)

---

## 3. Dual Bore

### Current State
- Single `BoreWaveguide bore` in ReedWindVoice.h (line 46)
- `pDualBore` parameter pointer cached but never read in rendering loop
- `pDronePitch` and `pFeedbackPath` also cached but unused

### Implementation Approach

**New member in ReedWindVoice.h:**
```cpp
BoreWaveguide bore2;  // Second bore for drone mode
float prevBore2Minus = 0.0f;
```

**Prepare:**
```cpp
bore2.prepare(sampleRate, maxBlockSize);
```

**In renderNextBlock, after reading dualBore parameter:**
```cpp
bool dualBoreActive = (pDualBore != nullptr) && (pDualBore->load() > 0.5f);
```

**Bore2 frequency:**
```cpp
float dronePitchCents = (pDronePitch != nullptr) ? pDronePitch->load() : 0.0f;
float bore2Hz = frequency * std::pow(2.0f, dronePitchCents / 1200.0f);
bore2.setFrequency(bore2Hz);
```

**Reed coupling (combined impedance):**
Both bores receive the same `p_bore_plus` from the reed/mouthpiece.
The backward wave seen by the reed is a weighted mix:
```cpp
float feedbackPath = (pFeedbackPath != nullptr) ? pFeedbackPath->load() : 0.0f;

// Process both bores
float bore1_bwd = bore.processSample(p_bore_plus);
float bore2_bwd = bore2.processSample(p_bore_plus);

// Cross-coupled backward waves
float bore1_effective = bore1_bwd * (1.0f - feedbackPath) + bore2_bwd * feedbackPath;
float bore2_effective = bore2_bwd * (1.0f - feedbackPath) + bore1_bwd * feedbackPath;

prevBoreMinus = bore1_effective;
prevBore2Minus = bore2_effective;

// Output: sum both radiated outputs
float output = (bore.getRadiatedOutput() + bore2.getRadiatedOutput()) * normalization * outputGain;
```

Wait -- re-reading the CONTEXT-3.4 more carefully. The backward wave mixing affects the _next iteration's_ input to each bore, not the current sample. The feedback path cross-couples bore outputs back into the other bore's input. Let me reconsider.

Actually, the per-sample processing has a one-sample delay via `prevBoreMinus`. The dual bore needs careful treatment:

**Revised per-sample loop:**
```cpp
// Get previous backward waves
float p_bore_minus = prevBoreMinus;          // bore1 backward from previous sample

if (dualBoreActive)
{
    // Reed sees combined impedance
    float bore2_bwd_prev = prevBore2Minus;
    float combined_bwd = p_bore_minus * (1.0f - feedbackPath * 0.5f)
                       + bore2_bwd_prev * (feedbackPath * 0.5f);
    p_bore_minus = combined_bwd;
}

// Reed + chamber computes p_bore_plus
float u_reed = reedModel.processSample(p_mouth, p_bore_minus);
float p_bore_plus = chamber.processSample(u_reed, p_bore_minus, Z_c);

// Both bores receive same excitation
prevBoreMinus = bore.processSample(p_bore_plus);

if (dualBoreActive)
{
    prevBore2Minus = bore2.processSample(p_bore_plus);
    output = (bore.getRadiatedOutput() + bore2.getRadiatedOutput()) * normalization * outputGain;
}
else
{
    output = bore.getRadiatedOutput() * normalization * outputGain;
}
```

**CPU when DUAL_BORE=off:** Zero additional cost -- `bore2.processSample()` never called.

### Drone Bore Tone Holes
Per CONTEXT-3.4: drone bore shares primary bore's tone hole settings. Call `bore2.updateToneHoles()` and `bore2.updateParams()` with same values as bore1.

### Reference Patterns
- O-Bowed `SympatheticStringEngine.h`: energy gating pattern (skip processing when energy < threshold)
- O-Lyrica `SympatheticResonance.h`: thread-safe coupling with fixed-size arrays

---

## 4. Drone Pitch (Parameter Range Change)

### Current State
- `PluginProcessor.cpp:256`: `dronePitch` range (-24, 24, 0.1) semitones, ParameterID version 1
- CONTEXT-3.4 requires: (-2400, 2400, 1) cents

### JUCE Parameter Versioning Research

**Key finding:** JUCE ParameterID `versionHint` primarily affects AU parameter ordering in Logic/GarageBand. It does NOT trigger automatic value migration during `setStateInformation()`.

**Existing codebase precedent:** O-FreqPulse has `ParameterID { "steps", 2 }` -- only known version-bumped parameter.

### Migration Approach

1. **Change parameter definition** (ParameterID version 1 -> 2):
```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID { "dronePitch", 2 },
    "Drone Pitch",
    juce::NormalisableRange<float>(-2400.0f, 2400.0f, 1.0f),
    0.0f,
    "cents"
));
```

2. **Add state migration in setStateInformation:**
```cpp
// Migrate dronePitch v1 (semitones) -> v2 (cents)
auto dronePitchChild = vt.getChildWithProperty("id", "dronePitch");
if (dronePitchChild.isValid())
{
    float oldValue = dronePitchChild.getProperty("value", 0.0f);
    if (std::abs(oldValue) <= 24.5f)  // Old range was -24 to 24
        dronePitchChild.setProperty("value", oldValue * 100.0f, nullptr);
}
```

3. **Update PluginEditor relay** if applicable (slider range change)

### Risk
- The plugin is pre-release (not shipped). No user state to migrate. Migration code is still good practice but low-risk.
- Default value 0.0f is identical in both scales.

---

## 5. Feedback Path (Cross-Coupling Stability)

### Current State
- `pFeedbackPath` cached but unused

### Implementation Approach

Feedback cross-couples backward waves between bore1 and bore2:
- At `feedbackPath=0`: bores independent (output mixed only)
- At `feedbackPath=1`: maximum coupling (bores share backward energy)

### Stability Analysis

The feedback path mixes backward waves: `bore1_bwd * (1-f) + bore2_bwd * f`. This is a linear mix -- the total energy is bounded by the maximum of the two bore energies when `f < 1.0`. The concern is energy accumulation over time.

**Safety mechanisms already in place:**
1. Bore loop gain < 1.0 (viscFilter + bell loss) unless infiniteSustain is high
2. Reed model has natural energy-limiting (closure clamp + flow clamp at 0.01 m^3/s)
3. tanh soft-clip at voice output (ReedWindVoice.cpp:409)
4. NaN/Inf guard with zero-reset (ReedWindVoice.cpp:403-408)

**Additional safety for extreme combos (infiniteSustain=1 + feedbackPath=1 + dualBore=on):**
Scale feedback by a damping factor to prevent runaway:
```cpp
float safeFeedback = feedbackPath * 0.5f;  // Max 50% cross-coupling
```
Or use energy-based adaptive scaling:
```cpp
float boreEnergy = bore.getEnergy() + bore2.getEnergy();
float energyScale = (boreEnergy > 1.0f) ? 1.0f / boreEnergy : 1.0f;
float safeFeedback = feedbackPath * 0.5f * energyScale;
```

**Recommendation:** Start with the simpler `feedbackPath * 0.5f` cap. The tanh at output handles edge cases. Can tighten later if instability is observed during testing.

### Codebase Patterns
- O-Lyrica SympatheticResonance: soft-clip at `SOFT_CLIP_THRESHOLD = 0.1f`
- O-Bowed WaveguideString: feedback coefficient clamped `juce::jlimit(0.9f, 0.99999f, perCycleDecay)`

---

## 6. Bore Profile (Multi-Segment Taper)

### Current State
- `BoreWaveguide::updateParams()`: uniform `halfAngle` applied to all 5 segments
- `pBoreProfile` parameter exists (Choice: Simple=0, Multi-segment=1)

### Implementation Approach

**Multi-segment mode** applies per-section taper ratios to the 5 segments:
```cpp
constexpr float taperRatios_simple[5]       = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
constexpr float taperRatios_multiSegment[5] = { 0.3f, 0.5f, 1.0f, 1.2f, 2.0f };
// Throat narrow -> Body moderate -> Bell wide flare
```

Per CONTEXT-3.4, these are preset constants. `BORE_CHARACTER` still controls overall magnitude.

```cpp
int boreProfileIdx = static_cast<int>(boreProfile);
const float* ratios = (boreProfileIdx == 1) ? taperRatios_multiSegment : taperRatios_simple;

for (int i = 0; i < 5; ++i)
{
    float effectiveHalfAngle = halfAngle * ratios[i];
    float center = /* ... reverse bore interpolated center ... */;
    float r_at_seg = r_in + center * L * std::tan(effectiveHalfAngle);
    // ... compute scale factors ...
}
```

**Sonic effect:**
- Simple: uniform taper, saxophone-like
- Multi-segment: narrow throat + wide bell = more realistic instrument-like harmonic series. Throat controls low-frequency resonance, bell section controls radiation/projection.

### Regression Safety
At `boreProfile=0` (Simple, default): all ratios = 1.0 = identical to Phase 3.3 behavior.

---

## 7. Files to Modify

| File | Changes |
|------|---------|
| `Source/DSP/BoreWaveguide.h` | Add `infiniteSustain`, `reverseBore`, `boreProfile` to `updateParams()`. Apply visc gain interpolation, bell loss reduction, reverse segment centers, multi-segment taper ratios. |
| `Source/ReedWindVoice.h` | Add `BoreWaveguide bore2`, `float prevBore2Minus` members. |
| `Source/ReedWindVoice.cpp` | Wire 6 new params in `renderNextBlock()`. Dual bore per-sample loop. Bore2 prepare/reset in noteStarted/noteStopped. Feedback path cross-coupling. |
| `Source/PluginProcessor.cpp` | Change `dronePitch` range to cents (-2400, 2400, 1), bump ParameterID to v2. Add migration in `setStateInformation()`. |
| `Source/PluginEditor.h/cpp` | Update dronePitch relay range if needed. |

---

## 8. Pitfalls & Mitigations

| Pitfall | Mitigation |
|---------|------------|
| Infinite sustain + active reed = unbounded energy | tanh soft-clip at output + reed flow clamp already in place |
| Feedback path runaway with high infiniteSustain | Cap feedback at 50% cross-coupling (`feedbackPath * 0.5f`) |
| dronePitch parameter range change breaks old state | Migration code in setStateInformation (multiply by 100) |
| Dual bore doubles CPU | Skip bore2 entirely when DUAL_BORE=off (zero cost) |
| Reverse bore at extreme values + small throatRadius = numerical edge case | `r_at_seg` always > 0 since `r_in > 0` and center * L * tan > 0 |
| Bore profile taper ratio > 1 at bell could cause extreme scale factors | Clamp `effectiveHalfAngle` to max 5 degrees |

---

## 9. Parameter Summary (6 New Active)

| Parameter | ID | Range | Default | Effect |
|-----------|----|-------|---------|--------|
| Infinite Sustain | infiniteSustain | 0-1 | 0 | Bore loop loss reduction |
| Reverse Bore | reverseBore | 0-1 | 0 | Inverted conical taper |
| Dual Bore | dualBore | bool | false | Second parallel waveguide |
| Drone Pitch | dronePitch | -2400 to 2400 cents | 0 | Bore2 frequency offset |
| Feedback Path | feedbackPath | 0-1 | 0 | Cross-coupling strength |
| Bore Profile | boreProfile | Choice (Simple/Multi) | Simple | Taper distribution |

Cumulative active: 24 (Phase 3.3) + 6 = **30 parameters**

---

## 10. JUCE API Usage

No new JUCE modules required. All features implemented with:
- Existing `juce::dsp::DelayLine` (bore2 uses same type)
- Existing `juce::dsp::IIR::Filter` (bore2 has its own instances)
- `std::pow(2.0f, cents / 1200.0f)` for cent-to-frequency conversion
- `juce::ParameterID` version hint bump (1 -> 2) for AU ordering

---

## 11. Module Opportunities

None identified. All Phase 3.4 features are specific to the bore waveguide architecture and don't warrant extraction.
