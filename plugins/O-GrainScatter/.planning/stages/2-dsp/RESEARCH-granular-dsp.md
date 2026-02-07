# Stage 2: DSP Implementation - Granular Synthesis Engine Research

**Researched:** 2026-02-07
**Domain:** Granular synthesis DSP, JUCE 8.0.4 audio processing APIs
**Confidence:** HIGH (verified against JUCE 8.0.4 source at `/Users/taylorbrook/JUCE`)

## Summary

This research covers all DSP subsystems required for O-GrainScatter's granular stutter engine: circular delay buffer with Lagrange interpolation, 64-voice grain pool with Hann windowing, PPQ-based beat synchronization, freeze buffer management, feedback with soft clipping, and SmoothedValue usage.

All JUCE APIs were verified directly against the JUCE 8.0.4 source code on disk. The PPQ handling approach was cross-referenced against the working O-FreqPulse plugin in this project, which uses the same `getPosition()` API pattern with per-sample PPQ interpolation. The Lagrange 3rd-order interpolation coefficients were verified against JUCE's own `juce::dsp::DelayLine<float, DelayLineInterpolationTypes::Lagrange3rd>` implementation.

**Primary recommendation:** Build all DSP components as header-only classes in `Source/dsp/`. Use a manual circular buffer (not `juce::dsp::DelayLine`) because grains need arbitrary position reads. Compute Hann windows per-sample (not LUT). Interpolate PPQ linearly within blocks using `bpm / (60.0 * sampleRate)` stride. Pre-allocate everything in `prepareToPlay()`.

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- All 7 DSP components confirmed: DelayBuffer, GrainPool, GrainScheduler, TempoTracker, FreezeManager, ScaleQuantizer, EuclideanGenerator
- 64-voice grain pool with Lagrange 3rd-order interpolation and Hann window envelopes
- Beat sync via PPQ subdivision crossing detection with Euclidean gating
- Freeze captures 4x grain size from delay buffer, loops indefinitely
- 5 scales (Chromatic, Major, Minor, Pentatonic, Whole Tone) with 4 pitch modes
- Feedback path with soft clipping safety
- DSP file organization: header-only (.h) in `Source/dsp/`
- "Texture" parameter renamed to "Spread" (ID `spread`, display "Spread") -- position spread only
- Feedback safety: soft clip at 0.95 using `tanh`-style saturation
- Small latency acceptable (~5ms)
- New parameter: "Stutter Gate" (Bool, default off) -- 18th parameter total
- Stutter Gate Off = dry signal always passes through; On = full signal gated between repeat bursts

### Claude's Discretion
- Internal implementation details of each DSP class
- Performance optimization choices (e.g., Hann LUT vs compute)
- Exact crossfade duration for freeze engage/release (within ~5ms constraint)
- PPQ interpolation strategy within blocks

### Deferred Ideas (OUT OF SCOPE)
- None listed -- all decisions resolved
</user_constraints>

---

## Standard Stack

### Core JUCE 8.0.4 APIs

| API | Module | Purpose | Verified |
|-----|--------|---------|----------|
| `juce::AudioBuffer<float>` | juce_audio_basics | Circular delay buffer storage, freeze buffer | YES - source at `buffers/juce_AudioSampleBuffer.h` |
| `juce::AudioPlayHead::PositionInfo` | juce_audio_basics | PPQ position, BPM, playing state | YES - source at `audio_play_head/juce_AudioPlayHead.h` |
| `juce::SmoothedValue<float>` | juce_audio_basics | Zipper-free dry/wet and feedback | YES - source at `utilities/juce_SmoothedValue.h` |
| `juce::Random` | juce_core | Real-time safe PRNG (per-instance) | YES - seed-based, no system calls |
| `juce::MathConstants<float>::twoPi` | juce_core | Hann window calculation | YES |
| `juce::AudioProcessorValueTreeState` | juce_audio_processors | Parameter management | YES - already in PluginProcessor |

### What NOT to Use

| Component | Don't Use | Why |
|-----------|-----------|-----|
| Circular buffer | `juce::dsp::DelayLine` | DelayLine manages its own read/write pointers with a single-tap model. Even with `popSample(ch, delay, false)` for multi-tap, it uses a shared `delayFrac`/`delayInt` state that gets overwritten by `setDelay()` on each call. 64 grains each reading at different offsets would conflict. Manual buffer gives direct pointer access. |
| Window function | `juce::dsp::WindowingFunction` | Designed for FFT windowing (fills a buffer once). We need per-sample phase-based window values for variable-length grains. Direct `cos()` is simpler and faster. |
| System random | `juce::Random::getSystemRandom()` | Shared instance, potential thread contention. Use per-processor `juce::Random` instance. |

