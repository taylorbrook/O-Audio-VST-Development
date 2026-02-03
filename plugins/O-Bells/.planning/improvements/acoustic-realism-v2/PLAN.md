---
milestone: acoustic-realism-v2
domain: dsp
execute_agent: dsp-agent
version_bump: minor
base_version: 1.2.0
target_version: 1.3.0
created: 2026-02-03
---

# Plan: Acoustic Realism v2

**Plugin:** O-Bells
**Milestone:** acoustic-realism-v2
**Created:** 2026-02-03
**Phase:** Plan

---

## Summary

**Improvement:** Five DSP enhancements to make O-Bells sound more realistic: shimmer quality, bloom bug fix + spectral bloom, material differentiation via dropdown, per-note inharmonicity randomization, and convolution-style attack noise with volume control.

**Approach:** Fix bloom by delaying decay until bloom completes; widen shimmer LFO range with better prime multipliers; change material parameter from Float to Choice with exaggerated properties; add Gaussian-distributed per-note pitch/amplitude variation; replace noise-based strike with impulse-driven resonant filter bank.

**Version:** 1.2.0 → 1.3.0 (MINOR)

---

## Task Breakdown

### Task 1: Widen Shimmer LFO Range and Improve Desynchronization

**Outcome:** Shimmer parameter produces organic metallic shimmer without audible LFO synchronization patterns.

**Files to modify:**
- `BellVoice.cpp` (`initializeShimmer()`, lines ~887-933)

**Changes:**
1. Change base LFO range from `0.5f, 3.0f` to `0.1f, 8.0f`
2. Replace prime LFO ratios with more spread values: `{1.0f, 1.31f, 1.73f, 2.17f, 2.71f, 3.31f, 4.13f, 5.03f}`
3. Verify random initial phase seeding is working

**Verification:**
- [ ] Build succeeds
- [ ] At shimmer=100%, no audible pulsing or synchronization artifacts
- [ ] Shimmer increases smoothly from subtle to pronounced
- [ ] A/B test vs v1.2.0 confirms improved organicness

**Dependencies:** None

**Estimated effort:** Small

---

### Task 2: Fix Bloom Bug - Delay Decay Until Bloom Completes

**Outcome:** Bloom parameter produces audible amplitude swell instead of being immediately masked by decay.

**Files to modify:**
- `BellVoice.cpp` (`applyMultiStageDecay()`, lines ~297)

**Changes:**
1. Add early return in `applyMultiStageDecay()` when `partial.bloomPhase < 1.0f`
2. This ensures bloom has time to complete before decay takes over

**Code:**
```cpp
void BellVoice::applyMultiStageDecay(ModalPartial& partial, int partialIndex)
{
    // Skip decay during bloom phase
    if (partial.bloomPhase < 1.0f)
        return;

    // ... existing decay logic
}
```

**Verification:**
- [ ] Build succeeds
- [ ] At bloom=50%, audible swell on attack
- [ ] At bloom=100%, dramatic "woooosh" as amplitude rises
- [ ] No amplitude spikes or clipping

**Dependencies:** None

**Estimated effort:** Small

---

### Task 3: Implement Spectral Bloom (Staggered Partial Timing)

**Outcome:** Higher partials fade in later than fundamental, creating spectral "opening up" effect.

**Files to modify:**
- `BellVoice.cpp` (`initializeBloom()`)

**Changes:**
1. Replace uniform bloom duration with partial-index-based timing:
   - Partials 0-1 (low): 5ms, 95% initial amplitude (near-instant)
   - Partials 2-4 (mid): 30-80ms based on bloom, 50-70% initial
   - Partials 5-7 (high): 50-150ms based on bloom, 15-50% initial
2. Higher partials now "bloom in" creating spectral movement

**Verification:**
- [ ] Build succeeds
- [ ] At bloom=0%, instant full spectrum (no spectral bloom)
- [ ] At bloom=100%, audible high-partial fade-in over ~150ms
- [ ] Fundamental always present immediately

**Dependencies:** Task 2 (bloom must work first)

**Estimated effort:** Medium

---

### Task 4: Add Gaussian Random Helper Function

**Outcome:** Real-time safe Gaussian random number generator available for per-note variation.

**Files to modify:**
- `BellVoice.h` (add private helper)
- `BellVoice.cpp` (implement if complex)

**Changes:**
1. Add `gaussianRandom()` helper using Box-Muller transform or CLT approximation
2. Use allocation-free implementation with static state

