# Phase 2.4: Polyphonic Grain Scheduler - Research

**Researched:** 2026-02-14
**Domain:** Polyphonic grain synthesis, MIDI routing, voice management, overlap-add rendering
**Confidence:** HIGH

## Summary

This research covers the implementation of a 64-voice polyphonic grain scheduler for O-TextureForge's concatenative synthesis engine. The scheduler reads from a pre-analyzed mono corpus buffer, applies pitch shifting via linear interpolation, windows each grain with a Hann envelope, and sums all active voices into the output buffer (overlap-add). Three MIDI modes (Generative Drone, Pitch-Mapped, Trigger+Modulate) route MIDI events to different voice allocation strategies.

The primary reference implementation is O-GrainScatter's `GrainPool` + `GrainScheduler` pattern, which is already proven in production in this codebase. O-TextureForge differs in that it reads from a static corpus buffer (not a live delay buffer) and uses KD-tree queries to select grain indices rather than positional offsets. The voice struct needs additional fields for grain index, MIDI note tracking, and age counter for voice stealing.

**Primary recommendation:** Model the voice pool directly after O-GrainScatter's `GrainPool` pattern (pre-allocated `std::array<GrainVoice, 64>`, round-robin allocation with oldest-steal fallback), but adapt for corpus-buffer reading with fractional-sample linear interpolation and MIDI-driven grain selection.

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Resample audio to DAW sample rate on load (not during playback)
- Convert all loaded files to mono (sum stereo channels)
- 19D descriptor space: 13 MFCCs + spectral centroid + spectral flatness + spectral flux + spectral rolloff + RMS energy + zero-crossing rate
- Fixed segmentation at load time using GRAIN_SIZE value; parameter changes after load only affect playback envelope
- 50% overlap for hop size (hop = grainSize / 2)
- Grain scheduler is self-contained in O-TextureForge (no shared module extraction)
- Generative Drone MIDI mode implemented first; Pitch-Mapped and Trigger+Modulate added in second pass within Stage 2
- KD-tree macro knob search uses direct descriptor targeting (Energy->RMS, Brightness->Centroid, Texture->Flatness)

### Claude's Discretion
- Voice pool size: 64 (from BRIEF.md, confirmed in ARCHITECTURE.md)
- Hann window envelope (from ARCHITECTURE.md)
- Linear interpolation for pitch shifting (from ARCHITECTURE.md)
- Voice stealing algorithm details

### Deferred Ideas (OUT OF SCOPE)
- LFO modulation of XY position (v2)
- Freeze/capture feature (v2)
- Path recorder (v2)
- Multiple file layering (v2)
- Factory presets per source type (v2)
</user_constraints>

## Standard Stack

### Core (JUCE 8.0.4 - already linked)

| Library/Class | Version | Purpose | Why Standard |
|---------------|---------|---------|--------------|
| `juce::MidiBuffer` + `MidiBufferIterator` | JUCE 8.0.4 | Sample-accurate MIDI event iteration | Built-in range-for, `samplePosition` field, `findNextSamplePosition()` |
| `juce::MidiMessage` | JUCE 8.0.4 | Parse note-on/off, CC, aftertouch | `isNoteOn()`, `getNoteNumber()`, `getFloatVelocity()`, `isController()`, `getControllerNumber()`, `getControllerValue()`, `isChannelPressure()`, `getChannelPressureValue()` |
| `juce::ScopedNoDenormals` | JUCE 8.0.4 | Prevent denormal CPU penalties | RAII, sets DAZ+FZ bits on MXCSR register. Used in every `processBlock` in this codebase. |
| `juce::Decibels::decibelsToGain()` | JUCE 8.0.4 | dB-to-linear gain conversion | Template function: `std::pow(10.0, decibels * 0.05)`. Default minusInfinityDb = -100. |
| `juce::FloatVectorOperations` | JUCE 8.0.4 | SIMD-accelerated buffer ops | `clear()`, `add()`, `multiply()` for vectorized output buffer operations |
| `juce::MathConstants<float>::twoPi` | JUCE 8.0.4 | Hann window formula constant | Standard constant for `0.5 * (1 - cos(twoPi * phase))` |
| `juce::AudioBuffer<float>` | JUCE 8.0.4 | Corpus buffer + output buffer access | `getSample()`, `addSample()`, `getWritePointer()` |

### Supporting (already in project)

| Library/Class | Version | Purpose | When to Use |
|---------------|---------|---------|-------------|
| `juce::Random` | JUCE 8.0.4 | Per-block random variation | Grain selection variation radius, non-repeating grain selection |
| `std::array<GrainVoice, 64>` | C++ STL | Pre-allocated voice pool | Zero allocation on audio thread |
| `std::atomic<float>*` | C++ STL | APVTS parameter reads | Already cached in PluginProcessor.h |

### Not Needed