---

## Architecture Patterns

### Recommended File Structure

```
Source/dsp/
    DelayBuffer.h          # Circular stereo buffer + Lagrange3rd read
    GrainPool.h            # GrainVoice struct + GrainPool class (spawn, process, Hann window)
    GrainScheduler.h       # Free mode + Sync mode grain scheduling
    TempoTracker.h         # PPQ reading from AudioPlayHead, manual fallback
    FreezeManager.h        # Freeze buffer capture, interpolated read, engage/release
    ScaleQuantizer.h       # 5 scales, root note, pitch->rate, PitchLadder modes
    EuclideanGenerator.h   # Bjorklund pattern generation (namespace, not class)
```

All header-only. All classes are small-medium and benefit from inlining in the per-sample inner loop.

### Pattern 1: Pre-allocated Voice Pool with Round-Robin Stealing

**What:** Fixed-size `std::array<GrainVoice, 64>` with a round-robin index for allocation. No dynamic allocation ever.

**When to use:** Any polyphonic per-sample processor where voices are spawned at runtime.

**Implementation:**
```cpp
// Source: Verified pattern from ARCHITECTURE.md + real-time safety requirements
struct GrainVoice
{
    bool active = false;
    float readPosition = 0.0f;       // Delay offset in samples (fractional)
    float playbackRate = 1.0f;       // Pitch ratio
    float panPosition = 0.5f;        // 0=left, 1=right
    int samplesRemaining = 0;        // Countdown
    int grainLengthSamples = 0;      // For Hann window phase calculation
    bool reverse = false;
    bool readFromFrozen = false;
};

class GrainPool
{
public:
    static constexpr int MAX_VOICES = 64;

    void spawnGrain(const GrainParams& params)
    {
        // Round-robin: always advance, overwrite if all voices active
        auto& voice = voices[nextVoiceIndex];
        voice = {}; // Reset
        voice.active = true;
        voice.readPosition = params.startOffset;
        voice.playbackRate = params.playbackRate;
        voice.panPosition = params.pan;
        voice.grainLengthSamples = params.lengthSamples;
        voice.samplesRemaining = params.lengthSamples;
        voice.reverse = params.reverse;
        voice.readFromFrozen = params.frozen;

        nextVoiceIndex = (nextVoiceIndex + 1) % MAX_VOICES;
    }

private:
    std::array<GrainVoice, MAX_VOICES> voices;
    int nextVoiceIndex = 0;
};
```

**Key insight:** Round-robin guarantees O(1) allocation with no searching for free voices. The oldest voice is always the next one in the ring, so it gets stolen first. No sorting or priority needed.

### Pattern 2: Per-Sample Processing Loop

**What:** The processBlock inner loop processes one sample at a time across all components.

**Critical ordering:**
```cpp
for (int i = 0; i < numSamples; ++i)
{
    // 1. Read input
    float inL = buffer.getSample(0, i);
    float inR = buffer.getSample(1, i);

    // 2. Write to delay buffer (input + feedback)
    delayBuffer.pushSample(inL + feedbackL, inR + feedbackR);

    // 3. Check if grain should spawn at this sample
    if (spawnScheduledAtSample(i))
        grainPool.spawnGrain(/* params */);

    // 4. Process all active grains -> wet output
    float wetL = 0.0f, wetR = 0.0f;
    grainPool.processSample(delayBuffer, freezeManager, wetL, wetR);

    // 5. Compute feedback from wet signal
    float fbAmount = feedbackSmoothed.getNextValue();
    feedbackL = softClip(wetL * fbAmount);
    feedbackR = softClip(wetR * fbAmount);

    // 6. Mix dry/wet
    float mix = dryWetSmoothed.getNextValue();
    buffer.setSample(0, i, inL * (1.0f - mix) + wetL * mix);
    buffer.setSample(1, i, inR * (1.0f - mix) + wetR * mix);
}
```

**Why this order matters:**
- Delay buffer must be written BEFORE grains read from it (grains read backward from write head)
- Feedback uses PREVIOUS iteration's wet output (1-sample delay is acceptable and prevents infinite recursion)
- SmoothedValue `getNextValue()` called once per sample, applied to both channels

