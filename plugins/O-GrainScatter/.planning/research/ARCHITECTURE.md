# O-GrainScatter DSP Architecture

## Overview

O-GrainScatter is a granular stutter engine that bridges rhythmic stutter effects and ambient granular textures. It captures incoming audio into a circular delay buffer and re-triggers it as grains — either beat-synchronized (stutter) or density-based (cloud). A Texture morph control provides seamless transition between the two paradigms.

**Core Innovation:** "Harmonic Stutter" — 64-voice granular synthesis with musical scale quantization, beat-synchronized triggering, and density-based texture morphing in one effect.

---

## Signal Flow

```
                                    ┌─────────────────┐
                                    │   TempoTracker   │
                                    │   (PPQ-based)    │
                                    └────────┬────────┘
                                             │
                                             ▼
┌─────────┐    ┌──────────────┐    ┌─────────────────┐    ┌────────────────┐
│  Input   │──▶│ DelayBuffer  │──▶│ GrainScheduler  │──▶│  GrainPool     │
│ (stereo) │   │ (2s circular)│    │ (Free or Sync)  │    │ (64 voices)    │
└─────────┘    └──────────────┘    └────────┬────────┘    └───────┬────────┘
                      ▲                     │                     │
                      │              ┌──────┴──────┐              │
                      │              │             │              ▼
                      │     ┌────────┴───┐  ┌─────┴─────┐  ┌──────────────┐
                      │     │EuclideanGen│  │FreezeState │  │ScaleQuantizer│
                      │     └────────────┘  └───────────┘  │+ PitchLadder │
                      │                                     └──────┬───────┘
                      │                                            │
                      │         ┌──────────┐                       ▼
                      └─────────│ Feedback │◀──────────────┌──────────────┐
                                └──────────┘               │  Dry/Wet Mix │──▶ Output
                                                           └──────────────┘
```

### Per-Sample Flow

1. **Input** written into circular DelayBuffer (stereo, 2 seconds)
2. **GrainScheduler** decides when to spawn grains:
   - Free mode: density-based interval timer
   - Sync mode: PPQ subdivision crossing + Euclidean gate + probability gate
3. **GrainPool** processes all active voices:
   - Each grain reads from DelayBuffer (or FreezeState buffer if frozen)
   - Lagrange 3rd-order interpolation for sub-sample accuracy
   - Hann window envelope applied per grain
   - Pitch determined by ScaleQuantizer + PitchLadder mode
   - Pan position applied per grain (random spread)
   - Reverse grains read backwards
4. **Texture morph** controls grain spawn position spread + active voice count
5. **Feedback** mixes grain output back into DelayBuffer input
6. **Dry/Wet** blends processed grain sum with original input

---

## Class Architecture

### 1. `GrainScatterProcessor` (PluginProcessor)

**Responsibility:** Top-level JUCE AudioProcessor. Owns all DSP components, manages APVTS, orchestrates processBlock.

```
class GrainScatterProcessor : public juce::AudioProcessor
{
    juce::AudioProcessorValueTreeState apvts;

    DelayBuffer delayBuffer;
    GrainPool grainPool;
    GrainScheduler scheduler;
    TempoTracker tempoTracker;
    FreezeManager freezeManager;
    ScaleQuantizer scaleQuantizer;

    // Smoothed parameters (updated per block)
    juce::SmoothedValue<float> dryWetSmoothed;
    juce::SmoothedValue<float> feedbackSmoothed;
};
```

**Key Methods:**
- `prepareToPlay()` — Initialize all components with sample rate/block size
- `processBlock()` — Per-block orchestration (see Processing Loop below)
- `createParameterLayout()` — Define all ~20 APVTS parameters

### 2. `DelayBuffer`

**Responsibility:** Circular stereo buffer for capturing incoming audio. Provides interpolated reads at arbitrary positions.

```
class DelayBuffer
{
public:
    void prepare(double sampleRate, int maxDelaySamples);  // 2s @ sampleRate
    void pushSample(float leftIn, float rightIn);
    float readSample(int channel, float delaySamples) const;  // Lagrange3rd interpolated
    void copyRegion(juce::AudioBuffer<float>& dest, int startOffset, int lengthSamples) const;

private:
    juce::AudioBuffer<float> buffer;  // [2][sampleRate * 2]
    int writePosition = 0;
    int bufferSize = 0;
};
```

**JUCE API:** Manual circular buffer (not juce::dsp::DelayLine — we need direct position access for grain reads at arbitrary offsets, not just fixed delay taps).

