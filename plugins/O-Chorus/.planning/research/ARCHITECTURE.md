---
status: APPROVED
generated_by: research-planning-agent
generated_at: 2026-02-07
plugin_name: O-Chorus
juce_version: 8.0.4
complexity_score: 2.8
implementation_strategy: single-pass
---

# O-Chorus - DSP Architecture Specification

**Plugin Type:** Multi-voice chorus effect
**Signal Path:** Stereo in → Stereo out
**Processing Domain:** Time-domain delay line modulation
**Target Character:** Lush analog BBD-style chorus

---

## 1. Executive Summary

O-Chorus is a multi-voice chorus plugin (1-8 voices) with BBD-inspired analog warmth. The architecture centers on modulated delay lines with per-voice LFO phase offsets, analog saturation, tone control, and stereo imaging. Implementation uses JUCE's `dsp::DelayLine` with cubic interpolation for smooth modulation, combined with simple waveshaping saturation and one-pole filtering.

**Key Technical Decisions:**
- Lagrange 3rd-order interpolation for delay lines (balance of quality and CPU)
- Per-voice phase offset: `(2π * voiceIndex) / numVoices`
- Randomized depth variation per voice (5-15% deviation from base depth)
- Soft-clipping saturation with tanh for analog warmth
- One-pole lowpass in wet signal path for BBD high-frequency rolloff
- Stereo panning via equal-power pan law across voice array

---

## 2. Core Components

### 2.1 Multi-Voice Delay Line Engine

**Implementation:** Array of `juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd>`

**Specifications:**
- **Voice count:** 1-8 (user-selectable parameter)
- **Base delay range:** 5-15ms (classic chorus territory)
- **Interpolation:** Lagrange 3rd-order (good quality, moderate CPU)
- **Max delay buffer:** 50ms (allows modulation headroom)

**JUCE Module:** `juce::dsp::DelayLine` from `juce_dsp`

**Per-voice state:**
```cpp
struct ChorusVoice {
    juce::dsp::DelayLine<float, juce::dsp::DelayLineInterpolationTypes::Lagrange3rd> delayLine;
    float lfoPhaseOffset;      // Fixed per-voice: (2π * index) / numVoices
    float depthVariation;      // Randomized once on init: 0.85 to 1.15
    float panPosition;         // Calculated: index / (numVoices - 1) for stereo spread
};

std::array<ChorusVoice, 8> voices;  // Fixed array, only first N active
```

**Rationale for Lagrange3rd:**
- Better frequency response than linear interpolation
- Lower CPU than Thiran allpass (not needed for chorus modulation rates)
- Avoids zipper noise during modulation
- Sufficient for LFO rates (0.05 - 5 Hz)

**Alternative considered:** Thiran allpass interpolation (flat magnitude response) was evaluated but deemed unnecessary for chorus since modulation is slow (< 5 Hz) and Lagrange3rd provides adequate quality at lower CPU cost.

---

### 2.2 LFO Modulation System

**Implementation:** Sine wave oscillator with per-voice phase offset

**Specifications:**
- **Waveform:** Sine (with optional slight triangle blend for smoothness)
- **Rate range:** 0.05 - 5.0 Hz (user parameter)
- **Depth range:** 0 - 100% (user parameter, maps to delay time modulation)
- **Phase distribution:** `phaseOffset[i] = (2π * i) / numVoices`
- **Depth randomization:** Each voice gets random multiplier 0.85-1.15 (fixed on init)

**Per-sample calculation:**
```cpp
float lfoPhase = (lfoRate * 2π / sampleRate) * sampleCounter + voice.lfoPhaseOffset;
float lfoValue = sin(lfoPhase);  // Range: -1.0 to +1.0
float modAmount = depth * voice.depthVariation;  // Apply per-voice randomization
float modulatedDelay = baseDelayMs + (lfoValue * modAmount * delayRangeMs);
```

**Base delay calculation:**
- **Base delay:** 10ms (center of 5-15ms range)
- **Modulation range:** ±depth% of base delay
- **Example:** At 100% depth, delay varies from 5ms to 15ms

**JUCE Integration:** No built-in JUCE oscillator needed; simple phase accumulator with `std::sin()`.

**Professional reference:** Strymon Ola tri-chorus uses 3 independent delay lines with phase distribution for "cyclonic swirl" without seasick modulation. Juno-60 uses quadrature LFO (90° offset) between two BBD chips.

---

### 2.3 Analog Saturation

**Implementation:** Soft-clipping waveshaping with tanh

**Specifications:**
- **Function:** `std::tanh(x * drive) / std::tanh(drive)` (normalized)
- **Drive range:** 0.0 - 0.5 (subtle saturation for warmth, not distortion)
- **Application point:** Wet signal AFTER delay, BEFORE tone control
- **Asymmetry:** Optional slight asymmetry for BBD character (higher drive on positive half)

