# Phase 3.4 Execution Plan: Impossible Physics + Dual Bore

**Created:** 2026-04-05
**Stage:** 2 (DSP)
**Phase:** 3.4
**Goal:** Add infinite sustain, reverse bore, dual bore with drone pitch, feedback path cross-coupling, and multi-segment bore profile. Change `dronePitch` parameter from semitones to cents. All features bypass at default values -- Phase 3.3 regression guaranteed.

---

## Tasks

### 1. [ ] Extend BoreWaveguide::updateParams with infiniteSustain, reverseBore, boreProfile

**Files:** `Source/DSP/BoreWaveguide.h`

**Changes:**

Expand `updateParams()` signature to accept 3 new parameters:
```cpp
void updateParams(float boreCharacter, float bellSize, float boreDiameter, float boreLength,
                  float infiniteSustain = 0.0f, float reverseBore = 0.0f, float boreProfile = 0.0f)
```

**Infinite sustain (lines ~130-136, visc + bell filter section):**
- Viscothermal gain: `float g = 0.995f + infiniteSustain * 0.005f;` (approaches 1.0)
- Bell cutoff: `float sustainedBellCutoff = bellCutoff + infiniteSustain * (sr * 0.499f - bellCutoff);`
- Use `sustainedBellCutoff` instead of `bellCutoff` for bell filter coefficient computation
- At `infiniteSustain=0`: `g=0.995`, bellCutoff unchanged (identical to Phase 3.3)
- At `infiniteSustain=1`: `g=1.0`, bell filter → total reflection (lossless bore loop)

**Reverse bore (lines ~96-121, conical scaling section):**
- Add reversed center positions:
  ```cpp
  constexpr float normalCenters[5]   = { 0.05f, 0.20f, 0.40f, 0.625f, 0.875f };
  constexpr float reversedCenters[5] = { 0.95f, 0.80f, 0.60f, 0.375f, 0.125f };
  ```
- Interpolate: `float center = normalCenters[i] + reverseBore * (reversedCenters[i] - normalCenters[i]);`
- Use `center` instead of `segCenters[i]` in scale factor computation
- At `reverseBore=0`: normal. At `reverseBore=1`: inverted (hichiriki-like)
- Only audible when `boreCharacter > 0` (cylindrical has halfAngle=0 → all scales = 1.0)

**Bore profile multi-segment taper (lines ~96-121, same scaling section):**
- Add taper ratio arrays:
  ```cpp
  constexpr float taperSimple[5]   = { 1.0f, 1.0f, 1.0f, 1.0f, 1.0f };
  constexpr float taperMulti[5]    = { 0.3f, 0.5f, 1.0f, 1.2f, 2.0f };
  ```
- Select ratios based on `boreProfile` (0=Simple, 1=Multi-segment)
- Apply: `float effectiveHalfAngle = halfAngle * ratios[i];`
- Clamp `effectiveHalfAngle` to max 5 degrees to prevent extreme scale factors
- At `boreProfile=0`: all ratios = 1.0 (identical to Phase 3.3)

**Regression:** All three features have default=0 → all code paths reduce to Phase 3.3 behavior.

**Depends on:** None

---

### 2. [ ] Add bore2 member + prepare/reset to ReedWindVoice

**Files:** `Source/ReedWindVoice.h`, `Source/ReedWindVoice.cpp`

**Header changes (ReedWindVoice.h):**
- Add member: `BoreWaveguide bore2;`
- Add member: `float prevBore2Minus = 0.0f;`

**prepare() (ReedWindVoice.cpp ~77-95):**
- Add: `bore2.prepare(sampleRate, maxBlockSize);`
- Add: `prevBore2Minus = 0.0f;`

**noteStarted() normal onset (ReedWindVoice.cpp ~124-137):**
- Add: `bore2.reset();` alongside existing `bore.reset()`
- Add: `prevBore2Minus = 0.0f;`

**noteStarted() legato path (ReedWindVoice.cpp ~101-118):**
- If dual bore active, also update bore2 params and frequency (same tone holes as bore1)

**noteStopped() hard stop (ReedWindVoice.cpp ~190-201):**
- Add: `bore2.reset();`
- Add: `prevBore2Minus = 0.0f;`

**Depends on:** None (can run in parallel with Task 1)

---

### 3. [ ] Wire 6 new parameters in renderNextBlock per-sample loop

**Files:** `Source/ReedWindVoice.cpp`

**Per-block reads (after line ~269):**
Add reads for 6 new parameters:
```cpp
float infiniteSustain = (pInfiniteSustain != nullptr) ? pInfiniteSustain->load() : 0.0f;
float reverseBore     = (pReverseBore != nullptr) ? pReverseBore->load() : 0.0f;
bool  dualBoreActive  = (pDualBore != nullptr) && (pDualBore->load() > 0.5f);
float dronePitchCents = (pDronePitch != nullptr) ? pDronePitch->load() : 0.0f;
float feedbackPath    = (pFeedbackPath != nullptr) ? pFeedbackPath->load() : 0.0f;
float boreProfileVal  = (pBoreProfile != nullptr) ? pBoreProfile->load() : 0.0f;
```

**Pass new params to bore.updateParams() (line ~289):**
```cpp
bore.updateParams(boreCharacter, bellSize, boreDiameter, boreLength,
                  infiniteSustain, reverseBore, boreProfileVal);
```

**Wire bore2 (if dual bore active), after bore.updateParams():**
```cpp
if (dualBoreActive)
{
    bore2.updateParams(boreCharacter, bellSize, boreDiameter, boreLength,
                       infiniteSustain, reverseBore, boreProfileVal);
    bore2.updateToneHoles(toneHoleCutoff, registerHole);

    float bore2Hz = frequency * std::pow(2.0f, dronePitchCents / 1200.0f);
    bore2.setFrequency(bore2Hz);
}
```