| Instead of | Why Not |
|------------|---------|
| `juce::Synthesiser` + `SynthesiserVoice` | Too heavyweight for grain voices. O-GrainScatter uses custom pool, and grain voices need simpler lifecycle than Synthesiser's note-on/off model. Synthesiser adds vtable overhead per voice, MIDI routing overhead, and forces SynthesiserSound abstraction. Custom pool is 5x simpler. |
| `juce::LagrangeInterpolator` | Overkill for grain pitch shifting at +/-1 octave range. Linear interpolation is sufficient and matches O-GrainScatter's pattern. Can upgrade later if artifacts are audible. |
| `juce::dsp::WindowingFunction` | For pre-computed table only (background thread). Real-time envelope uses inline formula `0.5f * (1.0f - std::cos(twoPi * phase))` for per-sample Hann, matching O-GrainScatter exactly. |

## Architecture Patterns

### Recommended Source File Structure

```
Source/dsp/
  GrainVoice.h          # GrainVoice struct + GrainPool class (64 voices)
  GrainScheduler.h/.cpp # MIDI mode routing, density timer, voice spawning
```

### Pattern 1: GrainVoice Struct (Pre-allocated POD)

**What:** Plain struct with all fields needed for a single grain voice. No virtual functions, no heap allocation. 64 of these live in a `std::array`.

**When to use:** Always. Every grain voice uses this struct.

**GrainVoice fields (from analysis of O-GrainScatter + O-TextureForge requirements):**

```cpp
// Source: O-GrainScatter GrainPool.h adapted for corpus-based reading
struct GrainVoice
{
    bool active = false;              // Is this voice currently producing audio?
    int grainIndex = -1;              // Index into grainDatabase (which grain in corpus)
    int grainStartSample = 0;         // Absolute start sample in corpus buffer
    int grainLengthSamples = 0;       // Duration of this grain in samples
    float readPosition = 0.0f;        // Fractional read position within grain (for pitch shift)
    float playbackRate = 1.0f;        // Pitch ratio: 1.0 = original, 2.0 = octave up
    float gain = 1.0f;                // Per-voice gain (from velocity, etc.)
    float envelope = 0.0f;            // Current envelope value (computed per sample)
    int samplesElapsed = 0;           // How many samples since voice started (integer counter)
    int ageCounter = 0;               // Monotonically increasing age for voice stealing

    // MIDI tracking (Pitch-Mapped mode)
    int midiNote = -1;                // MIDI note that triggered this voice (-1 = none/drone)
    int midiChannel = 0;              // MIDI channel for polyphonic note-off matching
};
```

**Key differences from O-GrainScatter's GrainVoice:**
- No `positionOffset` (we read from corpus, not delay buffer)
- No `panPosition` (corpus is mono, output to both channels)
- No `reverse` flag (not needed for v1 concatenative synthesis)
- No spatial fields (no ambisonics in O-TextureForge)
- Added `grainIndex`, `grainStartSample`, `ageCounter`, `midiNote`

### Pattern 2: Voice Pool with Round-Robin + Oldest-Steal

**What:** O-GrainScatter's exact allocation algorithm adapted for O-TextureForge.

**When to use:** Every time a new grain must be triggered.

```cpp
// Source: O-GrainScatter GrainPool.h::spawnGrain() - verified in codebase
class GrainPool
{
public:
    static constexpr int MaxVoices = 64;

    int allocateVoice()
    {
        // Phase 1: Find next inactive voice (round-robin scan)
        for (int i = 0; i < MaxVoices; ++i)
        {
            int idx = (nextVoice + i) % MaxVoices;
            if (!voices[static_cast<size_t>(idx)].active)
            {
                nextVoice = (idx + 1) % MaxVoices;
                return idx;
            }
        }

        // Phase 2: All voices active - steal oldest (highest age)
        int oldestIdx = 0;
        int maxAge = 0;
        for (int i = 0; i < MaxVoices; ++i)
        {
            if (voices[static_cast<size_t>(i)].ageCounter > maxAge)
            {
                maxAge = voices[static_cast<size_t>(i)].ageCounter;
                oldestIdx = i;
            }
        }
        nextVoice = (oldestIdx + 1) % MaxVoices;
        return oldestIdx;
    }

private:
    std::array<GrainVoice, MaxVoices> voices {};
    int nextVoice = 0;
    int globalAge = 0;  // Incremented each time a voice is allocated
};
```

**Age tracking:** Increment `globalAge` each time `allocateVoice()` is called. Assign `voice.ageCounter = globalAge` when spawning. This gives O(N) oldest-voice lookup without maintaining a sorted structure.

### Pattern 3: Sample-Accurate MIDI Processing in processBlock

**What:** Interleave MIDI event handling with per-sample audio rendering so that note-on/off events take effect at their exact sample position within the block.

**When to use:** Pitch-Mapped and Trigger+Modulate modes. Drone mode ignores MIDI notes but still processes CC messages.

**Verified from JUCE 8.0.4 source (juce_MidiBuffer.h):**
- `MidiBuffer` supports range-for via `MidiBufferIterator`
- Each `MidiMessageMetadata` has `.samplePosition` (int), `.data`, `.numBytes`
- `metadata.getMessage()` constructs a `MidiMessage` for detailed parsing
- `findNextSamplePosition(int)` returns iterator to first event at or after given sample

