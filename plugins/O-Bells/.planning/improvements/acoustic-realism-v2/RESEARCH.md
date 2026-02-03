# O-Bells Acoustic Realism v2 - Research Findings

## Executive Summary

Analysis of the current O-Bells implementation reveals five targeted improvements needed for v1.3.0. The bloom implementation has a critical bug where amplitude is immediately overwritten by `applyMultiStageDecay()` before bloom can take effect. The shimmer LFO system uses a narrow rate range (0.5-3 Hz) and prime multipliers that create audible synchronization artifacts. Material differentiation is limited by subtle property differences and continuous slider interpolation. Implementation of all five improvements is achievable within the existing `BellVoice` architecture with minimal structural changes.

---

## 1. Shimmer Quality

### Current Implementation

**Location:** `BellVoice.cpp`, lines 887-933

The shimmer system uses per-partial LFO frequency modulation:

```cpp
// LFO rates use prime multipliers but narrow range
static const float primeLFORatios[NUM_PARTIALS] = {1.0f, 1.1f, 1.3f, 1.7f, 1.9f, 2.3f, 2.9f, 3.1f};
float baseLFOFreq = juce::jmap(currentShimmer, 0.5f, 3.0f);  // Limited range
partial.shimmerLFORate = baseLFOFreq * primeLFORatios[partialIndex] / static_cast<float>(currentSampleRate);
```

**Issues identified:**
1. Base LFO range is only 0.5-3 Hz (requirement: 0.1-8 Hz)
2. Prime multipliers (1.1, 1.3, 1.7...) create clustered rates that still phase-lock
3. All LFOs use sine waves, creating predictable patterns
4. Random initial phases exist but prime ratio spacing still creates audible synchronization

### Recommended Approach

1. **Widen LFO rate range:** Change from `0.5f, 3.0f` to `0.1f, 8.0f`
2. **Use larger prime multipliers:** Replace with more spread values like `{1.0f, 1.31f, 1.73f, 2.17f, 2.71f, 3.31f, 4.13f, 5.03f}` (primes converted to multipliers)
3. **Verify random initial phase seeding:** Current `rand()` usage is correct but could benefit from std::random for better distribution

**Code change in `initializeShimmer()`:**
```cpp
// BEFORE
float baseLFOFreq = juce::jmap(currentShimmer, 0.5f, 3.0f);

// AFTER
float baseLFOFreq = juce::jmap(currentShimmer, 0.1f, 8.0f);
static const float primeLFORatios[NUM_PARTIALS] = {1.0f, 1.31f, 1.73f, 2.17f, 2.71f, 3.31f, 4.13f, 5.03f};
```

### Files to Modify

- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/BellVoice.cpp` (lines 887-933)

---

## 2. Bloom Fix + Spectral Bloom

### Bug Analysis

**Critical Bug Found:** The bloom amplitude modulation is being immediately overwritten by `applyMultiStageDecay()`.

**Execution order in `renderNextBlock()` (lines 291-297):**
```cpp
// Apply bloom (spectral swelling before decay)
applyBloom(partial);                           // Line 294: Sets amplitude correctly