**Interpolation:** Lagrange 3rd-order for sub-sample grain read positions:
```cpp
// 4-point Lagrange interpolation
float lagrange3(float y0, float y1, float y2, float y3, float frac)
{
    float c0 = y1;
    float c1 = y2 - (1.0f/3.0f)*y0 - 0.5f*y1 - (1.0f/6.0f)*y3;
    float c2 = 0.5f*(y0 + y2) - y1;
    float c3 = (1.0f/6.0f)*(y3 - y0) + 0.5f*(y1 - y2);
    return ((c3*frac + c2)*frac + c1)*frac + c0;
}
```

### 3. `GrainVoice` (struct)

**Responsibility:** State for a single grain. No methods — pure data, processed by GrainPool.

```
struct GrainVoice
{
    bool active = false;
    float readPosition = 0.0f;       // Position in delay buffer (samples from write head)
    float playbackRate = 1.0f;       // Pitch ratio (from scale quantizer)
    float panPosition = 0.5f;        // 0=left, 1=right
    int samplesRemaining = 0;        // Countdown to grain end
    int grainLengthSamples = 0;      // Total grain length (for window calc)
    bool reverse = false;            // Read direction
    bool readFromFrozen = false;     // True if grain was spawned during freeze
};
```

### 4. `GrainPool`

**Responsibility:** Manages 64 pre-allocated GrainVoice structs. Processes all active voices per sample. Handles voice stealing (oldest-first).

```
class GrainPool
{
public:
    static constexpr int MAX_VOICES = 64;

    void prepare(double sampleRate);
    void spawnGrain(const GrainParams& params);
    void processSample(const DelayBuffer& delay, const FreezeManager& freeze,
                       float& outL, float& outR);
    int getActiveCount() const;

private:
    std::array<GrainVoice, MAX_VOICES> voices;
    int nextVoiceIndex = 0;  // Round-robin allocation

    float hannWindow(float phase) const;  // phase 0..1
};
```

**Hann Window:**
```cpp
float hannWindow(float phase) const
{
    return 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));
}
```

**Voice Allocation:** Round-robin with oldest-steal. `nextVoiceIndex` advances on each spawn. If all voices active, overwrites oldest (round-robin position).

### 5. `GrainScheduler`

**Responsibility:** Decides when to spawn grains. Two modes: Free (density timer) and Sync (beat-aligned with Euclidean gating).

```
class GrainScheduler
{
public:
    struct SpawnRequest
    {
        int sampleOffset;  // Sample position within current block
        // All other grain params come from processor state
    };

    void prepare(double sampleRate);
    void processBlock(int numSamples,
                      const TempoTracker::SyncInfo& sync,
                      int syncModeIndex,
                      float density,
                      float probability,
                      const std::vector<bool>& euclideanPattern,
                      std::vector<SpawnRequest>& outRequests);

private:
    // Free mode state
    double samplesUntilNextGrain = 0.0;

    // Sync mode state
    double lastPpqPosition = 0.0;
    int euclideanStep = 0;

    juce::Random rng;
};
```

**Free Mode:** Convert density (1-100%) to inter-grain interval. At 100%, one grain per ~10ms. At 1%, one grain per ~1000ms. Timer counts down per sample.

**Sync Mode:** For each sample in the block, check if PPQ position crossed a subdivision boundary. If yes, check Euclidean pattern gate, then probability gate. If both pass, emit SpawnRequest.

### 6. `TempoTracker`

**Responsibility:** Reads DAW transport via AudioPlayHead. Provides PPQ position, BPM, playing state. Falls back to manual tracking in standalone.

```
class TempoTracker
{
public:
    struct SyncInfo
    {
        double bpm = 120.0;
        double ppqPosition = 0.0;
        double ppqPerSample = 0.0;  // Pre-computed for per-sample PPQ calc
        bool isPlaying = false;
    };

    void prepare(double sampleRate);
    SyncInfo update(juce::AudioPlayHead* playHead, int numSamples);

private:
    double sampleRate = 44100.0;
    double manualPpq = 0.0;     // Fallback PPQ counter
    double lastBpm = 120.0;
};
```

**Subdivision values (in quarter notes):**
| Index | Label | PPQ Value |
|-------|-------|-----------|
| 0 | Free | N/A |
| 1 | 1/4 | 1.0 |
| 2 | 1/8 | 0.5 |
| 3 | 1/16 | 0.25 |
| 4 | 1/32 | 0.125 |
| 5 | 1/8T | 1.0/3.0 |
| 6 | 1/16T | 1.0/6.0 |

**Crossing detection:**
```cpp
bool crossedSubdivision(double oldPpq, double newPpq, double subdivPpq)
{
    return std::floor(newPpq / subdivPpq) > std::floor(oldPpq / subdivPpq);
}
```

### 7. `FreezeManager`

**Responsibility:** Captures a snapshot of the delay buffer on freeze engage. Provides read access to frozen audio. Handles smooth engage/release.