**Algorithm:**
```cpp
float saturate(float wetSample, float driveAmount) {
    if (driveAmount < 0.01f) return wetSample;  // Bypass if minimal

    // Subtle asymmetry: positive half driven slightly harder (BBD emulation)
    float driven = wetSample * (1.0f + driveAmount);
    if (wetSample >= 0.0f) {
        return std::tanh(driven) / std::tanh(1.0f + driveAmount);
    } else {
        return std::tanh(driven * 0.9f) / std::tanh((1.0f + driveAmount) * 0.9f);
    }
}
```

**JUCE Module:** None required (standard library `<cmath>`)

**Professional reference:** D16 Syntorus 2 activates "Analog BBD Emulation" for warmth. Boss CE-1/CE-2 used MN3207 BBD chips with inherent soft saturation.

**Rationale:** Tanh provides smooth, musical saturation without harsh clipping. Asymmetry mimics BBD transfer characteristics. No oversampling needed at low drive levels (chorus is gentle effect).

**Alternative considered:** Jiles-Atherton hysteresis model (from circuit-modeling-fundamentals.md) for authentic BBD behavior. Rejected due to complexity; tanh provides 90% of character at 10% of CPU cost.

---

### 2.4 Tone Control (BBD High-Frequency Rolloff)

**Implementation:** One-pole lowpass filter on wet signal

**Specifications:**
- **Filter type:** First-order IIR lowpass
- **Cutoff range:** 2kHz - 20kHz (mapped from Tone parameter -100% to +100%)
- **Default:** ~8kHz (neutral position, typical BBD bandwidth)
- **Topology:** Direct Form I biquad (simple, efficient)

**JUCE Module:** `juce::dsp::IIR::Filter<float>` from `juce_dsp`

**Implementation:**
```cpp
juce::dsp::IIR::Filter<float> toneFilter;

void prepareToPlay(double sampleRate, int samplesPerBlock) {
    // ... other preparation ...
    float cutoffFreq = mapToneParamToCutoff(toneParam);  // 2kHz - 20kHz
    *toneFilter.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(
        sampleRate, cutoffFreq);
}

float processedWet = toneFilter.processSample(saturatedWet);
```

**Parameter mapping:**
- **Tone = -100%:** 2kHz cutoff (dark, lo-fi BBD)
- **Tone = 0%:** 8kHz cutoff (neutral, classic BBD)
- **Tone = +100%:** 20kHz cutoff (bright, modern)

**Professional reference:** ValhallaDelay's BBD mode includes "limited high frequency response" as a key BBD characteristic. Vintage BBD chips (MN3005, MN3207) had natural rolloff around 8-10kHz.

---

### 2.5 Stereo Imaging

**Implementation:** Equal-power panning across voice array

**Specifications:**
- **Width parameter:** 0% - 100% (user control)
- **Width = 0%:** All voices centered (mono imaging)
- **Width = 100%:** Voices spread evenly across stereo field
- **Pan law:** Constant power (sin/cos)

**Per-voice pan calculation:**
```cpp
// On voice initialization (when numVoices changes)
for (int i = 0; i < numVoices; ++i) {
    if (numVoices == 1) {
        voices[i].panPosition = 0.5f;  // Center
    } else {
        voices[i].panPosition = (float)i / (numVoices - 1);  // 0.0 (L) to 1.0 (R)
    }
}

// During processing
float panAngle = voices[i].panPosition * (π / 2);  // 0 to π/2
float leftGain = std::cos(panAngle) * (1.0f - width + width * voices[i].panPosition);
float rightGain = std::sin(panAngle) * (1.0f - width + width * voices[i].panPosition);
```

**Width parameter effect:**
- Scales pan spread from mono (width=0%) to full stereo (width=100%)
- Preserves center-weighted image at lower width values
- Maintains perceived loudness via equal-power law

**Professional reference:** Strymon Ola tri-chorus uses 3 delay lines per channel with independent stereo positioning. Dimension D chorus uses 4 BBD chips with stereo panning for wide image.

---

## 3. Processing Chain

### 3.1 Signal Flow Diagram

```
Input L/R (stereo)
    │
    ├─────────────────────────────────[Dry Path]──────────────────────────────┐
    │                                                                           │
    └──► [Mono Sum for Chorus Input*]                                         │
            │                                                                   │
            ├──► Voice 1: LFO → Delay → Saturation → Tone ──┬──► [Pan L/R] ──┤
            ├──► Voice 2: LFO → Delay → Saturation → Tone ──┤                 │
            ├──► Voice 3: LFO → Delay → Saturation → Tone ──┤                 │
            └──► Voice N: LFO → Delay → Saturation → Tone ──┘                 │
                                                                                │
                                     [Sum All Voices] ◄────────────────────────┘
                                             │
                                     [Mix: Dry + Wet]
                                             │
                                        Output L/R
```

**\*Mono sum:** For stereo input, chorus processes mono sum (L+R)/2 to avoid phase issues. All voices receive same input signal, stereo imaging comes from pan distribution only.