**Code option (CLT approximation - simpler):**
```cpp
// In BellVoice.h, private section:
float gaussianApprox() {
    // Sum of 3 uniform randoms approximates Gaussian (Central Limit Theorem)
    float sum = 0.0f;
    for (int i = 0; i < 3; ++i)
        sum += (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
    return sum / 1.73f;  // Scale to ~unit variance
}
```

**Verification:**
- [ ] Build succeeds
- [ ] Function returns values roughly in [-2, 2] range
- [ ] No allocations (verify with sanitizer if needed)

**Dependencies:** None

**Estimated effort:** Small

---

### Task 5: Add Per-Note Inharmonicity Randomization

**Outcome:** Each note strike has subtle pitch and amplitude variations across partials, making repeated notes sound organic.

**Files to modify:**
- `BellVoice.cpp` (`initializePartials()`, and octave layer loops in `startNote()`)

**Changes:**
1. In `initializePartials()`, add per-partial pitch offset (±10 cents std dev)
2. Add per-partial amplitude variation (±25% std dev, clamped 50%-150%)
3. Apply same randomization to sub-octave and upper-octave initializations

**Code:**
```cpp
// After frequency calculation:
float pitchOffset = gaussianApprox() * 10.0f;  // ±10 cents
partial.frequency *= std::pow(2.0f, pitchOffset / 1200.0f);
partial.phaseIncrement = partial.frequency / static_cast<float>(currentSampleRate);

// After amplitude calculation:
float ampVariation = 1.0f + gaussianApprox() * 0.25f;
ampVariation = juce::jlimit(0.5f, 1.5f, ampVariation);
partial.amplitude *= ampVariation;
```

**Verification:**
- [ ] Build succeeds
- [ ] 10 repeated strikes of same note sound subtly different
- [ ] Bell identity preserved (not chaotic)
- [ ] Variations are subtle, not jarring

**Dependencies:** Task 4 (needs gaussianApprox helper)

**Estimated effort:** Medium

---

### Task 6: Change Material Parameter from Float to Choice

**Outcome:** Material is a dropdown with 5 discrete options instead of continuous slider.

**Files to modify:**
- `PluginProcessor.cpp` (`createParameterLayout()`)
- `PluginProcessor.h` (parameter pointer type comment update)

**Changes:**
1. Replace `AudioParameterFloat` with `AudioParameterChoice`:
```cpp
layout.add(std::make_unique<juce::AudioParameterChoice>(
    juce::ParameterID { "material", 1 },
    "Material",
    juce::StringArray { "Bronze", "Brass", "Steel", "Aluminum", "Cast Iron" },
    0  // Default: Bronze
));
```

**Verification:**
- [ ] Build succeeds
- [ ] Parameter stores discrete values (0, 1, 2, 3, 4)
- [ ] DAW shows dropdown in generic UI

**Dependencies:** None

**Estimated effort:** Small

---

### Task 7: Update Material Relay/Attachment to ComboBox

**Outcome:** Editor uses WebComboBoxRelay for material parameter.

**Files to modify:**
- `PluginEditor.h` (change relay/attachment types)
- `PluginEditor.cpp` (change creation code)

**Changes:**
1. Change `materialRelay` from `WebSliderRelay` to `WebComboBoxRelay`
2. Change `materialAttachment` from `WebSliderParameterAttachment` to `WebComboBoxParameterAttachment`
3. Update creation in constructor

**In PluginEditor.h:**
```cpp
// Change:
std::unique_ptr<juce::WebSliderRelay> materialRelay;
std::unique_ptr<juce::WebSliderParameterAttachment> materialAttachment;
// To:
std::unique_ptr<juce::WebComboBoxRelay> materialRelay;
std::unique_ptr<juce::WebComboBoxParameterAttachment> materialAttachment;
```

**Verification:**
- [ ] Build succeeds
- [ ] Material relay communicates with JavaScript as combobox

**Dependencies:** Task 6 (parameter must be Choice first)

**Estimated effort:** Small

---

### Task 8: Simplify getMaterialProperties to Discrete Lookup

**Outcome:** Material properties looked up by discrete index, no interpolation.

**Files to modify:**
- `BellVoice.cpp` (`getMaterialProperties()`)