### Pattern 3: Block-Start PPQ with Per-Sample Linear Interpolation

**What:** Read PPQ once at block start from `AudioPlayHead`, then linearly interpolate per-sample.

**Source:** Verified from JUCE 8.0.4 `AudioPlayHead.h` line 577-590 -- "Fetches details about the transport's position at the start of the current processing block." Also matches the working pattern in O-FreqPulse `PluginProcessor.cpp` lines 484-520.

```cpp
// At block start:
auto* playHead = getPlayHead();
double blockStartPpq = 0.0;
double ppqPerSample = 0.0;
bool gotValidPosition = false;

if (playHead != nullptr)
{
    if (auto posInfo = playHead->getPosition())
    {
        if (posInfo->getIsPlaying() && posInfo->getPpqPosition().hasValue())
        {
            blockStartPpq = *posInfo->getPpqPosition();
            double bpm = posInfo->getBpm().orFallback(120.0);
            ppqPerSample = bpm / (60.0 * sampleRate);
            gotValidPosition = true;
        }
    }
}

// Standalone fallback:
if (!gotValidPosition)
{
    ppqPerSample = 120.0 / (60.0 * sampleRate);
    blockStartPpq = lastPpqPosition;  // Continue from last block
    gotValidPosition = true;
}

// Per-sample:
for (int i = 0; i < numSamples; ++i)
{
    double samplePpq = blockStartPpq + ppqPerSample * (double)i;
    // ... subdivision crossing detection ...
}

// End of block:
lastPpqPosition = blockStartPpq + ppqPerSample * (double)numSamples;
```

### Anti-Patterns to Avoid

- **Calling `getPlayHead()->getPosition()` per-sample:** The position only changes per-block. Calling it 512 times wastes cycles and returns the same value each time.
- **Using `std::vector` for Euclidean patterns on the audio thread:** Use `std::array<bool, 16>` with an atomic length field. The vector itself is fine for generation (message thread), but the audio thread must read a fixed-size copy.
- **Allocating freeze buffer on engage:** Pre-allocate max size (2 seconds) in `prepareToPlay()`. On engage, just `memcpy`/`copyFrom` and set the length.
- **Using `juce::Random::getSystemRandom()` on the audio thread:** Shared global instance. Use a member `juce::Random rng` per-processor.

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Parameter smoothing | Custom ramp logic | `juce::SmoothedValue<float>` | Handles edge cases (reset, skip, target already reached). Linear type is correct for gain/mix. |
| Random number generation | `rand()` or `std::mt19937` | `juce::Random` (per-instance) | Real-time safe, deterministic seed, no system calls, `nextFloat()` returns [0,1). |
| Parameter value access | Manual atomic wrappers | `APVTS::getRawParameterValue()` | Returns `std::atomic<float>*`, lock-free, already set up in Stage 1 code. |
| State save/restore | Custom XML serialization | `APVTS::copyState()` / `replaceState()` | Already implemented in Stage 1 `getStateInformation()`/`setStateInformation()`. |

**Key insight:** The Lagrange interpolation and Hann window ARE hand-rolled, but correctly so -- they're 3-5 lines of arithmetic with no edge cases. The JUCE equivalents (`DelayLine`, `WindowingFunction`) are designed for different use cases.

---

## Common Pitfalls

### Pitfall 1: Circular Buffer Wrap-Around in Lagrange Interpolation
**What goes wrong:** Lagrange 3rd-order reads 4 adjacent samples (indices n-1, n, n+1, n+2). At the buffer boundary, one or more indices wrap around, reading garbage if not handled.
**Why it happens:** The fractional read position maps to 4 sample indices. When any index goes negative or exceeds buffer size, it must wrap.
**How to avoid:** Always modulo-wrap all 4 indices independently:
```cpp
float readSample(int channel, float delaySamples) const
{
    float readPos = (float)writePosition - delaySamples;
    int intPos = (int)std::floor(readPos);
    float frac = readPos - (float)intPos;

    // 4-point Lagrange needs samples at intPos-1, intPos, intPos+1, intPos+2
    auto wrap = [this](int idx) {
        return ((idx % bufferSize) + bufferSize) % bufferSize;
    };

    const float* data = buffer.getReadPointer(channel);
    float y0 = data[wrap(intPos - 1)];
    float y1 = data[wrap(intPos)];
    float y2 = data[wrap(intPos + 1)];
    float y3 = data[wrap(intPos + 2)];

    return lagrange3(y0, y1, y2, y3, frac);
}
```
**Warning signs:** Periodic clicks at a rate matching the buffer length (every ~2 seconds at 44.1kHz).