```cpp
// Source: juce_MidiBuffer.h lines 48-70 + lines 271-285 (verified in local JUCE 8.0.4)
void processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    int numSamples = buffer.getNumSamples();

    // Read parameters atomically (same pattern as O-GrainScatter)
    int midiMode = static_cast<int>(midiModeParam->load());
    float outputGainDb = outputGainParam->load();
    // ... other params ...

    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getWritePointer(1);
    buffer.clear();

    // Collect MIDI events into a lightweight structure for sample-accurate processing
    auto midiIter = midiMessages.cbegin();
    auto midiEnd = midiMessages.cend();

    for (int sample = 0; sample < numSamples; ++sample)
    {
        // Process all MIDI events at this exact sample position
        while (midiIter != midiEnd && (*midiIter).samplePosition == sample)
        {
            auto msg = (*midiIter).getMessage();
            handleMidiMessage(msg, midiMode, sample);
            ++midiIter;
        }

        // Drone mode: check internal timer at this sample
        if (midiMode == 2) // Generative Drone
            checkDroneTimer(sample);

        // Render all active voices for this sample
        float outSample = 0.0f;
        renderAllVoices(outSample, sample);

        outL[sample] = outSample;
        outR[sample] = outSample;  // Mono corpus -> both channels
    }

    // Apply output gain (dB to linear)
    float gainLinear = juce::Decibels::decibelsToGain(outputGainDb);
    juce::FloatVectorOperations::multiply(outL, gainLinear, numSamples);
    juce::FloatVectorOperations::multiply(outR, gainLinear, numSamples);
}
```

### Pattern 4: Grain Rendering with Linear Interpolation

**What:** Read from corpus buffer at fractional sample positions using linear interpolation. The `readPosition` advances by `playbackRate` each sample (1.0 = original pitch, 2.0 = octave up, 0.5 = octave down).

**When to use:** Every active voice, every sample.

```cpp
// Source: O-GrainScatter GrainPool.h::processSample() adapted for corpus buffer
void renderVoiceSample(GrainVoice& v, const juce::AudioBuffer<float>& corpus, float& outSample)
{
    if (!v.active) return;

    // Compute envelope phase (0.0 at start, 1.0 at end)
    float phase = static_cast<float>(v.samplesElapsed) / static_cast<float>(v.grainLengthSamples);

    // Hann window envelope (identical to O-GrainScatter)
    float envelope = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));

    // Read from corpus at fractional position (linear interpolation)
    float absReadPos = static_cast<float>(v.grainStartSample) + v.readPosition;
    int idx0 = static_cast<int>(absReadPos);
    int idx1 = idx0 + 1;
    float frac = absReadPos - static_cast<float>(idx0);

    // Bounds check against corpus buffer
    int corpusLength = corpus.getNumSamples();
    if (idx0 >= 0 && idx1 < corpusLength)
    {
        float s0 = corpus.getSample(0, idx0);
        float s1 = corpus.getSample(0, idx1);
        float interpolated = s0 + frac * (s1 - s0);

        outSample += interpolated * envelope * v.gain;
    }

    // Advance read position by playback rate
    v.readPosition += v.playbackRate;
    ++v.samplesElapsed;

    // Deactivate when grain finished
    if (v.samplesElapsed >= v.grainLengthSamples)
        v.active = false;
}
```

**Pitch ratio formula (verified from ARCHITECTURE.md):**
```cpp
// C3 (MIDI 60) = original pitch (playbackRate = 1.0)
float playbackRate = std::pow(2.0f, (static_cast<float>(midiNote) - 60.0f) / 12.0f);
```

| MIDI Note | Name | Playback Rate |
|-----------|------|---------------|
| 48 | C2 | 0.5 (octave down) |
| 60 | C3 | 1.0 (original) |
| 72 | C4 | 2.0 (octave up) |
| 64 | E3 | 1.26 (major third up) |
| 67 | G3 | 1.498 (perfect fifth up) |

### Pattern 5: Hann Window with Crossfade Parameter

**What:** The CROSSFADE parameter (0-100%) controls how much of each grain's length is spent in fade-in and fade-out. At 100%, the entire grain is a full Hann window. At 0%, there's no fading (rectangular window - will cause clicks). At 50%, the first and last 25% of the grain are faded.

**When to use:** Envelope computation for every active voice per sample.

```cpp
// Pre-computed envelope table approach (build once in prepareToPlay)
static constexpr int ENVELOPE_TABLE_SIZE = 4096;
std::array<float, ENVELOPE_TABLE_SIZE> hannTable;

void buildHannTable()
{
    for (int i = 0; i < ENVELOPE_TABLE_SIZE; ++i)
    {
        float phase = static_cast<float>(i) / static_cast<float>(ENVELOPE_TABLE_SIZE);
        hannTable[static_cast<size_t>(i)] =
            0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));
    }
}

// Per-sample envelope lookup with crossfade percentage
float getEnvelope(int samplesElapsed, int grainLength, float crossfadePct)
{
    float phase = static_cast<float>(samplesElapsed) / static_cast<float>(grainLength);
    float fadeLength = crossfadePct / 100.0f;  // 0.0 to 1.0

    if (fadeLength < 0.001f)
        return 1.0f;  // No crossfade = rectangular window

    float envPhase;
    if (phase < fadeLength * 0.5f)
    {
        // Fade-in region: map [0, fadeLength/2] to [0, 0.5] of Hann
        envPhase = phase / fadeLength;
    }
    else if (phase > 1.0f - fadeLength * 0.5f)
    {
        // Fade-out region: map [1-fadeLength/2, 1] to [0.5, 1.0] of Hann
        envPhase = 1.0f - (1.0f - phase) / fadeLength;
    }
    else
    {
        return 1.0f;  // Sustain region: full amplitude
    }

    // Lookup in pre-computed Hann table
    int tableIdx = juce::jlimit(0, ENVELOPE_TABLE_SIZE - 1,
        static_cast<int>(envPhase * static_cast<float>(ENVELOPE_TABLE_SIZE)));
    return hannTable[static_cast<size_t>(tableIdx)];
}
```