// Apply multi-stage decay (always active in v1.2.0)
applyMultiStageDecay(partial, p);              // Line 297: IMMEDIATELY OVERWRITES amplitude!
```

**Bug trace:**
1. `initializeBloom()` correctly sets `partial.amplitude = partial.initialAmplitude` (30-70% of peak)
2. `applyBloom()` correctly interpolates amplitude toward `peakAmplitude` over bloom duration
3. `applyMultiStageDecay()` runs on the SAME sample and applies decay coefficient, fighting the bloom swell
4. Since `strikeDecayCoeffs[p]` decays faster than bloom can swell (especially for high partials), bloom effect is masked

**Fix required:** Bloom phase must complete BEFORE decay begins, or bloom must additively offset the decay target.

### Current Implementation

**Bloom initialization** (`BellVoice.cpp:834-857`):
```cpp
void BellVoice::initializeBloom(ModalPartial& partial, int partialIndex)
{
    // Bloom duration: 10ms (subtle) to 100ms (pronounced)
    float bloomDurationMs = juce::jmap(currentBloom, 10.0f, 100.0f);

    // Initial amplitude: start at 30-70% of target
    float initialFraction = juce::jmap(currentBloom, 0.7f, 0.3f);
    partial.initialAmplitude = partial.amplitude * initialFraction;
    partial.peakAmplitude = partial.amplitude;
    partial.amplitude = partial.initialAmplitude;
}
```

**Bloom application** (`BellVoice.cpp:859-880`):
- Uses cosine interpolation (smooth)
- Does NOT account for decay occurring simultaneously

### Recommended Approach

**Part A: Fix Amplitude Bloom Bug**

Option 1 (Preferred): Delay decay until bloom completes
```cpp
void BellVoice::applyMultiStageDecay(ModalPartial& partial, int partialIndex)
{
    // Skip decay during bloom phase
    if (partial.bloomPhase < 1.0f)
        return;

    // ... existing decay logic
}
```

Option 2: Bloom modulates peak amplitude, not current amplitude
```cpp
void BellVoice::applyBloom(ModalPartial& partial)
{
    if (partial.bloomPhase >= 1.0f)
        return;

    // Calculate bloom multiplier instead of absolute amplitude
    float cosineT = (1.0f - std::cos(partial.bloomPhase * juce::MathConstants<float>::pi)) * 0.5f;
    float bloomMultiplier = juce::jmap(cosineT, partial.initialFraction, 1.0f);

    // Apply as multiplier so decay can work on top
    partial.bloomMultiplier = bloomMultiplier;
    partial.bloomPhase += partial.bloomRate;
}
```
Then in `renderNextBlock()`: `voiceSample += partialSample * partial.amplitude * partial.bloomMultiplier;`

**Part B: Implement Spectral Bloom**

Extend bloom duration based on partial index:
```cpp
void BellVoice::initializeBloom(ModalPartial& partial, int partialIndex)
{
    if (currentBloom <= 0.0f)
    {
        partial.bloomPhase = 1.0f;
        return;
    }

    // Spectral bloom: stagger timing by partial
    // Low partials (0-1): instant (no bloom delay)
    // Mid partials (2-4): 50ms bloom
    // High partials (5-7): 80-150ms bloom (scales with bloom parameter)

    float baseDuration;
    float initialFraction;

    if (partialIndex < 2)
    {
        // Low partials: minimal bloom, start at full amplitude
        baseDuration = 5.0f;  // 5ms (near-instant)
        initialFraction = 0.95f;
    }
    else if (partialIndex < 5)
    {
        // Mid partials: moderate bloom
        baseDuration = juce::jmap(currentBloom, 30.0f, 80.0f);
        initialFraction = juce::jmap(currentBloom, 0.7f, 0.5f);
    }
    else
    {
        // High partials: pronounced bloom (creates "opening up" effect)
        baseDuration = juce::jmap(currentBloom, 50.0f, 150.0f);
        initialFraction = juce::jmap(currentBloom, 0.5f, 0.15f);  // Start very quiet
    }

    // ... rest of initialization
}
```

### Files to Modify

- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/BellVoice.h` (may need `bloomMultiplier` field)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/BellVoice.cpp` (initializeBloom, applyBloom, applyMultiStageDecay, renderNextBlock)

---

## 3. Material Differentiation

### Current Implementation

**Material property definitions** (`BellVoice.h:56-68`):
```cpp
static constexpr MaterialProperties MATERIAL_BRONZE     = {1.0f,  0.0f,   0.0f};
static constexpr MaterialProperties MATERIAL_BRASS      = {0.9f,  +0.05f, +0.02f};
static constexpr MaterialProperties MATERIAL_STEEL      = {1.4f,  +0.10f, +0.01f};
static constexpr MaterialProperties MATERIAL_ALUMINUM   = {0.7f,  +0.15f, +0.05f};
static constexpr MaterialProperties MATERIAL_CAST_IRON  = {1.2f,  -0.10f, +0.03f};
```

**Material interpolation** (`BellVoice.cpp:538-581`):
- Continuous slider (0.0-1.0) interpolates between adjacent materials
- Creates smooth transitions but blurs distinction between materials

**Current UI:** Material is an `AudioParameterFloat` slider in `PluginProcessor.cpp:59-64`:
```cpp
layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID { "material", 1 },
    "Material",
    juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
    0.25f
));
```

### Dropdown Implementation

**Step 1: Change parameter type to `AudioParameterChoice`**

In `PluginProcessor.cpp`, replace the material parameter:
```cpp
// Replace AudioParameterFloat with AudioParameterChoice
layout.add(std::make_unique<juce::AudioParameterChoice>(
    juce::ParameterID { "material", 1 },
    "Material",
    juce::StringArray { "Bronze", "Brass", "Steel", "Aluminum", "Cast Iron" },
    0  // Default: Bronze
));
```

**Step 2: Update PluginEditor to use WebComboBoxRelay**

In `PluginEditor.h`:
```cpp
// Change from:
std::unique_ptr<juce::WebSliderRelay> materialRelay;
std::unique_ptr<juce::WebSliderParameterAttachment> materialAttachment;