### Pitfall 2: PPQ Jump on DAW Loop Point
**What goes wrong:** When the DAW loops (e.g., from bar 8 back to bar 1), the PPQ jumps backward. Naive subdivision crossing detection (`floor(newPpq/subdiv) > floor(oldPpq/subdiv)`) fires a false trigger because the old PPQ is larger than the new one.
**Why it happens:** `PositionInfo::getPpqPosition()` reflects the actual playhead position, which jumps backward on loop.
**How to avoid:** Check for discontinuity and reset:
```cpp
// Detect PPQ jump (loop point or transport restart)
if (newPpq < oldPpq - 0.01)
{
    // PPQ jumped backward -- treat as fresh start, don't trigger
    oldPpq = newPpq;
    return false;
}
return std::floor(newPpq / subdivPpq) > std::floor(oldPpq / subdivPpq);
```
**Warning signs:** Extra grain burst every time DAW loops.

### Pitfall 3: Freeze Buffer Click on Engage
**What goes wrong:** When freeze engages, currently-active grains are reading from the delay buffer. If their read positions don't exist in the frozen buffer, they read garbage or create a discontinuity.
**Why it happens:** Active grains may be reading from positions outside the frozen region.
**How to avoid:** Two approaches (use both):
1. Let currently-active grains finish naturally (they keep reading from delay buffer via `readFromFrozen = false`). Only NEW grains spawned after engage read from frozen buffer.
2. Apply a ~5ms (220 samples at 44.1kHz) crossfade ramp when transitioning:
```cpp
// During crossfade period:
float crossfadeGain = (float)crossfadeSamplesRemaining / crossfadeLengthSamples;
float output = liveSample * crossfadeGain + frozenSample * (1.0f - crossfadeGain);
```
**Warning signs:** Audible click when pressing freeze button.

### Pitfall 4: Feedback Runaway
**What goes wrong:** With feedback > 0 and grains continuously playing, the signal level grows without bound, eventually clipping hard or producing digital distortion.
**Why it happens:** Each grain's output is mixed back into the delay buffer. If the total gain exceeds 1.0, the signal grows exponentially.
**How to avoid:** Apply soft clipping BEFORE writing back to the delay buffer:
```cpp
static constexpr float tanhScaler = 1.0f / std::tanh(3.0f);  // ~1.0049 -- precompute

float softClip(float x)
{
    return std::tanh(x * 3.0f) * tanhScaler * 0.95f;
}
```
Note: `std::tanh(3.0f)` is ~0.9951, so `tanhScaler` normalizes the output range. The 0.95 ceiling provides headroom. At normal levels, `tanh(x*3) * tanhScaler` is approximately linear (gain ~1.0). At high levels, it saturates smoothly.

**Performance note:** `std::tanh` costs ~6-140 CPU cycles depending on compiler/platform. For a single feedback sample per iteration (not 64 voices), this is negligible. If profiling shows it as a hotspot, use the rational approximation: `x * (27.0f + x*x) / (27.0f + 9.0f*x*x)` which has ~2.6% max error and costs ~3 cycles.

### Pitfall 5: SmoothedValue Reset on prepareToPlay
**What goes wrong:** `SmoothedValue::reset(sampleRate, rampTime)` sets the current value to the target, killing any in-progress ramp. If `prepareToPlay()` is called during playback (e.g., sample rate change), this causes a discontinuity.
**Why it happens:** JUCE's `SmoothedValue::reset()` calls `setCurrentAndTargetValue(target)`, which snaps immediately.
**How to avoid:** This is generally acceptable for `prepareToPlay()` (DAWs expect brief silence during reconfiguration). But be aware -- DO NOT call `reset()` in `processBlock()`. Only call `setTargetValue()` there.
```cpp
// In prepareToPlay():
dryWetSmoothed.reset(sampleRate, 0.02);  // 20ms ramp
feedbackSmoothed.reset(sampleRate, 0.02);

// In processBlock():
dryWetSmoothed.setTargetValue(dryWetParam->load() / 100.0f);  // 0-1 range
feedbackSmoothed.setTargetValue(feedbackParam->load() / 100.0f);
```