**Rewrite per-sample loop steps 4, 8, 9 (lines ~376-400) for dual bore + feedback:**

```cpp
// 4. Get bore feedback (wave arriving at reed from previous sample)
float p_bore_minus = prevBoreMinus;

if (dualBoreActive)
{
    // Reed sees combined impedance from both bores
    float safeFeedback = feedbackPath * 0.5f;  // Cap at 50%
    p_bore_minus = prevBoreMinus * (1.0f - safeFeedback)
                 + prevBore2Minus * safeFeedback;
}

// ... steps 5-7 unchanged (reed, chamber, mouthpiece) ...

// 8. Bore waveguide(s)
prevBoreMinus = bore.processSample(p_bore_plus);

float output;
if (dualBoreActive)
{
    prevBore2Minus = bore2.processSample(p_bore_plus);
    output = (bore.getRadiatedOutput() + bore2.getRadiatedOutput())
             * normalization * outputGain;
}
else
{
    output = bore.getRadiatedOutput() * normalization * outputGain;
}
```

**Post-block (after loop, line ~417):**
- Add: `if (dualBoreActive) bore2.snapFiltersToZero();`

**Depends on:** Tasks 1, 2

---

### 4. [ ] Change dronePitch parameter: semitones -> cents

**Files:** `Source/PluginProcessor.cpp`, `Source/PluginEditor.h`, `Source/PluginEditor.cpp`

**PluginProcessor.cpp (line ~254-261):**
Change parameter definition:
```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID { "dronePitch", 2 },  // Version bump 1->2
    "Drone Pitch",
    juce::NormalisableRange<float>(-2400.0f, 2400.0f, 1.0f),
    0.0f,
    "cents"
));
```

**PluginProcessor.cpp setStateInformation():**
Add migration code for old v1 state (semitones -> cents):
```cpp
auto dronePitchChild = vt.getChildWithProperty("id", "dronePitch");
if (dronePitchChild.isValid())
{
    float oldValue = dronePitchChild.getProperty("value", 0.0f);
    if (std::abs(oldValue) <= 24.5f)  // Was in old -24..24 semitone range
        dronePitchChild.setProperty("value", oldValue * 100.0f, nullptr);
}
```

**PluginEditor:** dronePitchRelay is a WebSliderRelay with no explicit range in the C++ constructor -- it picks up range from the APVTS parameter via the attachment. No editor-side changes needed unless explicit range overrides exist.

**Risk:** Low -- pre-release plugin, no user state. Default 0.0 is identical in both scales.

**Depends on:** None (can run in parallel with Tasks 1-2)

---

### 5. [ ] Build, test, validate

**Files:** Build system output

**Steps:**
1. `ninja O-Reed_VST3 O-Reed_AU` -- zero errors, zero warnings
2. Clear AU cache + install fresh binaries
3. `auval -v aumu ORed OuDv` -- PASS
4. `pluginval --validate O-Reed-dev.vst3 --strictness-level 5` -- PASS
5. Spot-check: all 6 new params at default → identical to Phase 3.3
6. Spot-check: infinite sustain at 1.0 → note sustains after release
7. Spot-check: dual bore on + drone pitch -1200 → octave below drone
8. Spot-check: feedback path at 1.0 + infinite sustain at 1.0 → stable (no runaway)

**Depends on:** Tasks 1-4

---

## File Summary

| File | Action | Tasks |
|------|--------|-------|
| `Source/DSP/BoreWaveguide.h` | Modify | 1 |
| `Source/ReedWindVoice.h` | Modify | 2 |
| `Source/ReedWindVoice.cpp` | Modify | 2, 3 |
| `Source/PluginProcessor.cpp` | Modify | 4 |

---

## Success Criteria

- [ ] INFINITE_SUSTAIN at 0: identical to Phase 3.3 (bore decays normally)
- [ ] INFINITE_SUSTAIN at 1: tone sustains indefinitely after note-off
- [ ] REVERSE_BORE at 0: identical to Phase 3.3
- [ ] REVERSE_BORE at 1 + boreCharacter > 0: unusual timbral character
- [ ] DUAL_BORE off: zero additional CPU cost
- [ ] DUAL_BORE on: second bore audible at DRONE_PITCH offset
- [ ] DRONE_PITCH at 0 cents: unison chorusing
- [ ] DRONE_PITCH at -1200 cents: octave below drone
- [ ] DRONE_PITCH at -700 cents: fifth below (arghul-like)
- [ ] FEEDBACK_PATH at 0: bores independent
- [ ] FEEDBACK_PATH at 1.0 + INFINITE_SUSTAIN at 1.0: stable, no runaway
- [ ] BORE_PROFILE Simple: identical to Phase 3.3
- [ ] BORE_PROFILE Multi-segment: audible difference (throat/body/bell taper)
- [ ] All 6 params at default: identical to Phase 3.3 (regression)
- [ ] VST3 + AU build zero errors
- [ ] auval PASS
- [ ] pluginval Level 5 PASS
- [ ] 30 active parameters confirmed

---

## Dependency Graph

```
Task 1 (BoreWaveguide)  ─┐
Task 2 (bore2 members)   ├─→ Task 3 (renderNextBlock wiring) ─→ Task 5 (build+validate)
Task 4 (dronePitch param)┘
```

Tasks 1, 2, 4 have no dependencies and can execute in parallel.
Task 3 depends on 1+2.
Task 5 depends on all.