**However:** O-GrainScatter uses inline computation (not a table) because `std::cos()` is fast enough for 64 voices per sample. The table optimization may be unnecessary but is free to implement. Recommend starting with inline computation to match O-GrainScatter and only optimize if profiling shows cost.

**Simple inline approach (preferred for initial implementation):**
```cpp
// Identical to O-GrainScatter GrainPool.h line 126-127
float phase = static_cast<float>(v.samplesElapsed) / static_cast<float>(v.grainLengthSamples);
float envelope = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));
```

This applies a full Hann window over the entire grain. The CROSSFADE parameter can be applied by modifying the phase mapping (fade-in/sustain/fade-out regions) as shown above.

### Pattern 6: Three MIDI Modes

#### Mode 0: Pitch-Mapped

```cpp
void handlePitchMapped(const juce::MidiMessage& msg)
{
    if (msg.isNoteOn() && msg.getFloatVelocity() > 0.0f)
    {
        int voiceIdx = grainPool.allocateVoice();
        auto& v = grainPool.getVoice(voiceIdx);

        // Velocity -> Energy descriptor bias for KD-tree query
        float velocityNorm = msg.getFloatVelocity();  // 0.0 to 1.0
        // ... build target descriptor, query KD-tree ...
        int grainIdx = queryKDTree(targetDescriptor);

        // Configure voice
        v.active = true;
        v.grainIndex = grainIdx;
        v.grainStartSample = grainDatabase[grainIdx].startSample;
        v.grainLengthSamples = grainDatabase[grainIdx].durationSamples;
        v.readPosition = 0.0f;
        v.playbackRate = std::pow(2.0f, (static_cast<float>(msg.getNoteNumber()) - 60.0f) / 12.0f);
        v.gain = velocityNorm;
        v.samplesElapsed = 0;
        v.ageCounter = ++globalAge;
        v.midiNote = msg.getNoteNumber();
        v.midiChannel = msg.getChannel();
    }
    else if (msg.isNoteOff() || (msg.isNoteOn() && msg.getVelocity() == 0))
    {
        // Find voice matching this note and release it
        // For grains, "release" means let the grain play to completion
        // OR immediately deactivate (design choice)
        // Recommendation: let grain finish naturally (grains are short, 10-500ms)
        for (auto& v : grainPool.getVoices())
        {
            if (v.active && v.midiNote == msg.getNoteNumber())
            {
                v.midiNote = -1;  // Unlink from MIDI note
                // Voice will deactivate when samplesElapsed >= grainLengthSamples
                break;  // Only release first matching voice
            }
        }
    }
}
```

#### Mode 1: Trigger + Modulate

```cpp
void handleTriggerModulate(const juce::MidiMessage& msg)
{
    if (msg.isNoteOn() && msg.getFloatVelocity() > 0.0f)
    {
        // Velocity controls variation radius
        float variationRadius = msg.getFloatVelocity() * variationParam->load();

        // Query KD-tree at current scatter cursor position + variation
        int grainIdx = queryKDTreeWithVariation(variationRadius);

        int voiceIdx = grainPool.allocateVoice();
        auto& v = grainPool.getVoice(voiceIdx);
        v.active = true;
        v.grainIndex = grainIdx;
        v.grainStartSample = grainDatabase[grainIdx].startSample;
        v.grainLengthSamples = grainDatabase[grainIdx].durationSamples;
        v.readPosition = 0.0f;
        v.playbackRate = 1.0f;  // No pitch shifting in trigger mode
        v.gain = msg.getFloatVelocity();
        v.samplesElapsed = 0;
        v.ageCounter = ++globalAge;
        v.midiNote = -1;  // No note tracking needed
    }
    else if (msg.isController())
    {
        if (msg.getControllerNumber() == 1)  // Mod wheel -> Scatter X
        {
            float value = static_cast<float>(msg.getControllerValue()) / 127.0f;
            // Update SCATTER_X parameter (or internal state)
            scatterXOverride = value;
        }
    }
    else if (msg.isChannelPressure())  // Aftertouch -> Scatter Y
    {
        float value = static_cast<float>(msg.getChannelPressureValue()) / 127.0f;
        scatterYOverride = value;
    }
}
```

#### Mode 2: Generative Drone (implement FIRST per CONTEXT.md)