### Pitfall 6: Grain-Level Hann Window Phase Calculation
**What goes wrong:** Using `samplesRemaining / grainLengthSamples` for the window phase produces a phase that starts at 1.0 and decreases to 0.0. The Hann window `0.5 * (1 - cos(2*pi*phase))` would start and end at 0, but the phase direction must be correct.
**Why it happens:** `samplesRemaining` counts down from `grainLengthSamples` to 0.
**How to avoid:** Compute phase as the proportion of the grain that has ELAPSED, not remaining:
```cpp
float phase = 1.0f - (float)voice.samplesRemaining / (float)voice.grainLengthSamples;
// phase goes from 0.0 (grain start) to ~1.0 (grain end)
float window = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));
```
This produces the correct Hann shape: starts at 0, peaks at 0.5 phase, returns to 0.

---

## Code Examples

### Lagrange 3rd-Order Interpolation (4-Point)

Verified against JUCE's own implementation in `juce_DelayLine.h` lines 234-265:

```cpp
// Source: JUCE 8.0.4 juce_dsp/processors/juce_DelayLine.h (Lagrange3rd path)
// Adapted for direct use with a circular buffer

inline float lagrange3(float y0, float y1, float y2, float y3, float frac)
{
    // JUCE's implementation uses Neville's algorithm form:
    float d1 = frac - 1.0f;
    float d2 = frac - 2.0f;
    float d3 = frac - 3.0f;

    float c1 = -d1 * d2 * d3 / 6.0f;
    float c2 = d2 * d3 * 0.5f;
    float c3 = -d1 * d3 * 0.5f;
    float c4 = d1 * d2 / 6.0f;

    return y0 * c1 + frac * (y1 * c2 + y2 * c3 + y3 * c4);
}
```

**Important:** JUCE's DelayLine adjusts `frac` when it's < 2.0 (see `updateInternalVariables()` line 293-298). For our manual buffer, `frac` is always in [0, 1) as computed by `readPos - floor(readPos)`, so no adjustment needed. The 4 sample points are at indices `intPos-1, intPos, intPos+1, intPos+2` relative to the integer read position.

**Alternative form** (from ARCHITECTURE.md, equivalent but different coefficient calculation):
```cpp
// Horner form -- slightly fewer operations
inline float lagrange3_horner(float y0, float y1, float y2, float y3, float frac)
{
    float c0 = y1;
    float c1 = y2 - (1.0f/3.0f)*y0 - 0.5f*y1 - (1.0f/6.0f)*y3;
    float c2 = 0.5f*(y0 + y2) - y1;
    float c3 = (1.0f/6.0f)*(y3 - y0) + 0.5f*(y1 - y2);
    return ((c3 * frac + c2) * frac + c1) * frac + c0;
}
```

Both forms are mathematically equivalent. The JUCE form (`lagrange3`) maps more directly to the standard Lagrange basis polynomial. The Horner form has one fewer multiply. Recommend using the JUCE form for consistency.

### Hann Window -- Compute Per-Sample (Not LUT)

**Recommendation: Compute per-sample, not lookup table.**

**Rationale (HIGH confidence, verified):**
- A Hann window is a single `cos()` call plus multiply and add
- Modern CPUs compute `cos()` in ~3-10 cycles with SIMD
- A lookup table for variable-length grains would need either:
  - A very large table (cache-hostile -- evicts useful data from L1/L2 cache)
  - Interpolation between table entries (adds complexity, barely saves cycles)
- With 64 voices max, that's 64 `cos()` calls per sample = ~640 cycles worst case = negligible at 44.1kHz
- The KVR consensus confirms: "Using large tables on modern CPUs for something you can compute on the fly is a terrible idea" due to cache pressure

```cpp
// Per-sample Hann window
// phase: 0.0 at grain start, ~1.0 at grain end
inline float hannWindow(float phase)
{
    return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));
}
```

### Subdivision Crossing Detection