```
class FreezeManager
{
public:
    void prepare(double sampleRate, int maxLengthSamples);
    void engage(const DelayBuffer& source, int grainSizeSamples);
    void release();
    bool isActive() const;
    float readSample(int channel, float position) const;  // Lagrange3rd interpolated
    int getCaptureLength() const;

private:
    juce::AudioBuffer<float> frozenBuffer;  // Pre-allocated at prepare()
    int captureLength = 0;
    bool active = false;
};
```

**Capture Strategy:** On engage, copy `4 * grainSizeSamples` from the delay buffer (looking backward from write head). This gives enough material for varied grain positions. Pre-allocate frozenBuffer at `prepare()` to max size (2s) — engage just sets captureLength, no allocation.

### 8. `ScaleQuantizer`

**Responsibility:** Maps arbitrary pitch offsets to nearest scale degree. Supports 5 scales. Handles PitchLadder sequencing modes.

```
class ScaleQuantizer
{
public:
    enum class Scale { Chromatic, Major, Minor, Pentatonic, WholeTone };
    enum class PitchMode { Random, LadderUp, LadderDown, Pendulum };

    float getNextPitch(PitchMode mode, Scale scale, int rootNote,
                       float randomAmount, juce::Random& rng);
    void resetLadder();

private:
    int ladderPosition = 0;
    bool ladderDirection = true;  // true=up

    static const std::array<std::vector<int>, 5> scaleIntervals;
    int quantizeToScale(int semitones, Scale scale) const;
};
```

**Scale Intervals (semitones from root):**
| Scale | Intervals |
|-------|-----------|
| Chromatic | 0,1,2,3,4,5,6,7,8,9,10,11 |
| Major | 0,2,4,5,7,9,11 |
| Minor | 0,2,3,5,7,8,10 |
| Pentatonic | 0,2,4,7,9 |
| Whole Tone | 0,2,4,6,8,10 |

**Pitch Mode Logic:**
- **Random:** Pick random semitone offset in [-12, +12], quantize to scale, scale by `randomAmount`
- **Ladder Up:** Step through scale degrees ascending, wrap at octave
- **Ladder Down:** Step through scale degrees descending, wrap at octave
- **Pendulum:** Ascend then descend, reverse at scale boundaries

**Output:** Playback rate = `pow(2.0f, semitones / 12.0f)`

### 9. `EuclideanGenerator` (free function or utility)

**Responsibility:** Generate Euclidean rhythm patterns using Bjorklund's algorithm.

```
namespace EuclideanGenerator
{
    // Returns vector of bools: true = active step
    std::vector<bool> generate(int steps, int pulses);
}
```

**Called on parameter change only** (not per-sample). Result cached in processor and passed to GrainScheduler.

**Algorithm:** Bresenham-style accumulator (equivalent to Bjorklund but simpler):
```cpp
std::vector<bool> generate(int steps, int pulses)
{
    std::vector<bool> pattern(steps, false);
    if (pulses <= 0) return pattern;
    if (pulses >= steps) { std::fill(pattern.begin(), pattern.end(), true); return pattern; }

    int bucket = 0;
    for (int i = 0; i < steps; ++i)
    {
        bucket += pulses;
        if (bucket >= steps)
        {
            bucket -= steps;
            pattern[i] = true;
        }
    }
    return pattern;
}
```

---

## Parameter Specification

### APVTS Parameter Layout

| ID | Type | Range | Default | Smoothed | Description |
|----|------|-------|---------|----------|-------------|
| `grain_size` | Float | 10-500 ms | 100 | No | Grain duration |
| `density` | Float | 1-100% | 50 | No | Grain spawn rate (Free mode) |
| `pitch_random` | Float | 0-100% | 0 | No | Pitch randomization amount |
| `pan_random` | Float | 0-100% | 0 | No | Stereo spread |
| `scale` | Choice | 5 options | Chromatic | No | Scale for pitch quantization |
| `root_note` | Choice | C-B (12) | C | No | Root note for scale |
| `reverse` | Float | 0-100% | 0 | No | Reverse grain probability |
| `feedback` | Float | 0-100% | 0 | Yes | Grain→buffer feedback |
| `dry_wet` | Float | 0-100% | 50 | Yes | Mix control |
| `sync_mode` | Choice | 7 options | Free | No | Trigger timing mode |
| `probability` | Float | 0-100% | 100 | No | Beat trigger probability |
| `repeats` | Int | 1-16 | 4 | No | Repeat count in sync mode |
| `texture` | Float | 0-100% | 0 | No | Stutter↔Cloud morph |
| `pitch_mode` | Choice | 4 options | Random | No | Pitch sequencing mode |
| `freeze` | Bool | on/off | off | No | Freeze buffer toggle |
| `euclidean_pulses` | Int | 1-16 | 4 | No | Active Euclidean hits |
| `euclidean_steps` | Int | 2-16 | 8 | No | Total Euclidean steps |

