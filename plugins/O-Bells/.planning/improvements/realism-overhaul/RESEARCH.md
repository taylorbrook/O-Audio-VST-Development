# O-Bells Realism Overhaul - Research

**Milestone:** realism-overhaul
**Phase:** Research
**Date:** 2026-02-02

## 1. Decay Envelope Analysis

### Current Implementation

The decay envelope system in BellVoice.cpp (lines 263-268, 729-754) supports three modes controlled by the `decayShape` parameter:

```cpp
// From BellVoice.cpp renderNextBlock():
if (currentDecayShape == 2)  // Multi-stage (research-based)
    applyMultiStageDecay(partial, p);
else if (currentDecayShape == 1)  // Exponential
    partial.amplitude *= partial.decayRate;
else  // Linear (decayShape == 0)
    partial.amplitude -= partial.decayRate * 0.0001f;
```

**Multi-stage envelope** is implemented in `calculateMultiStageCoefficients()` (lines 667-726) and `applyMultiStageDecay()` (lines 729-754):
- Uses Chaigne-style frequency-dependent damping: `R_k = b_1 + b_3 * f_k^2`
- Three phases: Strike, Body, Hum
- Pre-calculates decay coefficients per partial in `startNote()`
- Brilliance parameter controls `b_3` coefficient (high-frequency damping)

**Parameter Location:**
- `decayShapeParam` in PluginProcessor.cpp (lines 151-156)
- Exposed as AudioParameterChoice: "Linear", "Exponential", "Multi-stage"

### Simplification Approach

To remove Linear and Exponential modes:

1. **PluginProcessor.cpp:**
   - Remove `decayShape` parameter definition (lines 151-156)
   - Remove `decayShapeParam` cached pointer (line 315)
   - Remove `decayShape` read in processBlock (line 357)
   - Update `updateParameters()` call to remove `decayShape` argument

2. **BellVoice.cpp:**
   - Remove `currentDecayShape` variable
   - Remove the conditional in renderNextBlock - always call `applyMultiStageDecay()`
   - Remove the `if (currentDecayShape == 2)` check in startNote() - always calculate multi-stage coefficients
   - Simplify linear decay fallback removal in three loop locations (lines 263-268, 306-312, 349-355)

3. **BellVoice.h:**
   - Remove `currentDecayShape` member variable (line 120)
   - Remove from `updateParameters()` signature

4. **PluginEditor.cpp/.h:**
   - Remove `decayShapeRelay` and `decayShapeAttachment`
   - Remove from WebBrowserComponent options chain

### Affected Files

| File | Changes Required |
|------|------------------|
| `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/PluginProcessor.cpp` | Remove parameter, cached pointer, processBlock read |
| `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/PluginProcessor.h` | Remove `decayShapeParam` pointer |
| `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/BellVoice.cpp` | Remove conditionals, always use multi-stage |
| `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/BellVoice.h` | Remove member variable, update signature |
| `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/PluginEditor.cpp` | Remove relay/attachment |
| `/Users/taylorbrook/Dev/VST-development/plugins/O-Bells/Source/PluginEditor.h` | Remove relay/attachment declarations |

### Preset Impact

Presets using `decayShape` values 0 (Linear) or 1 (Exponential) will need updating:
- **Gamelan Saron** - uses decayShape 0.0 (Linear)
- **Gamelan Bonang** - uses decayShape 0.0 (Linear)
- **Horror Stinger** - uses decayShape 0.0 (Linear)

These presets should be re-voiced using the multi-stage envelope with appropriate Strike/Body/Hum settings.

---

## 2. Bloom Implementation

### Technical Approach

Bloom (spectral swelling) requires adding an **attack phase** before the decay phase for each partial. The approach:

1. **Per-partial attack envelope:**
   - Each partial starts at a reduced amplitude (e.g., 30-50% of target)
   - Swells to full amplitude over a bloom time period
   - Then enters normal multi-stage decay

2. **Frequency-dependent bloom rates:**
   - Higher partials bloom faster than lower partials
   - Creates more organic "building" resonance