```cpp
// Source: ARCHITECTURE.md + verified working in O-FreqPulse PluginProcessor.cpp

static constexpr double SUBDIV_VALUES[] = {
    0.0,      // Index 0: Free mode (unused)
    1.0,      // Index 1: 1/4 note
    0.5,      // Index 2: 1/8 note
    0.25,     // Index 3: 1/16 note
    0.125,    // Index 4: 1/32 note
    1.0/3.0,  // Index 5: 1/8 triplet
    1.0/6.0   // Index 6: 1/16 triplet
};

bool crossedSubdivision(double oldPpq, double newPpq, double subdivPpq)
{
    // Guard against backward jumps (DAW loop points)
    if (newPpq < oldPpq - 0.001)
        return false;

    return std::floor(newPpq / subdivPpq) > std::floor(oldPpq / subdivPpq);
}
```

### Freeze Buffer Copy from Circular Buffer

```cpp
// Source: Derived from juce::AudioBuffer::copyFrom API
// Freeze capture must handle circular buffer wrap-around

void FreezeManager::engage(const DelayBuffer& source, int grainSizeSamples)
{
    captureLength = juce::jmin(grainSizeSamples * 4, maxCaptureLength);

    // Copy from delay buffer, handling wrap-around
    // We want the most recent `captureLength` samples
    for (int ch = 0; ch < 2; ++ch)
    {
        for (int i = 0; i < captureLength; ++i)
        {
            // Read backward from write head: offset = captureLength - i
            frozenBuffer.setSample(ch, i,
                source.readSampleDirect(ch, captureLength - i));
        }
    }

    active = true;
    crossfadeSamplesRemaining = crossfadeLengthSamples;
}
```

### Soft Clipping for Feedback

```cpp
// Source: CONTEXT.md locked decision + tanh performance research

// Precompute at file scope (constexpr-safe in C++17 with some compilers,
// or compute once in prepare())
static const float kTanhScale = 1.0f / std::tanh(3.0f);

inline float softClip(float x)
{
    return std::tanh(x * 3.0f) * kTanhScale * 0.95f;
}

// Applied in processBlock inner loop:
feedbackL = softClip(wetL * fbAmount);
feedbackR = softClip(wetR * fbAmount);
```

### SmoothedValue Setup

```cpp
// Source: JUCE 8.0.4 juce_SmoothedValue.h - verified API

// In prepareToPlay():
dryWetSmoothed.reset(sampleRate, 0.02);  // 20ms ramp
feedbackSmoothed.reset(sampleRate, 0.02);
// Set initial values:
dryWetSmoothed.setCurrentAndTargetValue(dryWetParam->load() / 100.0f);
feedbackSmoothed.setCurrentAndTargetValue(feedbackParam->load() / 100.0f);

// In processBlock() -- once at block start:
dryWetSmoothed.setTargetValue(dryWetParam->load() / 100.0f);
feedbackSmoothed.setTargetValue(feedbackParam->load() / 100.0f);

// In per-sample loop:
float mix = dryWetSmoothed.getNextValue();     // Advances smoothing by 1 sample
float fb = feedbackSmoothed.getNextValue();
```

---

## Critical API Details (JUCE 8.0.4)

### AudioPlayHead::PositionInfo (JUCE 8 API)

**Source:** `/Users/taylorbrook/JUCE/modules/juce_audio_basics/audio_play_head/juce_AudioPlayHead.h`

The JUCE 8 API uses `Optional` return types (not raw values like the deprecated `CurrentPositionInfo`):

| Method | Return Type | Fallback Strategy |
|--------|-------------|-------------------|
| `getPpqPosition()` | `Optional<double>` | Use `orFallback(manualPpq)` |
| `getBpm()` | `Optional<double>` | Use `orFallback(120.0)` |
| `getIsPlaying()` | `bool` (not Optional) | Always available |
| `getTimeInSamples()` | `Optional<int64_t>` | Rarely needed for our use case |
| `getIsLooping()` | `bool` (not Optional) | Use to detect loop-point jumps |
| `getLoopPoints()` | `Optional<LoopPoints>` | For detecting loop boundaries |

**CRITICAL:** `getPosition()` returns position "at the start of the current processing block" (line 578 of header). It does NOT update per-sample. You MUST interpolate within the block using `ppqPerSample = bpm / (60.0 * sampleRate)`.

**CRITICAL:** Not all hosts provide all fields. `getPpqPosition()` may return nullopt if the transport is stopped or the host doesn't support it. Always use `orFallback()` or `hasValue()` checks.