**Changes:**
1. Replace interpolation logic with discrete switch:
```cpp
BellVoice::MaterialProperties BellVoice::getMaterialProperties(float material)
{
    // Choice parameter normalizes to 0.0, 0.25, 0.5, 0.75, 1.0
    int index = static_cast<int>(std::round(material * 4.0f));
    index = juce::jlimit(0, 4, index);

    switch (index)
    {
        case 0: return MATERIAL_BRONZE;
        case 1: return MATERIAL_BRASS;
        case 2: return MATERIAL_STEEL;
        case 3: return MATERIAL_ALUMINUM;
        case 4: return MATERIAL_CAST_IRON;
        default: return MATERIAL_BRONZE;
    }
}
```

**Verification:**
- [ ] Build succeeds
- [ ] Each material selection returns correct properties
- [ ] No interpolation between materials

**Dependencies:** Task 6 (parameter type change)

**Estimated effort:** Small

---

### Task 9: Exaggerate Material Property Values

**Outcome:** Materials are audibly distinct from each other.

**Files to modify:**
- `BellVoice.h` (material constant definitions, lines ~64-68)

**Changes:**
1. Update `MATERIAL_*` constexpr values to exaggerated research-based properties:

```cpp
// EXAGGERATED for audible differentiation
static constexpr MaterialProperties MATERIAL_BRONZE     = {1.0f,  0.0f,   0.0f};    // Baseline warm
static constexpr MaterialProperties MATERIAL_BRASS      = {0.7f,  +0.20f, +0.08f};  // Bright, short, jazzy
static constexpr MaterialProperties MATERIAL_STEEL      = {2.0f,  +0.25f, -0.05f};  // Very bright, long sustain
static constexpr MaterialProperties MATERIAL_ALUMINUM   = {0.5f,  +0.30f, +0.12f};  // Very bright, short, thin
static constexpr MaterialProperties MATERIAL_CAST_IRON  = {1.5f,  -0.25f, +0.15f};  // Dark, long, gamelan-like
```

**Verification:**
- [ ] Build succeeds
- [ ] Blind test can identify each material by ear
- [ ] Bronze = warm baseline, Steel = bright/long, Cast Iron = dark/gamelan

**Dependencies:** Task 8 (discrete lookup must work)

**Estimated effort:** Small

---

### Task 10: Update Material UI to Dropdown

**Outcome:** WebView renders material as dropdown select instead of slider.

**Files to modify:**
- `ui/index.html` (material control element)

**Changes:**
1. Replace material slider with `<select>` element
2. Add JavaScript event handling for combobox relay
3. Style dropdown to match Ouaricon aesthetic

**Verification:**
- [ ] UI shows dropdown with 5 material options
- [ ] Selection syncs with DAW parameter
- [ ] Preset loading sets correct material

**Dependencies:** Task 7 (relay must be ComboBox)

**Estimated effort:** Medium

---

### Task 11: Add attackLevel Parameter Definition

**Outcome:** New parameter "Attack" (0-100%) controls transient volume.

**Files to modify:**
- `PluginProcessor.cpp` (`createParameterLayout()`)
- `PluginProcessor.h` (add `attackLevelParam` pointer)

**Changes:**
1. Add parameter after `strikeNoiseChar`:
```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID { "attackLevel", 1 },
    "Attack",
    juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
    0.5f,  // Default 50% (natural level)
    "%"
));
```
2. Add pointer in header: `std::atomic<float>* attackLevelParam = nullptr;`
3. Cache in `prepareToPlay()`: `attackLevelParam = parameters.getRawParameterValue("attackLevel");`

**Verification:**
- [ ] Build succeeds
- [ ] Parameter shows in DAW generic UI
- [ ] Default value is 50%

**Dependencies:** None

**Estimated effort:** Small

---

### Task 12: Add Attack Level Relay and Attachment

**Outcome:** Editor has WebSliderRelay for Attack parameter.

**Files to modify:**
- `PluginEditor.h` (add relay/attachment declarations)
- `PluginEditor.cpp` (create relay/attachment)

**Changes:**
1. Add relay declaration: `std::unique_ptr<juce::WebSliderRelay> attackLevelRelay;`
2. Add attachment: `std::unique_ptr<juce::WebSliderParameterAttachment> attackLevelAttachment;`
3. Create in constructor (maintain CRITICAL ORDER: relays before WebView, attachments after)

**Verification:**
- [ ] Build succeeds
- [ ] Relay created and attached to parameter

**Dependencies:** Task 11 (parameter must exist)

**Estimated effort:** Small

---

### Task 13: Pass attackLevel to BellVoice

**Outcome:** Voice receives attack level value for noise generation.

**Files to modify:**
- `BellVoice.h` (add `currentAttackLevel` member, update `updateParameters` signature)
- `BellVoice.cpp` (`updateParameters()` implementation)
- `PluginProcessor.cpp` (`processBlock()` - pass value to voice)