3. **Implementation structure:**
   ```cpp
   struct ModalPartial {
       // Existing fields...
       float bloomPhase = 0.0f;      // 0 = start, 1 = fully bloomed
       float bloomRate = 0.0f;       // Per-sample increment
       float initialAmplitude = 0.0f; // Starting amplitude (reduced)
       float peakAmplitude = 0.0f;    // Target amplitude (with bloom)
   };
   ```

### Code Pattern

```cpp
// In startNote(), after initializePartials():
void BellVoice::initializeBloom(float bloomAmount)
{
    if (bloomAmount <= 0.0f)
        return;  // No bloom, traditional instant-peak behavior

    // Bloom time: 50-200ms based on bloom parameter
    float bloomTimeMs = juce::jmap(bloomAmount, 50.0f, 200.0f);
    float bloomTimeSamples = bloomTimeMs * 0.001f * currentSampleRate;

    for (int p = 0; p < NUM_PARTIALS; ++p)
    {
        auto& partial = fundamentalVoices[0].partials[p];

        // Store full amplitude as peak
        partial.peakAmplitude = partial.amplitude;

        // Start at reduced amplitude (30-80% based on bloom)
        float startRatio = 1.0f - (bloomAmount * 0.7f);  // 0.3 at 100% bloom
        partial.initialAmplitude = partial.peakAmplitude * startRatio;
        partial.amplitude = partial.initialAmplitude;

        // Higher partials bloom faster (multiply by partial index factor)
        float partialSpeedFactor = 1.0f + (p / static_cast<float>(NUM_PARTIALS)) * 0.5f;
        partial.bloomRate = partialSpeedFactor / bloomTimeSamples;
        partial.bloomPhase = 0.0f;
    }
}

// In renderNextBlock(), before decay application:
void BellVoice::applyBloom(ModalPartial& partial)
{
    if (partial.bloomPhase < 1.0f)
    {
        partial.bloomPhase += partial.bloomRate;
        if (partial.bloomPhase > 1.0f)
            partial.bloomPhase = 1.0f;

        // Smooth interpolation using cosine curve
        float t = 0.5f * (1.0f - std::cos(partial.bloomPhase * juce::MathConstants<float>::pi));
        partial.amplitude = partial.initialAmplitude +
                           (partial.peakAmplitude - partial.initialAmplitude) * t;
    }
}
```

### JUCE APIs Used

- `juce::jmap()` - Range mapping (already used extensively)
- `std::cos()` - Smooth interpolation curve
- Standard per-sample processing pattern

### Interaction with Multi-Stage Envelope

Bloom adds an attack phase that runs **before** the Strike phase begins decay:
- If bloom > 0: Attack phase builds amplitude, then Strike phase begins
- Strike phase still handles the bright transient decay
- Body and Hum phases unchanged

---

## 3. Shimmer Implementation

### Technical Approach

Shimmer (frequency drift/beating) requires **LFO modulation of partial frequencies** that increases during decay.

1. **Per-partial LFO:**
   - Each partial has its own LFO with unique rate
   - LFO modulates the partial's frequency (phase increment)
   - Different rates create complex beating patterns

2. **Decay-following modulation depth:**
   - Shimmer amount increases as note decays
   - Creates characteristic "alive" quality of struck metal
   - Envelope follower tracks amplitude to scale shimmer

### Code Pattern