---

### 3.2 Processing Order Per Voice

1. **Input:** Mono sum of stereo input
2. **Delay:** Modulated delay line read (5-15ms base, modulated by LFO)
3. **Saturation:** Soft-clipping warmth (tanh)
4. **Tone:** One-pole lowpass filtering
5. **Pan:** Equal-power pan to stereo field position
6. **Sum:** Accumulate into wet buffer

---

### 3.3 Mix Stage

**Implementation:** Linear crossfade between dry and wet signals

```cpp
// After all voices processed and summed
float wetL = /* sum of all voice outputs to left channel */;
float wetR = /* sum of all voice outputs to right channel */;

float outputL = dryL * (1.0f - mix) + wetL * mix;
float outputR = dryR * (1.0f - mix) + wetR * mix;
```

**Mix parameter:**
- **0%:** Full dry (bypass)
- **50%:** Equal dry/wet (classic chorus)
- **100%:** Full wet (vibrato mode)

---

## 4. System Architecture

### 4.1 Thread Safety

**Audio thread (real-time):**
- `processBlock()` - All DSP processing
- Parameter reads via atomic loads from APVTS
- NO allocations, NO locks, NO blocking operations

**Message thread (non-real-time):**
- Parameter updates from UI
- Voice count changes (requires delay line reinitialization)
- Random depth variations regenerated when voice count changes

**Synchronization strategy:**
- Use `juce::AudioProcessorValueTreeState` for thread-safe parameter communication
- Voice count changes trigger `prepareToPlay()` re-call via host
- Smoothed parameter values prevent audio glitches

---

### 4.2 Sample Rate Independence

**All time-based calculations scaled by sample rate:**

```cpp
void prepareToPlay(double sampleRate, int samplesPerBlock) {
    this->sampleRate = sampleRate;

    // Set delay line max size
    int maxDelaySamples = (int)(sampleRate * 0.05);  // 50ms max
    for (auto& voice : voices) {
        voice.delayLine.setMaximumDelayInSamples(maxDelaySamples);
        voice.delayLine.prepare(spec);
    }

    // Update tone filter for current sample rate
    updateToneFilter(sampleRate);
}
```

**LFO phase increment per sample:**
```cpp
float phaseIncrement = (lfoRate * 2.0f * juce::MathConstants<float>::pi) / sampleRate;
```

---

### 4.3 Latency