**Total: 17 parameters**

**Smoothed parameters:** Only `feedback` and `dry_wet` — these are applied per-sample in the output mixing stage and need zipper-free transitions. Other parameters affect grain spawning (discrete events) and don't need smoothing.

---

## Processing Loop (processBlock)

```
processBlock(buffer, midiMessages):
    1. Read parameter values from APVTS (atomic loads)
    2. Update smoothed values (feedback, dry_wet) for block

    3. Get SyncInfo from TempoTracker
    4. If euclidean params changed → regenerate pattern

    5. If freeze toggled on → FreezeManager.engage(delayBuffer, grainSize)
       If freeze toggled off → FreezeManager.release()

    6. Get spawn requests from GrainScheduler for this block

    7. For each sample in block:
        a. Push input sample into DelayBuffer
        b. If spawn request at this sample offset:
           - Calculate grain params (size, pitch, pan, reverse, position from texture)
           - GrainPool.spawnGrain(params)
        c. GrainPool.processSample(delayBuffer, freezeManager) → wetL, wetR
        d. Apply feedback: feedbackSample = wet * feedbackAmount → add to next DelayBuffer push
        e. Mix: out = dry * input + wet * grainOutput  (using smoothed dry_wet)
```

---

## Performance Budget

| Component | Per-Sample Cost | Notes |
|-----------|----------------|-------|
| DelayBuffer write | O(1) | Single sample push |
| DelayBuffer read | O(1) per grain | 4-point Lagrange interpolation |
| GrainPool process | O(64) worst case | Iterate all voices, skip inactive |
| Hann window | O(1) per grain | Single cos() call |
| Scheduler (Free) | O(1) | Timer decrement |
| Scheduler (Sync) | O(1) | PPQ comparison |
| Feedback | O(1) | Multiply + add |
| Dry/Wet mix | O(1) | Smoothed multiply |

**Total per-sample:** ~64 Lagrange reads + 64 window calcs + mixing = well within real-time budget at 44.1kHz stereo.

**Memory:** ~700KB (2s stereo delay @ 44.1kHz = 353K samples * 2ch * 4B = 2.8MB for delay, but shared. Frozen buffer pre-allocated same size. Voice pool = 64 * ~40B = negligible).

---

## Thread Safety

- **Audio thread (processBlock):** Reads APVTS atomics, writes to DelayBuffer, processes GrainPool. No allocations. No locks.
- **Message thread (UI):** Reads APVTS atomics for display. Sends parameter changes via APVTS (lock-free).
- **Euclidean pattern:** Generated on message thread when params change, stored as `std::vector<bool>`. Audio thread reads a cached copy. Use double-buffer or atomic flag swap to avoid data race. Alternatively, since pattern is small (max 16 bools), copy to a `std::array<bool, 16>` + atomic length — audio thread reads the array.
- **Freeze engage/release:** Triggered by bool parameter change. Audio thread checks `freeze` param each block and performs engage/release within processBlock (no thread crossing needed since frozenBuffer copy happens on audio thread from delay buffer which is also on audio thread).

---

## File Structure

```
plugins/O-GrainScatter/
├── CMakeLists.txt
├── src/
│   ├── PluginProcessor.h
│   ├── PluginProcessor.cpp
│   ├── PluginEditor.h
│   ├── PluginEditor.cpp
│   ├── dsp/
│   │   ├── DelayBuffer.h
│   │   ├── GrainPool.h          // GrainVoice struct + GrainPool class
│   │   ├── GrainScheduler.h
│   │   ├── TempoTracker.h
│   │   ├── FreezeManager.h
│   │   ├── ScaleQuantizer.h
│   │   └── EuclideanGenerator.h
│   └── ui/
│       └── index.html            // WebView UI (single-file with embedded CSS/JS)
└── .planning/
    ├── BRIEF.md
    ├── REQUIREMENTS.md
    └── research/
        └── ARCHITECTURE.md       // This file
```

---

## JUCE API Mapping

| Component | JUCE APIs Used |
|-----------|---------------|
| Parameters | `AudioProcessorValueTreeState`, `AudioParameterFloat/Int/Bool/Choice` |
| Tempo | `AudioPlayHead::getPosition()`, `PositionInfo::getBpm()`, `getPpqPosition()` |
| Audio Buffer | `juce::AudioBuffer<float>` (manual circular buffer) |
| Window Math | `juce::MathConstants<float>::twoPi`, `std::cos` |
| Random | `juce::Random` (per-instance, not system random — real-time safe) |
| WebView UI | `juce::WebBrowserComponent` with `WebBrowserComponent::Options` |
| State | `getStateInformation()` / `setStateInformation()` via APVTS |
| Plugin Format | `juce_add_plugin()` with `NEEDS_WEBVIEW2 TRUE` |