**Changes:**
1. Add member: `float currentAttackLevel = 0.5f;`
2. Update `updateParameters()` signature to include `float attackLevel`
3. Store in `currentAttackLevel`
4. In processor's `processBlock()`, read param and pass to voice update

**Verification:**
- [ ] Build succeeds
- [ ] Voice receives attack level value

**Dependencies:** Task 11 (parameter must exist)

**Estimated effort:** Small

---

### Task 14: Add Resonant Filter Bank to StrikeExciter

**Outcome:** Strike exciter has resonant filter bank instead of simple filtered noise.

**Files to modify:**
- `BellVoice.h` (`StrikeExciter` struct, lines ~101-117)

**Changes:**
1. Add resonator array and gains:
```cpp
struct StrikeExciter
{
    // Existing fields...

    // NEW: Resonant filter bank for impulse response
    static constexpr int NUM_RESONATORS = 4;
    juce::dsp::StateVariableTPTFilter<float> resonators[NUM_RESONATORS];
    float resonatorGains[NUM_RESONATORS] = {1.0f, 1.0f, 1.0f, 1.0f};

    // Impulse state
    int impulseSamplesRemaining = 0;
    float impulseAmplitude = 1.0f;
};
```

**Verification:**
- [ ] Build succeeds
- [ ] No undefined reference errors

**Dependencies:** None

**Estimated effort:** Small

---

### Task 15: Initialize Resonators in startNote

**Outcome:** Resonant filters tuned to first 4 partials with Q based on strike character.

**Files to modify:**
- `BellVoice.cpp` (`startNote()`, after fundamental calculation)

**Changes:**
1. Configure each resonator with partial frequency
2. Set Q based on `currentStrikeNoiseChar` (Click=high Q, Thud=low Q, Ping=medium Q)
3. Set gains based on mallet hardness
4. Set impulse duration to 2 samples

**Verification:**
- [ ] Build succeeds
- [ ] Resonators configured when note starts

**Dependencies:** Task 14 (struct must have fields)

**Estimated effort:** Medium

---

### Task 16: Implement Impulse-Driven generateStrikeNoise

**Outcome:** Strike noise generated via impulse through resonant filter bank.

**Files to modify:**
- `BellVoice.cpp` (`generateStrikeNoise()`)

**Changes:**
1. Generate impulse for first 2 samples of note
2. Process impulse through resonator bank in parallel
3. Sum resonator outputs with their gains
4. Apply overall decay envelope
5. Scale output by `currentAttackLevel * 2.0f` (so 50% = original level)

**Verification:**
- [ ] Build succeeds
- [ ] Strike noise sounds like metallic contact, not filtered noise
- [ ] Click = bright/short, Thud = soft/longer, Ping = metallic/resonant

**Dependencies:** Task 14, Task 15 (resonators must exist and be initialized)

**Estimated effort:** Medium

---

### Task 17: Add Attack Slider to UI

**Outcome:** WebView has Attack slider in appropriate section.

**Files to modify:**
- `ui/index.html`

**Changes:**
1. Add Attack slider in Advanced or Main panel (near Strike Noise Character)
2. Style to match existing sliders
3. Wire to `attackLevel` relay

**Verification:**
- [ ] UI shows Attack slider
- [ ] Slider syncs with DAW parameter
- [ ] At 0%, minimal transient; at 100%, exaggerated transient

**Dependencies:** Task 12 (relay must exist)

**Estimated effort:** Small

---

### Task 18: Build and Integration Test

**Outcome:** All changes compile, plugin loads, and improvements are audible.

**Changes:**
1. Full build: `ninja O-Bells_VST3 O-Bells_AU`
2. Clear AU cache and install
3. Load in DAW
4. Test each improvement

**Verification:**
- [ ] Build succeeds without warnings
- [ ] Plugin loads without crash
- [ ] Shimmer: no sync artifacts
- [ ] Bloom: audible swell at 50%+
- [ ] Materials: each sounds distinct
- [ ] Repeated notes: subtle variation
- [ ] Attack: slider controls transient level

**Dependencies:** All previous tasks

**Estimated effort:** Medium

---

## Dependency Graph