**Total plugin latency:** ~0 samples (delay lines don't contribute to latency, only add delayed copies)

**No lookahead required** - chorus is a modulation effect, not dynamics processing.

---

## 5. Parameter Mapping

### 5.1 Parameter Table

| Parameter | ID | Type | Range | Default | Unit | Skew | Smoothing |
|-----------|-----|------|-------|---------|------|------|-----------|
| Rate | RATE | Float | 0.05 - 5.0 | 1.0 | Hz | Log | 50ms |
| Depth | DEPTH | Float | 0.0 - 1.0 | 0.5 | Normalized | Linear | 50ms |
| Voices | VOICES | Int | 1 - 8 | 4 | Count | Linear | None* |
| Width | WIDTH | Float | 0.0 - 1.0 | 0.7 | Normalized | Linear | 50ms |
| Tone | TONE | Float | -1.0 - 1.0 | 0.0 | Normalized | Linear | 100ms |
| Mix | MIX | Float | 0.0 - 1.0 | 0.5 | Normalized | Linear | 50ms |

**\*Voices parameter:** No smoothing (discrete steps). Voice count change triggers reinitialization in `prepareToPlay()`.

---

### 5.2 Parameter Range Rationale

**Rate (0.05 - 5.0 Hz):**
- **0.05 Hz:** Ultra-slow sweep (20 second cycle) for subtle movement
- **1.0 Hz:** Classic chorus speed (Juno-60 territory)
- **5.0 Hz:** Fast modulation (vibrato-like, almost tremolo)
- **Logarithmic skew:** More control in musical range (0.5 - 2 Hz)

**Depth (0 - 100%):**
- **0%:** No modulation (dry signal only)
- **50%:** Moderate chorus thickness
- **100%:** Maximum modulation (±5ms from 10ms base = 5-15ms range)

**Voices (1 - 8):**
- **1 voice:** Simple vibrato/chorus
- **2 voices:** Classic stereo chorus (Juno-style)
- **3 voices:** Tri-chorus (Strymon Ola style)
- **4-8 voices:** Lush ensemble sound (thicker, denser)

**Width (0 - 100%):**
- **0%:** Mono (all voices centered)
- **50%:** Moderate spread
- **100%:** Full stereo width

**Tone (-100% to +100%):**
- **-100%:** Dark, lo-fi BBD (2kHz rolloff)
- **0%:** Neutral BBD character (8kHz rolloff)
- **+100%:** Bright, modern (20kHz rolloff)

**Mix (0 - 100%):**
- **0%:** Dry (bypass)
- **50%:** Classic chorus mix
- **100%:** Vibrato mode (wet only)

---

## 6. Algorithm Details

### 6.1 Delay Time Modulation

**Base delay:** 10ms (center of 5-15ms range)

**Modulated delay calculation per voice:**

```cpp
// Per-voice LFO phase
float lfoPhase = globalLFOPhase + voice.lfoPhaseOffset;
float lfoValue = std::sin(lfoPhase);  // -1.0 to +1.0

// Apply depth with per-voice randomization
float effectiveDepth = depthParam * voice.depthVariation;  // depthVariation: 0.85-1.15
float modulationAmount = lfoValue * effectiveDepth;  // -depth to +depth

// Calculate delay time in milliseconds
float baseDelayMs = 10.0f;  // Center of range
float delayRangeMs = 5.0f;  // ±5ms modulation at 100% depth
float modulatedDelayMs = baseDelayMs + (modulationAmount * delayRangeMs);

// Convert to samples and apply
float modulatedDelaySamples = (modulatedDelayMs / 1000.0f) * sampleRate;
float delayedSample = voice.delayLine.popSample(channel, modulatedDelaySamples);
```

**Depth randomization generation (once per voice):**
```cpp
// Called when voice count changes
std::default_random_engine generator(voiceIndex);  // Seeded for repeatability
std::uniform_real_distribution<float> distribution(0.85f, 1.15f);
voice.depthVariation = distribution(generator);
```

---

### 6.2 Phase Distribution

**Fixed phase offset per voice:**

```cpp
for (int i = 0; i < numVoices; ++i) {
    voices[i].lfoPhaseOffset = (2.0f * juce::MathConstants<float>::pi * i) / numVoices;
}
```

**Example for 4 voices:**
- Voice 0: 0° phase offset
- Voice 1: 90° phase offset
- Voice 2: 180° phase offset
- Voice 3: 270° phase offset

**Effect:** Creates evenly distributed LFO sweeps across voices, preventing synchronized "swoosh" and creating lush, continuous modulation.

---

### 6.3 Saturation Transfer Function

**Normalized tanh with asymmetry:**

```cpp
float saturate(float x, float drive) {
    const float maxDrive = 0.5f;  // Keep saturation subtle
    float scaledDrive = drive * maxDrive;

    if (scaledDrive < 0.01f) return x;  // Bypass if minimal

    // Asymmetric drive for BBD character
    float driveMultiplier = (x >= 0.0f) ? 1.0f : 0.9f;
    float driven = x * (1.0f + scaledDrive * driveMultiplier);
    float normalizer = std::tanh(1.0f + scaledDrive * driveMultiplier);

    return std::tanh(driven) / normalizer;
}
```

**Characteristics:**
- Positive peaks driven slightly harder (10% asymmetry)
- Normalized to maintain unity gain at input=1.0
- Gradual onset (subtle at low drive, more pronounced at high drive)

---

### 6.4 Tone Filter Frequency Mapping

**Exponential mapping from parameter to cutoff frequency:**

```cpp
float mapToneParamToCutoff(float toneParam) {
    // toneParam: -1.0 to +1.0
    // Output: 2000 Hz to 20000 Hz

    const float minCutoff = 2000.0f;
    const float maxCutoff = 20000.0f;
    const float centerCutoff = 8000.0f;  // At tone = 0.0

    if (toneParam < 0.0f) {
        // Negative: 2kHz to 8kHz
        float t = (toneParam + 1.0f);  // 0.0 to 1.0
        return minCutoff + (centerCutoff - minCutoff) * t;
    } else {
        // Positive: 8kHz to 20kHz
        float t = toneParam;  // 0.0 to 1.0
        return centerCutoff + (maxCutoff - centerCutoff) * t;
    }
}
```

---

### 6.5 Stereo Panning

**Equal-power pan law:**

```cpp
float calculatePanGains(float panPosition, float width, float& leftGain, float& rightGain) {
    // panPosition: 0.0 (left) to 1.0 (right)
    // width: 0.0 (mono) to 1.0 (full stereo)

    // Apply width scaling
    float effectivePan = 0.5f + (panPosition - 0.5f) * width;

    // Constant-power panning
    float panAngle = effectivePan * juce::MathConstants<float>::pi / 2.0f;  // 0 to π/2
    leftGain = std::cos(panAngle);
    rightGain = std::sin(panAngle);
}
```

**Width effect:**
- **Width = 0%:** All voices panned to center (leftGain = rightGain = 0.707)
- **Width = 100%:** Full pan spread across stereo field
- **Width = 50%:** Moderate spread (voices clustered toward center)

---

## 7. Integration Points

### 7.1 Dependencies

**Required JUCE modules:**
- `juce_audio_processors` - AudioProcessor base class, APVTS
- `juce_dsp` - DelayLine, IIR filters, ProcessSpec
- `juce_core` - Math constants, random number generation

**CMakeLists.txt:**
```cmake
target_link_libraries(O-Chorus
    PRIVATE
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_core
)
```

---

### 7.2 Parameter Interactions

**Rate ↔ Depth:**
- Higher rate with high depth can create seasick modulation
- Recommended: Inverse relationship for musical results (fast rate → lower depth)
- UI hint: "For faster rates, reduce depth to avoid excessive modulation"

**Voices ↔ Width:**
- More voices benefit from wider stereo spread
- Single voice → width has no effect (mono source)
- Recommended: Width = 50% for 2 voices, 70%+ for 4+ voices

**Depth ↔ Tone:**
- High depth with bright tone emphasizes modulation artifacts
- Dark tone (negative) smooths out deep modulation
- Recommended: Darker tone with deeper chorus for classic BBD sound

**Mix ↔ Voices:**
- More voices → can use higher mix without overwhelming dry signal
- Fewer voices → keep mix moderate for blend
- 100% mix = vibrato mode (works best with 1-2 voices)

---

### 7.3 Processing Order Constraints

**CRITICAL: Delay lines MUST be processed in this order:**

1. **LFO phase increment** (update global phase)
2. **Per-voice modulated delay read** (popSample with modulated delay time)
3. **Per-voice delay write** (pushSample with input)
4. **Saturation** (after delay read, before tone)
5. **Tone filter** (after saturation, before pan)
6. **Pan and sum** (final wet signal generation)
7. **Mix with dry** (very last step)

**Rationale:**
- Delay write AFTER read prevents one-sample latency
- Saturation AFTER delay prevents feedback buildup
- Tone AFTER saturation shapes final character
- Pan LAST ensures consistent stereo image

---

### 7.4 Thread Boundaries

**Audio thread operations (real-time safe):**
- All DSP processing in `processBlock()`
- Parameter reads from APVTS (atomic loads)
- Delay line reads/writes
- Filter processing

**Message thread operations (non-real-time):**
- Parameter updates from UI
- Voice count changes (triggers `prepareToPlay()`)
- Tone filter coefficient recalculation

**Shared state protection:**
- APVTS handles parameter thread safety automatically
- Voice count changes are safe (discrete steps, host calls `prepareToPlay()`)
- No custom locks needed

---

## 8. Implementation Risks

### 8.1 HIGH Risk: Delay Line Modulation Artifacts

**Issue:** Rapid delay time changes can cause clicks/pops even with interpolation.

**Symptoms:**
- Audible clicks at high LFO rates (>3 Hz) with high depth
- Frequency-dependent artifacts (worse with bright input signals)

**Mitigation:**
1. **Use Lagrange3rd interpolation** (better than linear, reduces artifacts)
2. **Limit LFO rate to 5 Hz max** (chorus shouldn't be faster)
3. **Smooth depth parameter changes** (50ms ramp via SmoothedValue)
4. **Cap maximum depth** to prevent extreme delay jumps

**Fallback architecture:**
- If artifacts persist: Switch to Thiran allpass interpolation (higher CPU, perfect magnitude response)
- Last resort: Oversample by 2x (doubles CPU, eliminates aliasing)

**Testing strategy:**
- Sweep LFO rate from 0.05 to 5 Hz at 100% depth
- Input: Sine wave at 1kHz, 5kHz, 10kHz
- Listen for clicks, pops, or pitch artifacts
- Validate with spectrum analyzer (no unexpected harmonics)

---

### 8.2 MEDIUM Risk: CPU Usage with 8 Voices

**Issue:** 8 voices × (delay + saturation + filter + pan) may exceed real-time budget on low-end systems.

**Estimated CPU cost (per voice at 48kHz):**
- Delay line (Lagrange3rd): ~15 ops/sample
- Tanh saturation: ~20 ops/sample
- One-pole filter: ~5 ops/sample
- Pan calculation: ~5 ops/sample
- **Total per voice:** ~45 ops/sample
- **8 voices:** ~360 ops/sample

**Mitigation:**
1. **Optimize saturation:** Use lookup table for tanh (trade memory for speed)
2. **SIMD processing:** Process multiple voices in parallel (SSE/NEON)
3. **Voice culling:** Allow user to reduce voice count if CPU is maxed
4. **Block processing:** Process in larger blocks to amortize overhead

**Fallback architecture:**
- If CPU too high: Reduce max voices to 4 (still lush chorus)
- Alternative: Offer "Lite mode" with linear interpolation (lower quality, lower CPU)

**Testing strategy:**
- Profile with 8 voices at 96kHz (worst case)
- Target: <10% CPU on Intel i5 (2015) or Apple M1
- Stress test: 10 instances in DAW with 256-sample buffer

---

### 8.3 MEDIUM Risk: Stereo Phase Coherence

**Issue:** Independent L/R modulation can cause phase cancellation when summed to mono.

**Symptoms:**
- Thin sound in mono playback (radio, phone speakers)
- Comb filtering when stereo mix summed to mono

**Mitigation:**
1. **Use mono input for chorus processing** (sum L+R before delay lines)
2. **Stereo imaging from pan only** (not from independent modulation)
3. **Test mono compatibility** in every build

**Fallback architecture:**
- If mono compatibility critical: Add "Mono Safe" mode (reduces width, ensures phase coherence)

**Testing strategy:**
- Process stereo input, sum output to mono, compare to dry
- Listen for "hollow" sound or excessive filtering
- Use correlation meter (should stay > 0.7 in mono sum)

---

### 8.4 LOW Risk: Parameter Smoothing Latency

**Issue:** Smoothed parameters (50-100ms ramps) may feel sluggish for rapid tweaks.

**Symptoms:**
- Rate changes lag behind knob movement
- Depth adjustments feel "mushy"

**Mitigation:**
1. **Use 50ms smoothing** (fast enough for real-time tweaking)
2. **Bypass smoothing for Voices parameter** (discrete steps don't need it)
3. **Offer "Fast" mode** for live performance (reduce smoothing to 10ms)

**Fallback architecture:**
- If smoothing too slow: Reduce to 20ms (slight risk of clicks, but more responsive)

**Testing strategy:**
- Rapid rate changes (0.05 → 5 Hz → 0.05 Hz)
- Confirm no audible clicks/pops
- Verify parameter response feels immediate (<100ms perceived latency)

---

### 8.5 LOW Risk: Denormal Numbers in Delay Lines

**Issue:** Very small values in delay buffers can cause CPU spikes (denormal penalty).

**Symptoms:**
- Random CPU usage spikes when plugin idle
- Worse on older CPUs (pre-2010 Intel)

**Mitigation:**
1. **Use `juce::ScopedNoDenormals` in `processBlock()`** (compiler intrinsics)
2. **Add DC offset removal** (highpass filter at 5 Hz on input)
3. **Flush delay lines to zero** when input silent for >1 second

**Fallback architecture:**
- If denormals persist: Add tiny noise (-120 dB) to prevent denormal formation

**Testing strategy:**
- Process silence for 10 seconds
- Monitor CPU usage (should stay flat, no spikes)
- Test on Intel Core 2 Duo (known denormal issues)

---

## 9. Architecture Decisions

### 9.1 Why Lagrange3rd Interpolation?

**Decision:** Use `juce::dsp::DelayLineInterpolationTypes::Lagrange3rd` over Thiran or Linear.

**Alternatives considered:**
1. **Linear interpolation** - Simplest, lowest CPU
   - **Rejected:** Introduces audible high-frequency rolloff, "dulls" chorus sound
2. **Thiran allpass interpolation** - Flat magnitude response, highest quality
   - **Rejected:** 2x CPU cost, overkill for slow LFO rates (chorus < 5 Hz)
3. **No interpolation** - Lowest CPU, integer delays only
   - **Rejected:** Causes severe zipper noise during modulation

**Why Lagrange3rd wins:**
- Good frequency response (minimal rolloff compared to linear)
- Moderate CPU cost (30% higher than linear, 50% lower than Thiran)
- Sufficient quality for chorus (LFO rates slow enough that artifacts minimal)
- Industry standard for modulation effects (used in most commercial chorus plugins)

**Supporting evidence:**
- From delay-effects-comprehensive-guide.md: "Lagrange 3rd Order: Better frequency response than linear, less CPU than Thiran. Best for: Modulated delays, chorus."
- JUCE DelayLine documentation: "Lagrange3rd provides a good trade-off between quality and performance for real-time modulation."

---

### 9.2 Why Mono Sum Input for Chorus?

**Decision:** Sum stereo input to mono BEFORE chorus processing, create stereo image via panning.

**Alternatives considered:**
1. **Independent L/R chorus processing** - Two separate chorus engines
   - **Rejected:** Phase cancellation in mono sum, doubles CPU cost
2. **Mid-side processing** - Chorus on mid only, preserve side
   - **Rejected:** Complexity for minimal benefit, chorus loses stereo width control

**Why mono sum wins:**
- **Mono compatibility:** No phase cancellation when summed to mono
- **Simpler architecture:** Single chorus engine, stereo from pan only
- **Lower CPU:** Half the delay lines needed
- **Predictable stereo image:** Width parameter directly controls spread

**Supporting evidence:**
- Professional chorus pedals (Boss CE-1, CE-2) use mono input with stereo output via dual BBD chips
- Strymon Ola processes mono input through 3 delay lines, creates stereo via panning
- From web research: "Stereo chorus effect processors vary the effect between the left and right channels by offsetting the delay or phase of the LFO, with the effect thereby enhanced because sounds are produced from multiple locations in the stereo field."

---

### 9.3 Why Tanh Saturation Over WDF Diode Clipper?

**Decision:** Use simple tanh waveshaping for saturation instead of Wave Digital Filter diode clipper.

**Alternatives considered:**
1. **WDF diode clipper** (from circuit-modeling-fundamentals.md)
   - Newton-Raphson iteration, Shockley diode equation
   - **Rejected:** 10x CPU cost, overkill for subtle chorus warmth
2. **Polynomial waveshaping** - `1.5x - 0.5x³`
   - **Rejected:** Less smooth than tanh, potential aliasing at high frequencies
3. **No saturation** - Clean delay only
   - **Rejected:** Loses "analog warmth" character from brief

**Why tanh wins:**
- **Simplicity:** Single function call, no iterative solver
- **CPU efficient:** ~20 ops/sample vs ~100 for WDF
- **Smooth transfer function:** No sharp transitions, minimal aliasing
- **Sufficient character:** 90% of BBD warmth at 10% of CPU cost
- **Normalizable:** Easy to maintain unity gain

**Supporting evidence:**
- From circuit-modeling-fundamentals.md: "Tanh provides smooth, musical saturation without harsh clipping."
- From web research (tape saturation): "The saturation curve depends on the tape material, but usually it's some sort of soft-clip sigmoid."
- BBD chorus effects have inherent soft saturation from bucket-brigade transfer characteristics, not hard clipping.

---

### 9.4 Why Fixed Phase Distribution Over Random?

**Decision:** Use fixed phase offset `(2π * i) / numVoices` instead of random phase per voice.

**Alternatives considered:**
1. **Random phase per voice** - Randomized 0-2π offset
   - **Rejected:** Unpredictable stereo image, inconsistent sound between sessions
2. **Golden ratio phase distribution** - Optimal spacing via φ = 1.618...
   - **Rejected:** Minimal perceptual difference vs even spacing, added complexity

**Why fixed even spacing wins:**
- **Predictable sound:** Same voice count always sounds the same
- **Optimal coverage:** Even spacing ensures no phase "gaps" in modulation
- **Symmetrical stereo image:** Pairs of voices balance L/R
- **Preset compatibility:** Same settings sound identical across sessions

**Supporting evidence:**
- From web research: "The Juno chorus uses quadrature LFO (90° offset) between two BBD chips."
- From web research: "A professional chorus plugin can be designed with three voices (left, centre and right) with the left and right voices set to have a -120º and +120º phase shift relative to the centre voice."
- Even phase distribution is industry standard for multi-voice chorus.

---

## 10. Special Considerations

### 10.1 Thread Safety

**Audio thread constraints:**
- NO memory allocation (all buffers pre-allocated in `prepareToPlay()`)
- NO locks or mutexes
- NO file I/O or network operations
- Parameters read via atomic loads from APVTS

**Message thread operations:**
- Parameter updates (APVTS handles thread safety)
- Voice count changes (triggers `prepareToPlay()` re-call)

**Synchronization:**
- APVTS provides lock-free parameter access
- Voice count change is atomic (discrete steps, no intermediate states)

---

### 10.2 Performance Optimization

**CPU budget target:** <5% on Intel i5 (2015) with 8 voices at 48kHz, 512-sample buffer

**Optimization strategies:**
1. **SIMD potential:** Process multiple voices in parallel (SSE/NEON)
2. **Lookup table for tanh:** Trade 4KB memory for 5x speedup
3. **Block processing:** Update LFO once per block instead of per sample
4. **Voice culling:** Only process active voices (when numVoices < 8)

**Profiling checkpoints:**
- Delay line processing: <40% of total CPU
- Saturation: <20% of total CPU
- Tone filtering: <10% of total CPU
- Pan and mix: <10% of total CPU
- Overhead: <20% of total CPU

---

### 10.3 Denormal Prevention

**Strategy:** Use `juce::ScopedNoDenormals` in `processBlock()`

```cpp
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&) override {
    juce::ScopedNoDenormals noDenormals;
    // ... DSP processing ...
}
```

**Additional safeguards:**
- Delay lines flushed to zero when silent
- Filter states clamped to prevent denormals
- DC blocker on input (highpass at 5 Hz)

---

### 10.4 Sample Rate Independence

**All time-based values scaled by sample rate:**

- LFO phase increment: `(rate * 2π) / sampleRate`
- Delay times in samples: `(delayMs / 1000.0f) * sampleRate`
- Filter coefficients recalculated on sample rate change

**Supported sample rates:** 44.1kHz, 48kHz, 88.2kHz, 96kHz, 176.4kHz, 192kHz

**Testing:**
- Validate identical sound across all sample rates
- Verify no artifacts at 192kHz (most demanding)
- Confirm CPU scales linearly with sample rate

---

### 10.5 Latency Reporting

**Plugin latency:** 0 samples

**Rationale:** Chorus adds delayed copies of signal but doesn't introduce processing latency. Dry signal passes through with no delay.

**Implementation:**
```cpp
int getLatencySamples() const override {
    return 0;  // Chorus is zero-latency
}
```

**Note:** Delay lines create wet signal delay (5-15ms), but this is intentional effect, not latency compensation.

---

## 11. Research References

### 11.1 Professional Plugins Analyzed

**1. Strymon Ola dBucket Chorus**
- Multi-voice implementation: 3 delay lines (tri-chorus mode)
- "Cyclonic swirl" character from phase distribution
- Source: [Strymon Ola Manual](https://www.strymon.net/manuals/Ola_UserManual.pdf)
- Observation: Tri-chorus prevents "swoosh" by distributing modulation evenly

**2. Boss CE-1 Chorus Ensemble**
- BBD chip: MN3207 (1024-stage)
- Warm analog character from BBD saturation
- Source: Web research on Boss CE-1 architecture
- Observation: BBD chips naturally roll off high frequencies (~8kHz)

**3. Roland Juno-60 Chorus**
- BBD chip: MN3009 (256-stage)
- Quadrature LFO (90° phase offset between two BBD chips)
- Source: Web research and forum discussions
- Observation: Dual BBD with quadrature LFO creates wide stereo image

**4. D16 Syntorus 2**
- "Analog BBD Emulation" mode for warmth
- Multi-voice with per-voice depth variation
- Source: D16 Syntorus 2 product page
- Observation: Modern plugins use randomization for organic feel

**5. TAL-Chorus-LX**
- Software reference for BBD emulation
- Mentioned in brief as software reference
- Observation: Software can achieve BBD character via filtering and saturation

---

### 11.2 Technical Resources

**JUCE Documentation:**
- [JUCE dsp::DelayLine API](https://docs.juce.com/master/classdsp_1_1DelayLine.html)
- [JUCE dsp::Chorus API](https://docs.juce.com/master/classdsp_1_1Chorus.html) (analyzed for reference, not used directly)
- [JUCE dsp::IIR::Filter](https://docs.juce.com/master/classjuce_1_1dsp_1_1IIR_1_1Filter.html)

**Academic:**
- Stanford CCRMA - Delay-Line Interpolation: https://ccrma.stanford.edu/~jos/pasp/Delay_Line_Interpolation.html
- Lagrange interpolation for fractional delay

**Internal Research Documents:**
- `research/delay-effects-comprehensive-guide.md` - Interpolation methods, modulation techniques
- `research/circuit-modeling-fundamentals.md` - Saturation waveshaping, BBD modeling

**Web Research:**
- Bucket-brigade delay history and emulation techniques
- Multi-voice chorus phase distribution strategies
- Analog saturation and tape emulation DSP techniques
- Stereo imaging for modulation effects

---

## 12. Appendix: JUCE Module Dependencies

**Required modules:**
- `juce::juce_audio_processors` - AudioProcessor, APVTS, parameters
- `juce::juce_dsp` - DelayLine, IIR filters, ProcessSpec
- `juce::juce_core` - MathConstants, Random, utilities

**CMakeLists.txt target_link_libraries:**
```cmake
target_link_libraries(O-Chorus
    PRIVATE
        juce::juce_audio_processors
        juce::juce_dsp
        juce::juce_core
)

juce_generate_juce_header(O-Chorus)  # CRITICAL for JUCE 8
```

**Include directives:**
```cpp
#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
```

---

## 13. Implementation Checklist

**Before Stage 2 (DSP Implementation):**

- [ ] Review juce8-critical-patterns.md for DelayLine usage
- [ ] Confirm ProcessSpec preparation pattern (sampleRate, maximumBlockSize, numChannels)
- [ ] Verify SmoothedValue usage for parameter smoothing
- [ ] Check ScopedNoDenormals placement in processBlock()
- [ ] Validate thread safety (audio vs message thread operations)

**During Stage 2 (DSP Implementation):**

- [ ] Initialize delay lines with correct max size (50ms)
- [ ] Implement per-voice phase offset calculation
- [ ] Generate randomized depth variations (seeded for repeatability)
- [ ] Implement LFO with correct phase accumulation
- [ ] Apply Lagrange3rd interpolation via JUCE DelayLine
- [ ] Implement tanh saturation with asymmetry
- [ ] Configure one-pole tone filter with correct coefficients
- [ ] Implement equal-power panning across voices
- [ ] Test mono compatibility (sum to mono, verify no phase issues)

**Testing checklist:**

- [ ] Sweep LFO rate 0.05-5 Hz, listen for artifacts
- [ ] Test all voice counts (1-8), verify stereo image
- [ ] Validate Width parameter (0-100%) effect on stereo spread
- [ ] Test Tone parameter (dark to bright), verify filter response
- [ ] Profile CPU usage with 8 voices at 96kHz
- [ ] Verify parameter smoothing (no clicks on rapid changes)
- [ ] Test denormal prevention (process silence, monitor CPU)
- [ ] Validate sample rate independence (44.1kHz to 192kHz)

---

**End of Architecture Specification**