**PPQ per-sample accuracy:** PPQ is only reported at block boundaries. At 512-sample blocks @ 44.1kHz, that's ~11.6ms resolution without interpolation. At 120 BPM, a 1/32 note is ~62.5ms, so block-level resolution is adequate for 1/32 notes. However, for sample-accurate grain placement, linear interpolation is strongly recommended and is the pattern used successfully in O-FreqPulse.

### SmoothedValue (JUCE 8.0.4)

**Source:** `/Users/taylorbrook/JUCE/modules/juce_audio_basics/utilities/juce_SmoothedValue.h`

Key API points:
- `reset(sampleRate, rampLengthInSeconds)` -- call in `prepareToPlay()` only
- `reset(numSteps)` -- alternative: set ramp length in samples directly
- `setTargetValue(newValue)` -- call at block start in `processBlock()`
- `getNextValue()` -- call once per sample in the inner loop
- `setCurrentAndTargetValue(value)` -- snap immediately (use in `prepareToPlay()`)
- `isSmoothing()` -- returns true while ramp is active
- `skip(numSamples)` -- skip ahead without processing (for when bypassed)
- Default smoothing type is `ValueSmoothingTypes::Linear` -- correct for gain/mix parameters

---

## State of the Art

| Old Approach | Current Approach (JUCE 8) | Impact |
|--------------|---------------------------|--------|
| `getCurrentPosition(CurrentPositionInfo&)` | `getPosition()` returning `Optional<PositionInfo>` | Old API is deprecated. New API has explicit Optional fields -- safer, forces null checks. |
| Raw `std::atomic<float>` for params | `APVTS::getRawParameterValue()` | Same underlying mechanism, but APVTS handles serialization, automation, thread safety. Already in use. |
| `setValue(x, force)` on SmoothedValue | `setTargetValue(x)` / `setCurrentAndTargetValue(x)` | Old `setValue` is deprecated since JUCE 7. |

**Deprecated/outdated:**
- `AudioPlayHead::getCurrentPosition()` -- deprecated, use `getPosition()` instead
- `SmoothedValue::setValue()` -- deprecated, use `setTargetValue()` / `setCurrentAndTargetValue()`

---

## PPQ Reliability Across DAWs

**Confidence: MEDIUM** (based on JUCE forum reports and O-FreqPulse testing, not exhaustive DAW testing)

| DAW | PPQ Behavior | Known Issues |
|-----|-------------|--------------|
| Logic Pro | Reliable, updates every block | Negative PPQ possible during count-in |
| Ableton Live | Generally reliable | `timeInSamples` may jump on tempo change; PPQ position itself is consistent |
| Cubase | Very reliable | Offline render may subdivide blocks to single samples for tempo automation accuracy |
| Reaper | Reliable | Slight timing discrepancies in recorded MIDI vs expected positions |
| FL Studio | Generally reliable | Transport state detection may differ from others |
| Standalone | No PlayHead available | Must implement manual PPQ counter with fallback BPM |

**Key insight from forum research:** Trust the host's PPQ value at each block start. Do NOT try to maintain your own running PPQ counter that overrides the host. Instead, use the host value as the authoritative position and only interpolate within a block. The O-FreqPulse approach (store `lastPpqPosition` for standalone fallback only) is correct.

**Tempo changes mid-block:** DAWs report tempo at block start only. A tempo change mid-block is invisible to the plugin. At typical buffer sizes (128-512 samples), this is 3-12ms of potential timing error. For granular stutter effects, this is inaudible. The Architecture doc's acceptance of "~5ms latency" confirms this is within tolerance.

---

## Memory Budget

| Component | Allocation | Size @ 44.1kHz | Size @ 96kHz |
|-----------|-----------|----------------|--------------|
| DelayBuffer (2s stereo) | `prepareToPlay()` | 2 * 88,200 * 4B = 689 KB | 2 * 192,000 * 4B = 1.5 MB |
| FreezeBuffer (2s stereo max) | `prepareToPlay()` | 689 KB | 1.5 MB |
| GrainPool (64 voices) | Static | 64 * ~40B = 2.5 KB | 2.5 KB |
| Euclidean pattern | Static | `std::array<bool,16>` = 16B | 16B |
| SmoothedValue x2 | Stack | ~24B each | ~24B each |
| **Total** | | **~1.4 MB** | **~3.0 MB** |

All allocations happen in `prepareToPlay()`. Zero allocations in `processBlock()`.

---

## Open Questions