// To:
std::unique_ptr<juce::WebComboBoxRelay> materialRelay;
std::unique_ptr<juce::WebComboBoxParameterAttachment> materialAttachment;
```

In `PluginEditor.cpp`:
```cpp
// Change relay creation (line ~22):
materialRelay = std::make_unique<juce::WebComboBoxRelay>("material");

// Change attachment creation (line ~224):
materialAttachment = std::make_unique<juce::WebComboBoxParameterAttachment>(
    *apvts.getParameter("material"), *materialRelay, nullptr);
```

**Step 3: Simplify `getMaterialProperties()` to discrete lookup**

```cpp
BellVoice::MaterialProperties BellVoice::getMaterialProperties(float material)
{
    // Discrete material selection (Choice parameter returns normalized 0.0, 0.25, 0.5, 0.75, 1.0)
    // Map to discrete indices
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

**Step 4: Exaggerate material properties for audible difference**

Update material definitions in `BellVoice.h`:
```cpp
// EXAGGERATED for audible differentiation (per requirements)
static constexpr MaterialProperties MATERIAL_BRONZE     = {1.0f,  0.0f,   0.0f};    // Baseline warm
static constexpr MaterialProperties MATERIAL_BRASS      = {0.7f,  +0.20f, +0.08f};  // Bright, short, jazzy
static constexpr MaterialProperties MATERIAL_STEEL      = {2.0f,  +0.25f, -0.05f};  // Very bright, long sustain
static constexpr MaterialProperties MATERIAL_ALUMINUM   = {0.5f,  +0.30f, +0.12f};  // Very bright, short, thin
static constexpr MaterialProperties MATERIAL_CAST_IRON  = {1.5f,  -0.25f, +0.15f};  // Dark, long, gamelan-like
```

### Recommended Approach

1. Change parameter type from Float to Choice (5 options)
2. Update editor relays and attachments
3. Simplify getMaterialProperties() to discrete lookup
4. Exaggerate material property values
5. Update UI (HTML/JS) to render dropdown instead of slider

### Files to Modify

- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/PluginProcessor.h` (parameter pointer type)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/PluginProcessor.cpp` (parameter definition)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/PluginEditor.h` (relay/attachment types)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/PluginEditor.cpp` (relay/attachment creation)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/BellVoice.h` (material constants)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/BellVoice.cpp` (getMaterialProperties)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/ui/index.html` (dropdown UI element)

---

## 4. Inharmonicity Randomization

### Current Implementation

**Partial initialization** (`BellVoice.cpp:619-654`):
```cpp
void BellVoice::initializePartials(float fundamental, float velocity)
{
    auto materialProps = getMaterialProperties(currentMaterial);

    for (int p = 0; p < NUM_PARTIALS; ++p)
    {
        // Deterministic frequency calculation - same every time
        float effectiveInharmonicity = currentInharmonicity + materialProps.inharmonicity;
        partial.frequency = calculatePartialFrequency(p, fundamental, effectiveInharmonicity);

        // Deterministic amplitude - same every time
        float baseAmplitude = calculatePartialAmplitude(p, effectiveBrightness);
        float strikeGain = calculateStrikePositionGain(p, currentStrikePosition);
        partial.amplitude = baseAmplitude * strikeGain * malletGain * velocity;
    }
}
```

**Where called:** `startNote()` at line 112 (and for octave layers at lines 137, 163)

### Recommended Approach

**Add per-note randomization without allocations:**

The requirements specify Gaussian distribution, but for real-time safety, we can use a fast approximation.

**Option A: Box-Muller transform (allocation-free)**
```cpp
// In BellVoice.h, add helper:
float gaussianRandom() {
    // Box-Muller transform using cached second value
    static float spare;
    static bool hasSpare = false;

    if (hasSpare) {
        hasSpare = false;
        return spare;
    }

    float u, v, s;
    do {
        u = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        v = (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
        s = u * u + v * v;
    } while (s >= 1.0f || s == 0.0f);

    s = std::sqrt(-2.0f * std::log(s) / s);
    spare = v * s;
    hasSpare = true;
    return u * s;
}
```

**Option B: Simpler uniform approximation (faster)**
```cpp
// Sum of 3 uniform randoms approximates Gaussian (Central Limit Theorem)
float gaussianApprox() {
    float sum = 0.0f;
    for (int i = 0; i < 3; ++i)
        sum += (static_cast<float>(rand()) / RAND_MAX) * 2.0f - 1.0f;
    return sum / 1.73f;  // Scale to ~unit variance
}
```

**Implementation in `initializePartials()`:**
```cpp
void BellVoice::initializePartials(float fundamental, float velocity)
{
    auto materialProps = getMaterialProperties(currentMaterial);

    for (int p = 0; p < NUM_PARTIALS; ++p)
    {
        auto& partial = fundamentalVoices[0].partials[p];

        float effectiveInharmonicity = currentInharmonicity + materialProps.inharmonicity;
        partial.frequency = calculatePartialFrequency(p, fundamental, effectiveInharmonicity);

        // === NEW: Per-note pitch randomization ===
        // +/- 10 cents standard deviation (Gaussian)
        float pitchOffset = gaussianRandom() * 10.0f;
        partial.frequency *= std::pow(2.0f, pitchOffset / 1200.0f);

        // Recalculate phase increment with randomized frequency
        partial.phaseIncrement = partial.frequency / static_cast<float>(currentSampleRate);
        partial.phase = 0.0f;

        // Calculate base amplitude
        float effectiveBrightness = currentBrightness + materialProps.brightnessOffset;
        effectiveBrightness = juce::jlimit(0.0f, 1.0f, effectiveBrightness);

        float baseAmplitude = calculatePartialAmplitude(p, effectiveBrightness);
        float strikeGain = calculateStrikePositionGain(p, currentStrikePosition);
        float malletGain = 1.0f + currentMalletHardness * (p / static_cast<float>(NUM_PARTIALS));

        // === NEW: Per-note amplitude randomization ===
        // +/- 25% variation (Gaussian)
        float ampVariation = 1.0f + gaussianRandom() * 0.25f;
        ampVariation = juce::jlimit(0.5f, 1.5f, ampVariation);  // Clamp for stability

        partial.amplitude = baseAmplitude * strikeGain * malletGain * velocity * ampVariation;
        partial.targetAmplitude = partial.amplitude;

        // ... rest of initialization
    }
}
```

**Note:** Use same randomization approach for octave layer initializations (sub-octave, upper-octave).

### Files to Modify

- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/BellVoice.h` (add gaussianRandom helper)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/BellVoice.cpp` (initializePartials, and octave layer loops in startNote)

---

## 5. Attack Noise Overhaul

### Current Implementation

**Strike noise structure** (`BellVoice.h:102-117`):
```cpp
struct StrikeExciter
{
    float amplitude = 0.0f;
    float decayRate = 0.0f;
    bool active = false;

    // One-pole filter state for noise shaping
    float filterState = 0.0f;
    float filterCoeff = 0.0f;

    // Resonant bandpass state (for Ping)
    float bp1 = 0.0f;
    float bp2 = 0.0f;
    float resonance = 0.0f;
    float centerFreq = 0.0f;
};
```

**Noise generation** (`BellVoice.cpp:656-692`):
- Uses white noise through simple filters (one-pole LP/HP, or SVF bandpass)
- Click: high-pass filtered, 3-8ms
- Thud: low-pass filtered, 15-30ms
- Ping: bandpass resonant at fundamental, 8-20ms

### Resonant Filter Approach

**Replace noise-based excitation with impulse-driven resonant filter bank:**

The goal is to create a short impulse (1-2 samples) that excites parallel resonant filters tuned to the first few partials. This creates a more realistic "metallic contact" sound.

**JUCE StateVariableTPTFilter availability:** Confirmed available in `juce_dsp` module. Used extensively in other plugins (O-IntonationPad, O-Bass, etc.).

**Implementation approach:**

1. **Add resonant filter bank to StrikeExciter:**
```cpp
struct StrikeExciter
{
    // Existing fields...

    // NEW: Resonant filter bank for impulse response
    static constexpr int NUM_RESONATORS = 4;  // First 4 partials
    juce::dsp::StateVariableTPTFilter<float> resonators[NUM_RESONATORS];
    float resonatorGains[NUM_RESONATORS];

    // Impulse state
    int impulseSamplesRemaining = 0;
    float impulseAmplitude = 1.0f;
};
```

2. **Initialize resonators in startNote:**
```cpp
// In startNote(), after calculating fundamental:
for (int r = 0; r < StrikeExciter::NUM_RESONATORS; ++r)
{
    float partialFreq = calculatePartialFrequency(r, fundamental, currentInharmonicity);

    // Configure resonator
    strikeNoise.resonators[r].reset();
    strikeNoise.resonators[r].setType(juce::dsp::StateVariableTPTFilterType::bandpass);
    strikeNoise.resonators[r].setCutoffFrequency(partialFreq);

    // Q based on strike character and partial index
    float baseQ;
    switch (currentStrikeNoiseChar)
    {
        case 0: baseQ = 30.0f; break;  // Click: high Q, short ring
        case 1: baseQ = 8.0f;  break;  // Thud: low Q, soft
        case 2: baseQ = 20.0f; break;  // Ping: medium Q, metallic
    }
    float partialQ = baseQ * (1.0f - r * 0.15f);  // Lower Q for higher partials
    strikeNoise.resonators[r].setResonance(juce::jlimit(0.1f, 50.0f, partialQ));

    // Gain distribution based on mallet hardness
    // Hard mallet = more high partial content, soft = more fundamental
    float hardnessWeight = (currentMalletHardness * r) / StrikeExciter::NUM_RESONATORS;
    strikeNoise.resonatorGains[r] = 1.0f - (1.0f - currentMalletHardness) * (r * 0.25f);
}

// Set impulse duration (1-2 samples for sharp attack)
strikeNoise.impulseSamplesRemaining = 2;
strikeNoise.impulseAmplitude = currentVelocity * (0.5f + currentMalletHardness * 0.5f);
```

3. **Generate resonant strike noise:**
```cpp
float BellVoice::generateStrikeNoise()
{
    float output = 0.0f;

    // Generate impulse (1-2 samples)
    float impulse = 0.0f;
    if (strikeNoise.impulseSamplesRemaining > 0)
    {
        impulse = strikeNoise.impulseAmplitude;
        strikeNoise.impulseSamplesRemaining--;
    }

    // Process through resonant filter bank
    for (int r = 0; r < StrikeExciter::NUM_RESONATORS; ++r)
    {
        float resonatorOut = strikeNoise.resonators[r].processSample(0, impulse);
        output += resonatorOut * strikeNoise.resonatorGains[r];
    }

    // Apply overall decay envelope
    output *= strikeNoise.amplitude;

    return output;
}
```

### New Parameter

**Add attackLevel parameter** (`PluginProcessor.cpp`):
```cpp
// In createParameterLayout(), add after strikeNoiseChar:
layout.add(std::make_unique<juce::AudioParameterFloat>(
    juce::ParameterID { "attackLevel", 1 },
    "Attack",
    juce::NormalisableRange<float>(0.0f, 1.0f, 0.01f),
    0.5f,  // Default 50% (natural level)
    "%"
));
```

**Cache pointer in processor and pass to voice:**
```cpp
// In PluginProcessor.h:
std::atomic<float>* attackLevelParam = nullptr;

// In prepareToPlay():
attackLevelParam = parameters.getRawParameterValue("attackLevel");

// In processBlock(), add to voice updateParameters call:
float attackLevel = attackLevelParam->load();
voice->updateParameters(..., attackLevel, ...);
```

**Apply in BellVoice:**
```cpp
// In renderNextBlock(), where noise is added:
leftOutput += noiseSignal * 0.3f * currentAttackLevel * 2.0f;  // Scale so 50% = natural, 100% = doubled
rightOutput += noiseSignal * 0.3f * currentAttackLevel * 2.0f;
```

### Files to Modify

- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/BellVoice.h` (StrikeExciter struct, add currentAttackLevel)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/BellVoice.cpp` (startNote resonator init, generateStrikeNoise, renderNextBlock)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/PluginProcessor.h` (attackLevelParam)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/PluginProcessor.cpp` (parameter definition, prepareToPlay, processBlock)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/PluginEditor.h` (relay/attachment for attackLevel)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/PluginEditor.cpp` (relay/attachment creation)
- `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/ui/index.html` (UI slider for Attack)

---

## Risk Assessment

### High Risk Changes

1. **Bloom bug fix (Improvement #2)**
   - Risk: Delaying decay start could cause amplitude spikes if bloom overshoots
   - Mitigation: Clamp bloom multiplier to max 1.0, test with bloom=100%

2. **Material parameter type change (Improvement #3)**
   - Risk: Breaking preset compatibility - existing presets store material as 0.0-1.0 float
   - Mitigation: Implement migration in preset loading (map old float ranges to discrete indices)
   - Note: This is expected and documented as breaking change (MINOR version bump)

3. **Attack noise resonant filter bank (Improvement #5)**
   - Risk: StateVariableTPTFilter may introduce latency or CPU spikes
   - Mitigation: Use processSample (not block processing), limit to 4 resonators, measure CPU

### Medium Risk Changes

1. **Shimmer LFO rate change (Improvement #1)**
   - Risk: Very slow LFOs (0.1 Hz) could create noticeable pitch drift at high shimmer
   - Mitigation: Test with shimmer=100%, verify no unmusical effects

2. **Inharmonicity randomization (Improvement #4)**
   - Risk: Large pitch/amplitude variations could make bell unrecognizable
   - Mitigation: Clamp values (pitch +/-20 cents max, amplitude 50%-150%), test repeatedly

### Low Risk Changes

- Exaggerating material property values (constants only)
- Adding attackLevel parameter (additive, doesn't change existing behavior)

---

## Migration Notes

### Preset Compatibility

**Material parameter change will break existing presets:**
- Old presets store `material` as float 0.0-1.0
- New parameter is Choice with 5 discrete values (0, 1, 2, 3, 4)
- JUCE normalizes Choice values: index / (numItems - 1) = 0.0, 0.25, 0.5, 0.75, 1.0

**Migration strategy:**
```cpp
// In OuariconPresetManager::setStateFromXml or similar:
if (auto* materialValue = xml->getChildByName("material"))
{
    float oldValue = materialValue->getDoubleAttribute("value", 0.25);
    // Map old continuous value to nearest discrete index
    int discreteIndex;
    if (oldValue < 0.125f) discreteIndex = 0;       // Bronze
    else if (oldValue < 0.375f) discreteIndex = 1;  // Brass
    else if (oldValue < 0.625f) discreteIndex = 2;  // Steel
    else if (oldValue < 0.875f) discreteIndex = 3;  // Aluminum
    else discreteIndex = 4;                          // Cast Iron
    // Set as normalized Choice value
    float normalizedValue = discreteIndex / 4.0f;
    // Apply to parameter...
}
```

### New Parameter Default

- `attackLevel` defaults to 0.5 (50%), so existing behavior is preserved
- At 50%, transient level matches current implementation
- Users can reduce (0% = pure tone) or increase (100% = percussive)

---

## Domain Classification

Based on analysis: **DSP** domain (dsp-agent should execute)

All five improvements are DSP-focused:
1. Shimmer - LFO frequency modulation parameters
2. Bloom - Envelope timing and amplitude modulation
3. Material - Parameter type change + property values (DSP impact)
4. Inharmonicity randomization - Frequency/amplitude calculations
5. Attack noise - Filter topology change

**GUI work required:**
- Material dropdown (WebComboBoxRelay change)
- Attack slider (new WebSliderRelay)

The GUI work is minimal and follows established patterns already in the codebase. DSP-agent can handle both aspects.