```
Task 1 (Shimmer) ──────────────────────────────────────────────────────────┐
                                                                           │
Task 2 (Bloom Fix) ──────> Task 3 (Spectral Bloom) ───────────────────────┤
                                                                           │
Task 4 (Gaussian) ──────> Task 5 (Inharmonicity Random) ──────────────────┤
                                                                           │
Task 6 (Material Param) ──> Task 7 (Relay) ──> Task 8 (Lookup) ──>        │
                                               Task 9 (Values) ──>         │
                                               Task 10 (UI) ──────────────┤
                                                                           │
Task 11 (Attack Param) ──> Task 12 (Relay) ──> Task 13 (Pass to Voice) ──┤
                                                                           │
Task 14 (Resonator Struct) ──> Task 15 (Init) ──> Task 16 (Generate) ────┤
                               Task 17 (Attack UI) ──────────────────────┤
                                                                           │
                                                                           v
                                                                   Task 18 (Build Test)
```

**Parallelizable waves:**
- Wave 1: Tasks 1, 2, 4, 6, 11, 14 (no dependencies)
- Wave 2: Tasks 3, 5, 7, 12, 15 (depend on Wave 1)
- Wave 3: Tasks 8, 13, 16, 17 (depend on Wave 2)
- Wave 4: Tasks 9, 10 (depend on Wave 3)
- Wave 5: Task 18 (depends on all)

---

## Execution Order

| Wave | Task | Description | Dependencies |
|------|------|-------------|--------------|
| 1 | 1 | Widen shimmer LFO range | None |
| 1 | 2 | Fix bloom bug | None |
| 1 | 4 | Add Gaussian helper | None |
| 1 | 6 | Change material to Choice | None |
| 1 | 11 | Add attackLevel param | None |
| 1 | 14 | Add resonator struct | None |
| 2 | 3 | Implement spectral bloom | 2 |
| 2 | 5 | Per-note randomization | 4 |
| 2 | 7 | Material relay to ComboBox | 6 |
| 2 | 12 | Attack relay | 11 |
| 2 | 15 | Initialize resonators | 14 |
| 3 | 8 | Discrete material lookup | 6 |
| 3 | 13 | Pass attack to voice | 11 |
| 3 | 16 | Impulse-driven noise | 14, 15 |
| 3 | 17 | Attack UI slider | 12 |
| 4 | 9 | Exaggerate material values | 8 |
| 4 | 10 | Material dropdown UI | 7 |
| 5 | 18 | Build and integration test | All |

---

## Risk Notes

1. **Bloom bug fix may cause amplitude spikes**
   - Impact: Clipping or distortion if bloom overshoots
   - Mitigation: Bloom multiplier clamped to max 1.0; tested at bloom=100%

2. **Material parameter change breaks preset compatibility**
   - Impact: Old presets may not load material correctly
   - Mitigation: This is expected (MINOR version bump); documented in CHANGELOG

3. **Resonant filter bank CPU cost**
   - Impact: Higher CPU usage per voice
   - Mitigation: Only 4 resonators; using processSample not blocks; measure with profiler

4. **Very slow shimmer LFOs (0.1 Hz) may cause audible pitch drift**
   - Impact: Notes may sound out of tune with high shimmer
   - Mitigation: Shimmer depth parameter still limits actual cents deviation

---

## Domain Agent Instructions

**Execute Agent:** dsp-agent

**Domain-specific rules to follow:**
- No allocations in processBlock or renderNextBlock
- All per-sample operations must be real-time safe
- Use juce::jlimit for all clamping to prevent out-of-range values
- Maintain member declaration order: relays → WebView → attachments
- StateVariableTPTFilter reset() must be called in startNote, not renderNextBlock

**Files the agent should read first:**
- `BellVoice.h` - Understand struct layouts and member organization
- `BellVoice.cpp` - Understand current bloom/shimmer/strike implementations
- `PluginProcessor.cpp` - Understand parameter layout pattern
- `PluginEditor.h` - Understand relay/attachment declaration order

---

## Success Criteria

From CONTEXT.md, the improvement is successful when:

1. [ ] **Shimmer**: No audible LFO synchronization at any setting
2. [ ] **Bloom**: Clearly audible spectral "opening" at bloom > 50%
3. [ ] **Materials**: Blind test can identify each material by ear
4. [ ] **Inharmonicity**: 10 repeated strikes sound subtly different
5. [ ] **Attack**: Transients sound like physical mallet impact
6. [ ] All tasks completed and verified
7. [ ] Build succeeds without warnings
8. [ ] Pluginval passes (Level 5+)

---

## Approval

```
Approve this plan?

1. Yes, proceed with execution
2. No, revise the plan
3. No, cancel milestone
4. Other

Choose (1-4): _
```

---

*Generated by improve-milestone plan phase*