```cpp
// New member variables in ModalPartial:
struct ModalPartial {
    // Existing fields...
    float shimmerLFOPhase = 0.0f;     // LFO phase (0-1)
    float shimmerLFORate = 0.0f;      // Per-sample phase increment
    float shimmerDepth = 0.0f;        // Max frequency deviation (cents)
};

// In startNote():
void BellVoice::initializeShimmer(float shimmerAmount)
{
    if (shimmerAmount <= 0.0f)
        return;

    // Base shimmer: 0.1 to 5 cents frequency deviation
    float maxDeviation = juce::jmap(shimmerAmount, 0.1f, 5.0f);

    for (int p = 0; p < NUM_PARTIALS; ++p)
    {
        auto& partial = fundamentalVoices[0].partials[p];

        // Each partial gets unique LFO rate: 0.3Hz to 3Hz, spread by partial index
        // Prime number ratios create more interesting beating
        static const float lfoRatios[] = {1.0f, 1.31f, 1.73f, 2.11f, 2.57f, 3.01f, 3.47f, 4.03f};
        float baseLFORate = 0.5f;  // Hz
        float lfoHz = baseLFORate * lfoRatios[p];
        partial.shimmerLFORate = lfoHz / currentSampleRate;

        // Random starting phase for each partial
        partial.shimmerLFOPhase = static_cast<float>(rand()) / RAND_MAX;

        // Higher partials get more shimmer
        partial.shimmerDepth = maxDeviation * (1.0f + p * 0.1f);
    }
}

// In renderNextBlock(), modify phase increment:
float BellVoice::applyShimmer(ModalPartial& partial, float decayProgress)
{
    // Advance LFO
    partial.shimmerLFOPhase += partial.shimmerLFORate;
    if (partial.shimmerLFOPhase >= 1.0f)
        partial.shimmerLFOPhase -= 1.0f;

    // Triangle LFO for smooth modulation
    float lfoValue = 2.0f * std::abs(partial.shimmerLFOPhase - 0.5f) - 0.5f;

    // Shimmer increases during decay (envelope follower)
    // decayProgress: 0 = note start, 1 = fully decayed
    float envelopeScale = 0.3f + 0.7f * decayProgress;

    // Convert cents to frequency ratio
    float deviationCents = lfoValue * partial.shimmerDepth * envelopeScale * currentShimmer;
    float frequencyRatio = std::pow(2.0f, deviationCents / 1200.0f);

    return partial.phaseIncrement * frequencyRatio;
}
```

### Decay Progress Calculation

Track decay progress per-partial or globally:
```cpp
// Global approach: time since note-on relative to total expected decay
float decayProgress = juce::jmin(1.0f, timeSeconds / (bodyEndTime * 2.0f));
```

---

## 4. Mallet Enhancement

### Current Mallet Behavior

From BellVoice.cpp:

```cpp
// Line 152: Mallet hardness affects transient amplitude
strikeNoise.amplitude = currentVelocity * (0.5f + currentMalletHardness * 0.5f);

// Lines 162-189: Decay time varies with hardness
float decayTime = juce::jmap(currentMalletHardness, 0.008f, 0.003f);  // Click: 8ms to 3ms
float decayTime = juce::jmap(currentMalletHardness, 0.030f, 0.015f); // Thud: 30ms to 15ms
float decayTime = juce::jmap(currentMalletHardness, 0.020f, 0.008f); // Ping: 20ms to 8ms

// Line 570: Higher partials boosted with harder mallets
float malletGain = 1.0f + currentMalletHardness * (p / static_cast<float>(NUM_PARTIALS));
```

### Enhancement Approach

Add temporal spreading (attack time) based on inverse of mallet hardness:

```cpp
// New member variable
float attackRampSamples = 0.0f;
float attackRampPosition = 0.0f;

// In startNote():
void BellVoice::initializeMalletAttack(float malletHardness)
{
    // Soft mallet = longer attack (0-50ms), hard mallet = instant
    float maxAttackMs = 50.0f;
    float attackMs = maxAttackMs * (1.0f - malletHardness);
    attackRampSamples = attackMs * 0.001f * currentSampleRate;
    attackRampPosition = 0.0f;

    // Also affect strike noise envelope duration
    if (malletHardness < 0.3f)
    {
        // Very soft: extend noise envelope, lower amplitude
        float softFactor = (0.3f - malletHardness) / 0.3f;
        strikeNoise.decayRate = std::pow(strikeNoise.decayRate, 1.0f - softFactor * 0.3f);
    }
}

// In renderNextBlock(), apply attack ramp:
float attackMultiplier = 1.0f;
if (attackRampSamples > 0.0f && attackRampPosition < attackRampSamples)
{
    attackMultiplier = attackRampPosition / attackRampSamples;
    // Smooth curve for natural feel
    attackMultiplier = 0.5f * (1.0f - std::cos(attackMultiplier * juce::MathConstants<float>::pi));
    ++attackRampPosition;
}

// Apply to output
leftOutput *= attackMultiplier;
rightOutput *= attackMultiplier;
```

---

## 5. Material Research

### Bronze (Bell Metal / Tin Bronze)