```cpp
// Internal timer state
int samplesUntilNextGrain = 0;

void checkDroneTimer(int currentSample)
{
    --samplesUntilNextGrain;
    if (samplesUntilNextGrain <= 0)
    {
        // Density (1-64) maps to inter-grain interval
        // Density=1 -> 1 grain/sec = sampleRate samples between grains
        // Density=64 -> 64 grains/sec = sampleRate/64 samples between grains
        int density = static_cast<int>(grainDensityParam->load());
        int intervalSamples = static_cast<int>(currentSampleRate / static_cast<double>(density));
        samplesUntilNextGrain = intervalSamples;

        // Query KD-tree at current macro knob + scatter position
        int grainIdx = queryKDTree(buildTargetDescriptor());

        int voiceIdx = grainPool.allocateVoice();
        auto& v = grainPool.getVoice(voiceIdx);
        v.active = true;
        v.grainIndex = grainIdx;
        v.grainStartSample = grainDatabase[grainIdx].startSample;
        v.grainLengthSamples = grainDatabase[grainIdx].durationSamples;
        v.readPosition = 0.0f;
        v.playbackRate = 1.0f;
        v.gain = 1.0f;
        v.samplesElapsed = 0;
        v.ageCounter = ++globalAge;
        v.midiNote = -1;
    }
}

// Drone mode still processes MIDI CC for parameter control
void handleDroneMidiCC(const juce::MidiMessage& msg)
{
    if (msg.isController())
    {
        int cc = msg.getControllerNumber();
        float value = static_cast<float>(msg.getControllerValue()) / 127.0f;

        switch (cc)
        {
            case 1:   scatterXOverride = value; break;   // Mod wheel -> X
            case 11:  scatterYOverride = value; break;   // Expression -> Y
            case 74:  /* Energy override */ break;        // Filter cutoff -> Energy
            case 71:  /* Brightness override */ break;    // Resonance -> Brightness
        }
    }
}
```

### Pattern 7: Double-Buffer Visualization Snapshot

**What:** Audio thread writes grain state to a double-buffer. GUI thread reads at 30Hz. Identical to O-GrainScatter's proven pattern.

```cpp
// Source: O-GrainScatter PluginProcessor.cpp lines 606-642 (verified in codebase)
struct VizSnapshot
{
    struct ActiveGrain
    {
        int grainIndex = -1;
        float envelope = 0.0f;
        float readPositionNorm = 0.0f;  // 0-1 within grain
    };

    int activeCount = 0;
    std::array<ActiveGrain, 64> activeGrains {};
    float cursorX = 0.0f;
    float cursorY = 0.0f;
};

// At end of processBlock, after rendering all samples:
{
    int writeIdx = vizWriteIndex.load(std::memory_order_relaxed);
    auto& snap = vizSnapshots[static_cast<size_t>(writeIdx)];
    snap.activeCount = 0;

    for (int i = 0; i < GrainPool::MaxVoices; ++i)
    {
        const auto& v = grainPool.getVoice(i);
        auto& sg = snap.activeGrains[static_cast<size_t>(i)];

        sg.grainIndex = v.active ? v.grainIndex : -1;
        if (v.active)
        {
            float phase = static_cast<float>(v.samplesElapsed)
                        / static_cast<float>(v.grainLengthSamples);
            sg.envelope = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));
            sg.readPositionNorm = phase;
            ++snap.activeCount;
        }
    }

    snap.cursorX = scatterXParam->load();
    snap.cursorY = scatterYParam->load();
    vizWriteIndex.store(1 - writeIdx, std::memory_order_release);
}

// GUI thread reads (in TimerCallback at 30Hz):
int readIdx = 1 - vizWriteIndex.load(std::memory_order_acquire);
const auto& snap = vizSnapshots[static_cast<size_t>(readIdx)];
```

### Anti-Patterns to Avoid