1. **Stutter Gate parameter addition**
   - What we know: CONTEXT.md specifies adding `stutter_gate` (Bool, default false) as the 18th parameter. Current Stage 1 code has 17 parameters and does not include `stutter_gate` or the `spread` rename (still uses `texture` ID).
   - What's unclear: Whether the parameter rename (texture -> spread) and new stutter_gate param should be added as part of this DSP stage, or if it was intended to be done in Stage 1.
   - Recommendation: Add both changes in the first DSP task (Layer 1 integration) since the DSP code needs to reference these parameter IDs. Straightforward -- just add a `std::atomic<float>* stutterGateParam` and change `texture` references to `spread`.

2. **Crossfade duration for freeze**
   - What we know: ~5ms is the target. At 44.1kHz that's ~220 samples. At 96kHz that's ~480 samples.
   - Recommendation: Use a sample count computed from sample rate in `prepareToPlay()`: `crossfadeLengthSamples = (int)(sampleRate * 0.005)`. Simple linear crossfade is sufficient -- no need for equal-power curves for a freeze transition.

3. **`std::tanh` constexpr computation**
   - What we know: `std::tanh` is not `constexpr` in C++17. The scale factor `1/tanh(3)` must be computed at runtime.
   - Recommendation: Compute in a `static const` at file scope or in `prepare()`. The value is `1.0f / 0.99505475f = 1.00497f`. Could also hardcode as a literal: `constexpr float kTanhScale = 1.00497f;`

---

## Sources

### Primary (HIGH confidence)
- JUCE 8.0.4 source code at `/Users/taylorbrook/JUCE/modules/`:
  - `juce_audio_basics/audio_play_head/juce_AudioPlayHead.h` -- PositionInfo API
  - `juce_audio_basics/utilities/juce_SmoothedValue.h` -- SmoothedValue API
  - `juce_dsp/processors/juce_DelayLine.h` -- Lagrange3rd interpolation reference
  - `juce_audio_basics/buffers/juce_AudioSampleBuffer.h` -- AudioBuffer API
  - `juce_core/maths/juce_Random.h` -- Random number generator (real-time safe)
  - `juce_core/containers/juce_Optional.h` -- `orFallback()` method
- O-FreqPulse `PluginProcessor.cpp` (working PPQ pattern in this project)
- O-GrainScatter ARCHITECTURE.md (locked DSP design)
- O-GrainScatter CONTEXT.md (user decisions from discuss phase)

### Secondary (MEDIUM confidence)
- [JUCE Forum: Sample-accurate timing with processBlock](https://forum.juce.com/t/performing-a-function-with-sample-accuracy-using-processblock/55077) -- PPQ phasor approach
- [JUCE Forum: Position of host tempo change](https://forum.juce.com/t/position-of-the-host-tempo-change/9318) -- Mid-block tempo change behavior across DAWs
- [JUCE Forum: AudioPlayHead partial PositionInfo](https://forum.juce.com/t/audioplayhead-how-to-handle-partial-positioninfo/62823) -- Handling missing fields

### Tertiary (LOW confidence)
- [KVR: Lookup table benchmark](https://www.kvraudio.com/forum/viewtopic.php?t=560936) -- Cache impact of LUT vs computation
- [KVR: Fast tanh approximation](https://www.kvraudio.com/forum/viewtopic.php?t=388650) -- `std::tanh` performance benchmarks
- [KVR: Circular buffer implementation](https://www.kvraudio.com/forum/viewtopic.php?t=408611) -- Wrap-around strategies
- [DSPRelated: Lagrange Interpolation](https://www.dsprelated.com/freebooks/pasp/Lagrange_Interpolation.html) -- Mathematical reference

---

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH -- all APIs verified against JUCE 8.0.4 source on disk
- Architecture: HIGH -- patterns verified against working O-FreqPulse code and JUCE source
- Lagrange interpolation: HIGH -- cross-referenced JUCE DelayLine implementation
- PPQ handling: HIGH for basic pattern, MEDIUM for cross-DAW behavior
- Hann window approach: HIGH -- performance reasoning verified against multiple sources
- Pitfalls: MEDIUM-HIGH -- drawn from project experience and forum reports
- Feedback soft clip: HIGH -- formula locked in CONTEXT.md, performance data from KVR

**Research date:** 2026-02-07
**Valid until:** 2026-04-07 (stable -- JUCE 8.0.4 APIs won't change, DSP algorithms are timeless)