- **Composition:** 78-80% Copper, 20-22% Tin (traditional bell metal)
- **Decay coefficient:** Low internal friction; sustain 51% longer than B8 bronze
- **Spectral emphasis:** Complex overtones, warm fundamental, prominent partials
- **Inharmonicity:** Moderate - characteristic "minor third" partial (~2.4x fundamental)
- **Key acoustic property:** Delta-phase (Cu31Sn8) intermetallic creates unique bell tone
- **Recommended decay multiplier:** 1.0 (baseline)

**Sources:**
- [Bell metal - Wikipedia](https://en.wikipedia.org/wiki/Bell_metal)
- [Structure and Damping Capacity of Br022 Bell Bronze](https://www.researchgate.net/publication/227274767_Structure_and_Damping_Capacity_of_Br022_Bell_Bronze)
- [Effect of High-tin Bronze Composition on Gamelan Materials](https://www.researchgate.net/publication/353840150_Effect_of_High-tin_Bronze_Composition_on_Physical_Mechanical_and_Acoustic_Properties_of_Gamelan_Materials)

### Brass

- **Composition:** ~67% Copper, ~33% Zinc
- **Decay coefficient:** Small damping capacity (similar to steel)
- **Spectral emphasis:** Brighter than bronze, more prominent upper partials
- **Inharmonicity:** Lower than bronze - more harmonic spectrum
- **Key acoustic property:** Higher internal friction than bronze, shorter sustain
- **Recommended decay multiplier:** 0.7-0.8

**Sources:**
- [Brass - Wikipedia](https://en.wikipedia.org/wiki/Brass)
- [Brass vs Bronze - Diffen](https://www.diffen.com/difference/Brass_vs_Bronze)

### Steel (Carbon Steel)

- **Composition:** Iron with 0.04-2% Carbon
- **Decay coefficient:** Very low internal damping (allows sustained vibration)
- **Spectral emphasis:** Bright, clear overtones; precise pitch
- **Inharmonicity:** Low - relatively harmonic
- **Key acoustic property:** High tensile strength allows high string tension (pianos)
- **Recommended decay multiplier:** 1.4-1.5 (longer sustain)

**Sources:**
- [Mechanical and Acoustic Properties of Alloys Used for Musical Instruments](https://pmc.ncbi.nlm.nih.gov/articles/PMC9369773/)
- [Elastic interactions in the Caribbean steel drum](https://www.sciencedirect.com/science/article/abs/pii/S1044580301001875)

### Aluminum

- **Composition:** Pure aluminum or aluminum alloys
- **Decay coefficient:** Loss factor range 0.0001 to 0.02 (varies by alloy)
- **Spectral emphasis:** Very bright, prominent high frequencies
- **Inharmonicity:** Low - near-harmonic
- **Key acoustic property:** Low acoustic impedance, fast sound velocity, quick decay
- **Recommended decay multiplier:** 0.5-0.6 (shorter, brighter)

**Sources:**
- [Mechanical and Acoustic Properties of Alloys Used for Musical Instruments](https://pmc.ncbi.nlm.nih.gov/articles/PMC9369773/)
- [NASA Technical Memorandum - Elastic Moduli and Damping](https://ntrs.nasa.gov/api/citations/19960015944/downloads/19960015944.pdf)

### Cast Iron

- **Composition:** Iron with 2-4% Carbon (higher than steel)
- **Decay coefficient:** Acoustically inert - high internal damping
- **Spectral emphasis:** Dark, muted; suppresses upper partials
- **Inharmonicity:** High - less harmonic spectrum
- **Key acoustic property:** High damping makes it ideal for piano frames (absorbs vibration)
- **Recommended decay multiplier:** 0.3-0.4 (very short, damped)

**Sources:**
- [Cast-Iron Frame - The Piano Deconstructed](https://www.piano.christophersmit.com/frame.html)
- [Piano - Wikipedia](https://en.wikipedia.org/wiki/Piano)

### Recommended Material Implementation

```cpp
// Updated material constants (research-based)
struct MaterialProperties {
    float decayMultiplier;    // Relative to bronze baseline
    float brightnessOffset;   // High-frequency emphasis (-1 to +1)
    float inharmonicity;      // Partial spread factor
};

static constexpr MaterialProperties MATERIALS[5] = {
    // Bronze (baseline)
    { 1.0f, 0.0f, 0.5f },
    // Brass (brighter, shorter)
    { 0.75f, 0.15f, 0.35f },
    // Steel (sustained, bright)
    { 1.5f, 0.1f, 0.25f },
    // Aluminum (fast, very bright)
    { 0.55f, 0.25f, 0.2f },
    // Cast Iron (damped, dark)
    { 0.35f, -0.3f, 0.65f }
};

// Material parameter maps 0-1 to 5 discrete materials
// 0.0 = Bronze, 0.25 = Brass, 0.5 = Steel, 0.75 = Aluminum, 1.0 = Cast Iron
```

---

## 6. Stereo Enhancement

### Haas Effect Implementation

The Haas effect creates perceived width through timing differences (1-35ms delay).

**JUCE Approach:** Use a simple delay line (ring buffer) for one channel:

```cpp
// Member variables
std::vector<float> haasDelayBuffer;
int haasDelayWritePos = 0;
int haasDelaySamples = 0;

// In prepare():
void prepareHaasDelay(double sampleRate, int maxBlockSize)
{
    // Max 30ms delay at 192kHz = 5760 samples
    int maxDelaySamples = static_cast<int>(0.030 * sampleRate) + 1;
    haasDelayBuffer.resize(maxDelaySamples, 0.0f);
    haasDelayWritePos = 0;
}

// Set delay based on stereoSpread parameter (0-30ms)
void setHaasDelay(float stereoSpread, double sampleRate)
{
    float delayMs = stereoSpread * 30.0f;  // 0-30ms
    haasDelaySamples = static_cast<int>(delayMs * 0.001f * sampleRate);
}

// In processBlock(), apply to right channel:
float processHaasDelay(float input)
{
    // Write input to buffer
    haasDelayBuffer[haasDelayWritePos] = input;

    // Read from delayed position
    int readPos = haasDelayWritePos - haasDelaySamples;
    if (readPos < 0)
        readPos += haasDelayBuffer.size();

    float output = haasDelayBuffer[readPos];

    // Advance write position
    haasDelayWritePos = (haasDelayWritePos + 1) % haasDelayBuffer.size();

    return output;
}
```

### Per-Partial Panning

Current implementation pans unison voices. Enhanced approach pans individual partials:

```cpp
// Per-partial pan position based on frequency
float getPartialPan(int partialIndex, float stereoSpread)
{
    // Low partials center, high partials spread wide
    float spreadFactor = static_cast<float>(partialIndex) / NUM_PARTIALS;

    // Alternate left/right for odd/even partials
    float direction = (partialIndex % 2 == 0) ? 1.0f : -1.0f;

    return direction * spreadFactor * stereoSpread;
}
```

### Stereo Movement Implementation

Add slow LFO modulation to partial pan positions:

```cpp
// Per-partial stereo movement
struct PartialStereoState {
    float panLFOPhase = 0.0f;
    float panLFORate = 0.0f;
    float basePan = 0.0f;
};

// In startNote():
void initializeStereoMovement(float stereoSpread)
{
    for (int p = 0; p < NUM_PARTIALS; ++p)
    {
        auto& state = partialStereo[p];

        // Slow LFO: 0.1 to 0.5 Hz, different per partial
        state.panLFORate = (0.1f + p * 0.05f) / currentSampleRate;
        state.panLFOPhase = static_cast<float>(rand()) / RAND_MAX;
        state.basePan = getPartialPan(p, stereoSpread);
    }
}

// In renderNextBlock():
float getModulatedPan(int partialIndex, float stereoSpread)
{
    auto& state = partialStereo[partialIndex];

    state.panLFOPhase += state.panLFORate;
    if (state.panLFOPhase >= 1.0f)
        state.panLFOPhase -= 1.0f;

    float lfoValue = std::sin(state.panLFOPhase * juce::MathConstants<float>::twoPi);
    float movementAmount = stereoSpread * 0.3f;  // 30% of spread as movement range

    return juce::jlimit(-1.0f, 1.0f, state.basePan + lfoValue * movementAmount);
}
```

---

## 7. Placeholder Scan Results

| File | Line | Issue | Priority |
|------|------|-------|----------|
| BellVoice.cpp | 268 | Magic number `0.0001f` for linear decay rate | M |
| BellVoice.cpp | 312, 355 | Duplicate magic number `0.0001f` | M |
| BellVoice.cpp | 377-378 | Magic number `0.3f` for noise mix level | L |
| BellVoice.cpp | 406 | Magic number `0.4f` for partial normalization | M |
| BellVoice.cpp | 587 | Using `rand()` - consider JUCE Random | L |
| BellVoice.cpp | 673 | Magic number `0.5f` for b1 base damping | M |
| BellVoice.cpp | 677 | Magic number `2e-8f` for b3 scaling | M |
| BellVoice.cpp | 702-706 | Magic multipliers 2.0, 1.0, 0.3 for strike decay | M |
| BellVoice.cpp | 722-723 | Magic numbers 0.8, 2.0 for damping factor | M |
| BellVoice.h | 48-50 | Partial ratios hardcoded - consider making configurable | L |
| BellVoice.h | 53 | DECAY_MULTIPLIERS hardcoded | M |
| BellVoice.h | 56-59 | Material decay multipliers placeholder values | H |
| PluginProcessor.h | 21-24 | Reverb spec hardcoded | L |

**Summary:**
- No TODO/FIXME comments found (clean codebase)
- Several magic numbers that should be named constants or derived from parameters
- Material decay multipliers are placeholder values that need research-based updates (HIGH priority)
- Some minor style issues (rand() vs JUCE Random)

---

## 8. Complexity Assessment

- **Estimated Scope:** Large
- **Domain:** DSP (Digital Signal Processing)
- **Estimated Implementation Time:** 3-5 days

### Key Risks

1. **Performance Impact:**
   - Adding per-partial LFOs (shimmer) adds computation
   - Per-partial stereo movement adds more state
   - Need to profile and optimize if necessary

2. **Parameter Interaction:**
   - Bloom and multi-stage Strike phase may conflict
   - Shimmer amount vs. decay timing needs careful tuning

3. **Preset Migration:**
   - Removing decayShape breaks 3 factory presets
   - Material rework may shift existing preset sounds

4. **Audio Artifacts:**
   - Shimmer LFOs could cause phase cancellation if rates align
   - Haas delay must be smoothly interpolated to avoid clicks

### Dependencies

- No external dependencies
- All features implementable with existing JUCE classes
- juce_dsp module already included (for potential DelayLine class)

---

## 9. Recommended Approach

### Implementation Order

1. **Phase 1: Decay Simplification** (Low risk, unblocks other work)
   - Remove decayShape parameter and Linear/Exponential modes
   - Fix affected factory presets
   - Test multi-stage is working correctly

2. **Phase 2: Material Research Integration** (Foundation for sound design)
   - Update material constants with research-based values
   - Implement 5-material system (Bronze/Brass/Steel/Aluminum/Cast Iron)
   - Adjust existing presets for new material mapping

3. **Phase 3: Bloom Parameter** (Additive feature)
   - Add bloom parameter to APVTS
   - Implement per-partial attack envelope
   - Add UI control

4. **Phase 4: Shimmer Parameter** (Additive feature)
   - Add shimmer parameter to APVTS
   - Implement per-partial LFO modulation
   - Add decay-following envelope
   - Add UI control

5. **Phase 5: Mallet Enhancement** (Behavior change)
   - Add attack time scaling based on mallet hardness
   - Adjust strike noise envelope for soft mallets

6. **Phase 6: Stereo Enhancement** (Additive feature)
   - Implement per-partial panning
   - Add Haas effect delay
   - Add stereo movement LFOs

7. **Phase 7: Preset Rework** (Final polish)
   - Update all 25 factory presets
   - Add Bloom/Shimmer to appropriate presets
   - Verify all presets with new material system

### Code Quality Notes

- Extract magic numbers to named constants
- Document the frequency-dependent damping formula
- Add unit tests for envelope calculations if framework supports
- Profile performance after shimmer/stereo additions

---

*Research Phase Complete - Ready for Plan Phase*