- **Allocating on the audio thread:** Never use `new`, `std::vector::push_back`, `std::string`, or any allocating function in processBlock. The voice pool MUST be pre-allocated.
- **Locking in processBlock:** Never use `std::mutex`, `juce::CriticalSection`, or `SpinLock` in the audio thread. Use atomic operations and lock-free data structures only.
- **Per-voice `std::cos()` with crossfade table lookup:** Do NOT pre-compute a crossfade table per grain length. Grain lengths vary. Either use inline `std::cos()` (O-GrainScatter's approach) or a fixed-size Hann lookup table indexed by normalized phase.
- **Processing MIDI after rendering:** MIDI events MUST be handled at their `samplePosition` before rendering that sample. Processing all MIDI at block start loses sample accuracy.
- **Using deprecated `MidiBuffer::Iterator`:** Use range-for (`for (const auto metadata : midiMessages)`) or `cbegin()`/`cend()` instead. The old `Iterator` class is deprecated in JUCE 8.

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| dB-to-linear conversion | Custom `pow(10, db/20)` | `juce::Decibels::decibelsToGain()` | Handles minus-infinity correctly, template-typed, consistent with rest of codebase |
| Denormal prevention | Manual FPU register manipulation | `juce::ScopedNoDenormals` at top of `processBlock` | RAII, cross-platform (SSE+NEON), used in every plugin in this project |
| MIDI message parsing | Raw byte inspection | `juce::MidiMessage::isNoteOn()`, `getNoteNumber()`, etc. | Handles running status, velocity-0-as-note-off, all edge cases |
| Buffer clearing | Manual loop | `buffer.clear()` or `juce::FloatVectorOperations::clear()` | SIMD-accelerated on all platforms |
| Buffer gain application | Manual per-sample multiply | `juce::FloatVectorOperations::multiply(dest, gain, numSamples)` | SIMD-accelerated, verified in JUCE source |
| Voice pool container | `std::vector<GrainVoice>` | `std::array<GrainVoice, 64>` | Fixed size, no heap allocation, cache-friendly |

**Key insight:** JUCE provides all utility functions needed. The custom code is grain rendering logic and MIDI mode routing only.

## Common Pitfalls

### Pitfall 1: MIDI Velocity-0 Note-On = Note-Off

**What goes wrong:** Missing note-off events because some MIDI controllers send velocity-0 note-on instead of explicit note-off.

**Why it happens:** MIDI spec allows note-on with velocity 0 as equivalent to note-off (saves bandwidth in running status).

**How to avoid:**
```cpp
// Always check both:
if (msg.isNoteOff() || (msg.isNoteOn() && msg.getVelocity() == 0))
{
    // Handle note-off
}
```
Or use `msg.isNoteOn(false)` which returns false for velocity-0 note-on (the `false` parameter = `returnTrueForVelocity0`). Verified from JUCE source: `isNoteOn(bool returnTrueForVelocity0 = false)`.

**Warning signs:** Voices accumulate and never release in Pitch-Mapped mode.

### Pitfall 2: Grain Read Position Exceeding Corpus Bounds

**What goes wrong:** When `playbackRate > 1.0`, `readPosition` advances faster than `samplesElapsed`. The fractional read position can exceed the grain boundary before `samplesElapsed` reaches `grainLengthSamples`.

**Why it happens:** `readPosition += playbackRate` where playbackRate > 1.0 means the fractional position outruns the integer counter.

**How to avoid:** Always bounds-check the absolute read position against the corpus buffer length:
```cpp
float absPos = v.grainStartSample + v.readPosition;
int idx0 = static_cast<int>(absPos);
if (idx0 < 0 || idx0 + 1 >= corpus.getNumSamples())
{
    v.active = false;
    return;
}
```
AND deactivate when readPosition exceeds grain length (whichever comes first):
```cpp
if (v.readPosition >= static_cast<float>(v.grainLengthSamples) || v.samplesElapsed >= v.grainLengthSamples)
    v.active = false;
```

**Warning signs:** Occasional clicks or garbage audio when playing notes far above C3.

### Pitfall 3: Corpus Not Loaded (Null Pointer)

**What goes wrong:** processBlock runs before any file is loaded. KD-tree and corpus buffer are null.

**Why it happens:** Plugin opens in DAW, processBlock called immediately, but user hasn't loaded a file yet.

**How to avoid:** Guard all corpus access with null checks:
```cpp
auto corpus = corpusBufferPtr.load(std::memory_order_acquire);
if (!corpus || !kdTreePtr.load(std::memory_order_acquire))
{
    buffer.clear();
    return;
}
```
This is already the pattern used in O-TextureForge's Stage 1 (`buffer.clear()` only).

**Warning signs:** Crash on plugin open before file load.

### Pitfall 4: Drone Mode Keeps Playing After File Unload

**What goes wrong:** Drone timer keeps firing and trying to access grains after corpus is replaced or cleared.

**Why it happens:** Drone timer is independent of corpus state.

**How to avoid:** Check corpus availability before spawning drone grains:
```cpp
void checkDroneTimer(int sample)
{
    if (!corpusReady) return;  // No corpus loaded
    // ... timer logic ...
}
```

### Pitfall 5: Output Gain Not Handling -inf Correctly

**What goes wrong:** When OUTPUT_GAIN is at minimum (-60 dB), `Decibels::decibelsToGain(-60)` returns ~0.001, not true silence.

**Why it happens:** The default minusInfinityDb is -100, so -60 still produces a small gain value.

**How to avoid:** If the parameter range is -60 to +12, and you want true silence at -60:
```cpp
float gainLinear = juce::Decibels::decibelsToGain(outputGainDb, -60.0f);
// This returns 0.0f when outputGainDb <= -60.0f
```
Pass the parameter's minimum as the `minusInfinityDb` argument.

### Pitfall 6: Voice Stealing Audible Clicks

**What goes wrong:** When all 64 voices are active and a new grain is triggered, stealing the oldest voice causes an abrupt cutoff (click).

**Why it happens:** The stolen voice's audio output drops to zero immediately.

**How to avoid:** Two strategies:
1. **Accept it:** 64 voices is generous. At typical density (8-16), stealing rarely occurs.
2. **Rapid fade-out:** When stealing, don't immediately deactivate. Set a flag that causes a fast 1-2ms fade-out before reuse. This adds complexity; recommend deferring unless clicks are audible in testing.

O-GrainScatter accepts immediate cutoff (no fade on steal), and it works fine in practice because grains are short and the Hann envelope tapers naturally.

## Code Examples

### Complete processBlock Flow (Drone Mode First)

```cpp
// Source: Synthesized from O-GrainScatter patterns + JUCE 8.0.4 API (verified)
void TextureForgeProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                          juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;

    auto corpus = corpusBufferPtr.load(std::memory_order_acquire);
    auto* tree = kdTreePtr.load(std::memory_order_acquire);

    if (!corpus || !tree || grainDatabase.empty())
    {
        buffer.clear();
        return;
    }

    int numSamples = buffer.getNumSamples();
    int midiMode = static_cast<int>(midiModeParam->load());
    float outputGainDb = outputGainParam->load();
    float crossfadePct = crossfadeParam->load();
    // ... read other params ...

    auto* outL = buffer.getWritePointer(0);
    auto* outR = buffer.getWritePointer(1);
    buffer.clear();

    // Sample-accurate MIDI + audio rendering
    auto midiIter = midiMessages.cbegin();
    auto midiEnd = midiMessages.cend();

    for (int i = 0; i < numSamples; ++i)
    {
        // Handle MIDI events at this sample
        while (midiIter != midiEnd && (*midiIter).samplePosition == i)
        {
            auto msg = (*midiIter).getMessage();

            switch (midiMode)
            {
                case 0: handlePitchMapped(msg); break;
                case 1: handleTriggerModulate(msg); break;
                case 2: handleDroneMidiCC(msg); break;
            }
            ++midiIter;
        }

        // Drone: internal timer
        if (midiMode == 2)
            checkDroneTimer(i);

        // Render all active grain voices
        float sample = 0.0f;
        for (auto& v : grainPool.getVoices())
        {
            if (!v.active) continue;

            float phase = static_cast<float>(v.samplesElapsed)
                        / static_cast<float>(v.grainLengthSamples);
            float envelope = 0.5f * (1.0f - std::cos(juce::MathConstants<float>::twoPi * phase));

            float absPos = static_cast<float>(v.grainStartSample) + v.readPosition;
            int idx0 = static_cast<int>(absPos);
            float frac = absPos - static_cast<float>(idx0);

            if (idx0 >= 0 && idx0 + 1 < corpus->getNumSamples())
            {
                float s0 = corpus->getSample(0, idx0);
                float s1 = corpus->getSample(0, idx0 + 1);
                sample += (s0 + frac * (s1 - s0)) * envelope * v.gain;
            }

            v.readPosition += v.playbackRate;
            ++v.samplesElapsed;

            if (v.samplesElapsed >= v.grainLengthSamples
                || v.readPosition >= static_cast<float>(v.grainLengthSamples))
                v.active = false;
        }

        outL[i] = sample;
        outR[i] = sample;
    }

    // Apply output gain
    float gainLinear = juce::Decibels::decibelsToGain(outputGainDb, -60.0f);
    juce::FloatVectorOperations::multiply(outL, gainLinear, numSamples);
    juce::FloatVectorOperations::multiply(outR, gainLinear, numSamples);

    // Update visualization snapshot (identical to O-GrainScatter pattern)
    updateVizSnapshot();
}
```

### Grain Length from GRAIN_SIZE Parameter

```cpp
// GRAIN_SIZE is in ms (10-500, default 50)
// But segmentation is fixed at load time. The GRAIN_SIZE parameter at playback
// only affects the envelope window length (how much of the pre-segmented grain to play).
// If GRAIN_SIZE param > actual grain duration, clamp to actual.
int playbackGrainLength = juce::jmin(
    static_cast<int>(grainSizeParam->load() * currentSampleRate / 1000.0),
    grainDatabase[grainIdx].durationSamples
);
```

### JUCE 8.0.4 API Verification

**`juce::Decibels::decibelsToGain()` (from juce_Decibels.h lines 53-58):**
```cpp
template <typename Type>
static Type decibelsToGain(Type decibels, Type minusInfinityDb = Type(defaultMinusInfinitydB))
{
    return decibels > minusInfinityDb ? std::pow(Type(10.0), decibels * Type(0.05))
                                      : Type();  // Returns 0 when at/below minusInfinity
}
// defaultMinusInfinitydB = -100
```

**`juce::ScopedNoDenormals` (from juce_FloatVectorOperations.h lines 230-240):**
```cpp
class ScopedNoDenormals
{
public:
    ScopedNoDenormals() noexcept;   // Saves FPU state, enables flush-to-zero
    ~ScopedNoDenormals() noexcept;  // Restores previous FPU state
private:
    intptr_t fpsr;  // Saved MXCSR/FPCR register
};
```
Place at the very top of `processBlock()`. RAII ensures denormals are disabled for the entire block scope.

**`MidiMessageMetadata` (from juce_MidiBuffer.h lines 48-71):**
```cpp
struct MidiMessageMetadata
{
    const uint8* data = nullptr;
    int numBytes = 0;
    int samplePosition = 0;  // Sample offset within the block (0 to numSamples-1)
    MidiMessage getMessage() const;  // Constructs MidiMessage from raw data
};
```

**`MidiBuffer::findNextSamplePosition(int)` (from juce_MidiBuffer.h line 285):**
Returns iterator to first event at or after the given sample position. Useful for block-splitting approach but not needed with the per-sample iteration pattern shown above.

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| `MidiBuffer::Iterator` (deprecated) | Range-for with `MidiBufferIterator` | JUCE 7+ | Cleaner syntax, same performance |
| `FloatVectorOperations::disableDenormalisedNumberSupport()` | `ScopedNoDenormals` (RAII) | JUCE 5+ | Safer, exception-safe, no manual restore |
| Manual dB formula `pow(10, db/20)` | `Decibels::decibelsToGain()` | Always available | Handles edge cases (minus infinity) |

**Note on `juce::MidiMessage::isNoteOn()` default parameter:**
In JUCE 8.0.4, `isNoteOn()` has a default parameter `returnTrueForVelocity0 = false`, meaning it returns false for velocity-0 note-on messages by default. This is the correct behavior for Pitch-Mapped mode (velocity-0 should be treated as note-off). Always use `msg.isNoteOn()` without arguments for standard behavior.

## Open Questions

1. **Grain length at playback vs analysis time:**
   - CONTEXT.md says "parameter changes after load only affect playback envelope"
   - If the user changes GRAIN_SIZE to 100ms but grains were segmented at 50ms, the playback envelope should be 50ms (clamped to actual grain duration)
   - Recommendation: Playback uses `min(grainSizeParam, grainDatabase[idx].durationSamples)` as the voice's `grainLengthSamples`

2. **Drone density vs overlap:**
   - At density=64 with 50ms grains, we get one grain every ~689 samples (at 44.1kHz). That means ~3.2 grains overlapping at any time. At density=64 with 500ms grains, ~32 overlapping grains. Well within 64-voice limit.
   - At density=1 with 50ms grains, one grain every 44100 samples = long gaps of silence between grains. This is expected behavior for sparse textures.

3. **Crossfade parameter effect on overlap-add:**
   - With CROSSFADE at 100%, the full Hann window ensures that overlapping grains sum to approximately constant amplitude (property of Hann with 50% overlap).
   - With CROSSFADE at 0%, rectangular windows will cause amplitude bumps where grains overlap. This is audibly acceptable as a creative choice.
   - Recommendation: Default 50% crossfade is good. The crossfade parameter controls the fade portion of each grain's envelope, not the overlap amount (overlap is determined by density/timer interval).

4. **CC mapping in Drone mode:**
   - CONTEXT.md doesn't specify exact CC-to-parameter mapping
   - Recommendation: CC1 (Mod Wheel) -> Scatter X, CC11 (Expression) -> Scatter Y, CC74 -> Energy, CC71 -> Brightness. These are standard MPE-adjacent mappings. Can be hardcoded for v1, made configurable in v2.

## Sources

### Primary (HIGH confidence)
- **O-GrainScatter GrainPool.h** (`/plugins/O-GrainScatter/Source/dsp/GrainPool.h`) - Full source read, verified voice pool architecture, Hann envelope, voice stealing, overlap-add rendering
- **O-GrainScatter GrainScheduler.h** (`/plugins/O-GrainScatter/Source/dsp/GrainScheduler.h`) - Density-to-interval mapping, sample-accurate spawn timing
- **O-GrainScatter PluginProcessor.cpp** (`/plugins/O-GrainScatter/Source/PluginProcessor.cpp`) - processBlock flow, viz snapshot pattern, parameter reads
- **JUCE 8.0.4 juce_MidiBuffer.h** (`/Users/taylorbrook/JUCE/modules/juce_audio_basics/midi/juce_MidiBuffer.h`) - MidiBufferIterator, MidiMessageMetadata, findNextSamplePosition, range-for pattern
- **JUCE 8.0.4 juce_Decibels.h** (`/Users/taylorbrook/JUCE/modules/juce_audio_basics/utilities/juce_Decibels.h`) - decibelsToGain implementation, minusInfinityDb default
- **JUCE 8.0.4 juce_FloatVectorOperations.h** (`/Users/taylorbrook/JUCE/modules/juce_audio_basics/buffers/juce_FloatVectorOperations.h`) - ScopedNoDenormals, disableDenormalisedNumberSupport
- **O-TextureForge ARCHITECTURE.md** (`/plugins/O-TextureForge/.planning/research/ARCHITECTURE.md`) - Voice struct spec, MIDI mode pseudocode, pitch ratio formula, crossfade algorithm
- **O-TextureForge CONTEXT.md Stage 2** (`/plugins/O-TextureForge/.planning/stages/2-dsp/CONTEXT.md`) - Locked decisions, phase ordering, descriptor dimensions

### Secondary (MEDIUM confidence)
- **O-Bells PluginProcessor.cpp** - processBlock MIDI iteration pattern with Synthesiser, confirmed range-for syntax
- **O-Marimba PluginProcessor.cpp** - MIDI note-on/off handling, velocity-0 check pattern

### Tertiary (LOW confidence)
- None. All findings verified against local source code.

## Metadata

**Confidence breakdown:**
- Standard stack: HIGH - All JUCE APIs verified against local 8.0.4 source headers
- Architecture: HIGH - Based directly on O-GrainScatter production code in this codebase
- Voice pool: HIGH - O-GrainScatter's GrainPool.h is the exact pattern, adapted for corpus reading
- MIDI processing: HIGH - JUCE MidiBuffer API verified in source, patterns confirmed across O-Bells and O-Marimba
- Pitfalls: HIGH - Based on actual JUCE source code behavior and O-GrainScatter production experience

**Research date:** 2026-02-14
**Valid until:** Indefinite (JUCE 8.0.4 API is stable, reference code is in this project)
