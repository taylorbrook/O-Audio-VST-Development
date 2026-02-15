# Overlap-Add (OLA) Crossfading Research - O-Texture

**Researched:** 2026-02-14
**Domain:** Real-time overlap-add synthesis for neural network decoder output in JUCE
**Confidence:** HIGH

## Summary

This research covers the design and implementation of an overlap-add (OLA) crossfading system for O-Texture's neural decoder output. The decoder generates 4096-sample mono audio blocks, with new blocks arriving every 2048 samples (50% overlap). Overlapping blocks are windowed with a Hann function and summed to produce seamless, click-free audio output.

The OLA system must bridge two fundamentally different timing domains: the fixed-size decoder blocks (4096 samples, triggered every 2048 samples) and the variable-size `processBlock` callbacks from the DAW host (typically 128--2048 samples, not guaranteed to divide evenly into 2048). An accumulator buffer pattern solves this cleanly.

The architecture uses a dedicated `OverlapAddProcessor` class that is completely decoupled from ANIRA's inference pipeline. The audio thread writes decoded blocks into the accumulator and reads output samples independently. Thread safety between the decoder callback and audio thread is handled via lock-free signaling (atomic flags or SPSC queues).

**Primary recommendation:** Use a linear accumulator buffer (not a ring buffer) of size `BLOCK_SIZE + HOP_SIZE` (6144 samples per channel). Each decoded block is Hann-windowed and added into the accumulator. Every `HOP_SIZE` samples, the consumed portion is shifted out. This is simpler than a ring buffer, avoids wrap-around complexity, and the `memmove` cost is negligible at these sizes.

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- Block size: 4096 samples, Hop size: 2048 samples, 50% overlap
- Hann window for crossfading
- Stereo via dual-decode latent offset (L and R decoded independently as mono, two separate OLA pipelines)
- 48kHz only (no resampler needed)
- ANIRA for async inference with lock-free queues
- Latency: ~6144 samples (~128ms at 48kHz), reported via setLatencySamples()
- Generate mode only in this stage (Transform mode deferred)

### Claude's Discretion
- Internal buffer layout (accumulator vs ring buffer)
- Fallback strategy when inference is late
- Class design and method signatures
- Thread synchronization primitives

### Deferred Ideas (OUT OF SCOPE)
- Transform mode / encoder pipeline
- Dry/wet mixing
- Variable overlap ratios (75%, 25%)
- Variable block sizes
</user_constraints>

---

## Standard Stack

### Core
| Library | Version | Purpose | Why Standard |
|---------|---------|---------|--------------|
| `juce::AudioBuffer<float>` | JUCE 8.0.4 | OLA accumulator buffer + Hann window storage | Native JUCE type, efficient memory layout, `addFrom()` and `copyFrom()` methods avoid manual pointer math |
| `std::array<float, 4096>` | C++17 | Pre-computed Hann window (compile-time size) | Zero-overhead, cache-friendly, known at compile time |
| `std::atomic<bool>` | C++17 | Lock-free signaling between inference and audio threads | Standard, no allocation, sufficient for flag-based coordination |

### Supporting
| Library | Purpose | When to Use |
|---------|---------|-------------|
| `juce::FloatVectorOperations` | SIMD-optimized multiply, add, copy | Apply Hann window to decoded block (vectorized multiply), shift accumulator |
| `juce::HeapBlock<float>` | Raw aligned memory for intermediate buffers if needed | Only if `AudioBuffer` overhead is unacceptable (unlikely) |

### Alternatives Considered
| Instead of | Could Use | Tradeoff |
|------------|-----------|----------|
| Linear accumulator + shift | Ring buffer with read/write pointers | Ring buffer avoids the `memmove` but adds wrap-around complexity for the additive overlap region; for 6144-sample buffers the shift cost is ~25 microseconds, negligible |
| `juce::AudioBuffer` for accumulator | Raw `float*` arrays | Raw arrays avoid `AudioBuffer` overhead but lose convenient `addFrom()`, `clear()`, `getWritePointer()` methods; not worth it |
| `std::atomic<bool>` for block-ready flag | SPSC lock-free queue (e.g., `juce::AbstractFifo`) | Queue is better if multiple blocks can be queued; atomic flag is simpler for single-block handoff |

---

## Architecture Patterns

### Recommended Class Structure

```
Source/
  dsp/
    OverlapAddProcessor.h      # OLA buffer management (per-channel)
    OverlapAddProcessor.cpp
    HannWindow.h               # Pre-computed Hann window (header-only)
```

### Pattern 1: Accumulator Buffer with Shift

**What:** A linear output buffer where decoded blocks are Hann-windowed and additively mixed. After each hop, the consumed samples are shifted out and the tail moves to the front.

**When to use:** Fixed block size, fixed hop size, 50% overlap -- exactly our case.

**Memory layout (per channel):**

```
Accumulator buffer: 6144 samples (BLOCK_SIZE + HOP_SIZE = 4096 + 2048)

After adding Block N at position 0:
[  Block N * Hann (4096 samples)  ][ zeros (2048) ]
|--- 4096 -------------------------|--- 2048 ------|

After shifting out HOP_SIZE samples and adding Block N+1 at position 0:
[  tail of N (2048)  +  head of N+1 (2048)  ][ tail of N+1 (2048) ][ zeros ]
|--- overlap region (summed) ------------------|--- N+1 only -------|
```

**Why this works:**
1. Block N is multiplied by the Hann window and ADDED into the accumulator at position 0
2. After outputting 2048 samples, we shift the buffer left by 2048 samples (`memmove`)
3. Clear the rightmost 2048 samples (now garbage from the shift)
4. Block N+1 is multiplied by the Hann window and ADDED into the accumulator at position 0
5. The overlap region (first 2048 samples) now contains the sum of Block N's tail (Hann fade-out) and Block N+1's head (Hann fade-in)

**Critical insight:** The Hann window at 50% overlap sums to exactly 1.0 in the overlap region. No gain correction needed.

### Pattern 2: Decoupled Read/Write with Sample Counter

**What:** The `processBlock` callback reads from the accumulator at a sample-level read pointer, independent of when decoded blocks arrive. A sample counter tracks how many samples have been consumed since the last hop boundary.

**When to use:** When `processBlock` buffer size does not divide evenly into hop size (always true in practice).

**Example:**

```cpp
// In processBlock (called with variable numSamples):
void processBlock(juce::AudioBuffer<float>& buffer, int numSamples)
{
    for (int ch = 0; ch < 2; ++ch)
    {
        auto* out = buffer.getWritePointer(ch);
        auto* accum = accumulator[ch].getReadPointer(0, readPosition);

        // Copy from accumulator to output
        std::memcpy(out, accum, sizeof(float) * numSamples);
    }

    readPosition += numSamples;

    // Check if we've consumed a full hop
    while (readPosition >= HOP_SIZE)
    {
        // Shift accumulator left by HOP_SIZE
        shiftAccumulator();
        readPosition -= HOP_SIZE;

        // Request next decoded block (triggers inference)
        requestNextBlock();

        // If a decoded block is ready, add it
        if (decodedBlockReady())
        {
            addDecodedBlock();
        }
    }
}
```

### Pattern 3: Stereo as Two Independent OLA Pipelines

**What:** L and R channels each have their own accumulator buffer and receive independently decoded mono blocks. No inter-channel coupling in the OLA stage.

**When to use:** When stereo is achieved via dual-decode with different latent vectors (our architecture).

```
Left pipeline:  z_left  --> Decoder --> Hann window --> Accumulator L --> Output L
Right pipeline: z_right --> Decoder --> Hann window --> Accumulator R --> Output R
```

### Anti-Patterns to Avoid

- **Copying the full Hann-windowed block then blending separately:** This requires storing both "current" and "previous" blocks and manually computing the blend. The accumulator approach is simpler -- just add windowed blocks and the overlap region self-blends.
- **Using the Hann window as a crossfade envelope (fade-in/fade-out separately):** While conceptually valid, this is more error-prone than the standard OLA approach where each full block gets the full Hann window applied.
- **Ring buffer for the accumulator:** Unnecessary complexity for a 6144-sample buffer. The wrap-around logic for additive overlap across the ring boundary is bug-prone. Linear buffer + shift is cleaner.
- **Allocating in processBlock:** Never allocate memory on the audio thread. All buffers must be pre-allocated in `prepareToPlay()`.

---

## Mathematical Foundation: Hann Window COLA Property

### Hann Window Definition

```
w[n] = 0.5 * (1 - cos(2 * pi * n / N))    for n = 0, 1, ..., N-1
```

This is equivalent to:

```
w[n] = sin^2(pi * n / N)
```

### Proof: 50% Overlap Sums to Unity

With hop size R = N/2, two consecutive windows overlap:

```
w[n] + w[n + N/2]
= sin^2(pi * n / N) + sin^2(pi * (n + N/2) / N)
= sin^2(pi * n / N) + sin^2(pi * n / N + pi / 2)
= sin^2(pi * n / N) + cos^2(pi * n / N)
= 1                                          (Pythagorean identity)
```

**Confidence: HIGH** -- This is a fundamental trigonometric identity. No gain correction, normalization, or scaling is needed when using Hann OLA at 50% overlap.

### Numerical Verification

```cpp
// Verify at build time or in prepareToPlay():
constexpr int N = 4096;
constexpr int R = N / 2; // hop = 2048
float maxError = 0.0f;
for (int n = 0; n < R; ++n)
{
    float sum = hannWindow[n + R] + hannWindow[n]; // overlapping portion
    maxError = std::max(maxError, std::abs(sum - 1.0f));
}
jassert(maxError < 1e-6f); // Should be ~0 (floating point precision)
```

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Vectorized multiply (Hann * block) | Manual `for` loop | `juce::FloatVectorOperations::multiply(dest, src, window, N)` | SIMD-optimized on all platforms (SSE/AVX on x86, NEON on ARM) |
| Buffer addition (accumulate) | Manual `for` loop | `juce::FloatVectorOperations::add(dest, src, N)` or `AudioBuffer::addFrom()` | SIMD-optimized, handles alignment |
| Buffer shift (memmove) | Manual copy loop | `std::memmove(dest, src, bytes)` | Compiler-optimized, handles overlapping regions correctly |
| Thread-safe block handoff | Mutex/lock | `std::atomic<bool>` or `juce::AbstractFifo` | Lock-free, real-time safe |
| Hann window computation | Runtime loop every time | Pre-compute once in constructor, store as `std::array` | Computed once, read many times |

**Key insight:** The OLA algorithm itself is simple enough to implement manually (no library needed), but the inner loops should use JUCE's SIMD-optimized vector operations for performance.

---

## Common Pitfalls

### Pitfall 1: Off-by-One in Hann Window Indexing

**What goes wrong:** Using `N` (4096) vs `N-1` in the Hann formula changes whether the window is "symmetric" (for filter design) or "periodic" (for OLA/DFT).

**Why it happens:** The symmetric Hann window uses `w[n] = 0.5 * (1 - cos(2*pi*n / (N-1)))` which does NOT sum to exactly 1.0 at 50% overlap. The periodic version uses `w[n] = 0.5 * (1 - cos(2*pi*n / N))`.

**How to avoid:** Use the periodic form (divide by N, not N-1). This is the correct form for overlap-add processing. The first and last samples of the symmetric window are both exactly 0, while the periodic window's last sample is NOT zero (it wraps periodically).

**Warning signs:** If you hear subtle amplitude modulation (pumping) at the hop rate (~23 Hz at 48kHz/2048 hop), the window may be symmetric instead of periodic.

```cpp
// CORRECT (periodic, for OLA):
for (int n = 0; n < BLOCK_SIZE; ++n)
    hannWindow[n] = 0.5f * (1.0f - std::cos(2.0f * M_PI * n / BLOCK_SIZE));

// WRONG (symmetric, for filter design):
for (int n = 0; n < BLOCK_SIZE; ++n)
    hannWindow[n] = 0.5f * (1.0f - std::cos(2.0f * M_PI * n / (BLOCK_SIZE - 1)));
```

### Pitfall 2: processBlock Buffer Size Assumptions

**What goes wrong:** Assuming `processBlock` will always be called with the buffer size from `prepareToPlay`. In reality, hosts can call with ANY buffer size, including 1 sample.

**Why it happens:** Some DAWs (Logic, Ableton) may call with smaller buffers during automation, tempo changes, or near transport boundaries.

**How to avoid:** The OLA system must work sample-by-sample. Use a read position counter that advances by `numSamples` each callback, and trigger hop operations when the counter crosses hop boundaries. Never assume `numSamples == hopSize` or that `numSamples` divides evenly into `hopSize`.

**Warning signs:** Clicks or silence at specific points during playback, especially during automation recording.

### Pitfall 3: Startup/First-Block Silence

**What goes wrong:** The OLA system needs at least one decoded block before it can output audio. If the first inference takes 1-3ms but `processBlock` is called immediately at playback start, the output buffer has nothing to read.

**Why it happens:** ANIRA inference is asynchronous. The first block may not be ready when the first `processBlock` call arrives.

**How to avoid:** Initialize the accumulator with silence (already zero). The first few callbacks will output silence until the first decoded block arrives. This is expected and covered by the reported latency (6144 samples = ~128ms). The DAW compensates for this latency.

**Warning signs:** Brief click at playback start if the first block is partially added mid-output.

### Pitfall 4: Accumulator Not Cleared After Shift

**What goes wrong:** After shifting the accumulator left by HOP_SIZE, the rightmost HOP_SIZE samples contain stale data from the previous position. If the next decoded block is ADDED (not copied) into this region, the stale data contaminates the output.

**Why it happens:** `memmove` moves data but doesn't zero the vacated region.

**How to avoid:** After every shift, explicitly clear the tail of the accumulator:

```cpp
void shiftAccumulator()
{
    for (int ch = 0; ch < numChannels; ++ch)
    {
        auto* buf = accumulator.getWritePointer(ch);
        std::memmove(buf, buf + HOP_SIZE, sizeof(float) * (ACCUM_SIZE - HOP_SIZE));
        // Clear the tail -- CRITICAL
        std::memset(buf + (ACCUM_SIZE - HOP_SIZE), 0, sizeof(float) * HOP_SIZE);
    }
}
```

### Pitfall 5: Inference Underrun (Late Block)

**What goes wrong:** If ANIRA can't complete inference before the audio thread needs the next block, the accumulator runs out of valid data and outputs silence or stale audio.

**Why it happens:** CPU load spike, other plugins competing for resources, or inference taking longer than expected (>2048 samples / 48000 Hz = 42.7ms).

**How to avoid:** Implement a fallback strategy (see detailed section below). The simplest approach: if no new block is available at hop time, repeat the last decoded block (causes a brief "freeze" effect, better than silence or clicks).

**Warning signs:** Occasional brief audio freezes or dropouts under heavy CPU load.

---

## Complete Class Design

### HannWindow.h

```cpp
// Source: Verified mathematical property (sin^2 + cos^2 = 1)
#pragma once

#include <array>
#include <cmath>

template <int BlockSize>
class HannWindow
{
public:
    HannWindow()
    {
        static constexpr float twoPi = 2.0f * static_cast<float>(M_PI);
        for (int n = 0; n < BlockSize; ++n)
            window[n] = 0.5f * (1.0f - std::cos(twoPi * static_cast<float>(n)
                                                        / static_cast<float>(BlockSize)));
    }

    const float* data() const noexcept { return window.data(); }
    float operator[](int n) const noexcept { return window[n]; }
    static constexpr int size() noexcept { return BlockSize; }

private:
    std::array<float, BlockSize> window{};
};
```

### OverlapAddProcessor.h

```cpp
#pragma once

#include <JuceHeader.h>
#include "HannWindow.h"

class OverlapAddProcessor
{
public:
    static constexpr int BLOCK_SIZE = 4096;
    static constexpr int HOP_SIZE   = 2048;
    static constexpr int ACCUM_SIZE = BLOCK_SIZE + HOP_SIZE; // 6144

    OverlapAddProcessor();

    /** Call from prepareToPlay. Allocates all buffers. */
    void prepare(int numChannels);

    /** Call from releaseResources. */
    void reset();

    /**
     * Add a decoded mono block for a specific channel.
     * The block is Hann-windowed and added into the accumulator.
     *
     * @param channel       0 = left, 1 = right
     * @param decodedBlock  pointer to BLOCK_SIZE mono samples from decoder
     */
    void addDecodedBlock(int channel, const float* decodedBlock);

    /**
     * Read samples from the accumulator into the output buffer.
     * Call from processBlock. Handles variable buffer sizes and
     * triggers hop shifts internally.
     *
     * @param output      destination buffer (one channel)
     * @param channel     which channel to read
     * @param numSamples  number of samples to read (processBlock buffer size)
     * @return            number of hop boundaries crossed (0, 1, or rarely 2+)
     */
    int readSamples(float* output, int channel, int numSamples);

    /**
     * Get the current read position within the current hop.
     * Useful for determining when to trigger the next inference.
     */
    int getReadPosition() const noexcept { return readPosition; }

    /**
     * Get how many samples remain before the next hop boundary.
     */
    int getSamplesUntilNextHop() const noexcept { return HOP_SIZE - readPosition; }

    /**
     * Get the total latency introduced by the OLA system in samples.
     * This does NOT include inference latency (ANIRA reports that separately).
     */
    static constexpr int getLatencySamples() noexcept { return BLOCK_SIZE; }

private:
    void shiftAccumulator();

    // Accumulator buffer: numChannels x ACCUM_SIZE samples
    juce::AudioBuffer<float> accumulator;

    // Temporary buffer for windowed block (avoids modifying input)
    std::array<float, BLOCK_SIZE> windowedBlock{};

    // Pre-computed Hann window
    HannWindow<BLOCK_SIZE> hannWindow;

    // Read position within current hop (0 to HOP_SIZE-1)
    int readPosition = 0;

    // Number of channels
    int channels = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(OverlapAddProcessor)
};
```

### OverlapAddProcessor.cpp (Key Methods)

```cpp
#include "OverlapAddProcessor.h"

OverlapAddProcessor::OverlapAddProcessor() {}

void OverlapAddProcessor::prepare(int numChannels)
{
    channels = numChannels;
    accumulator.setSize(numChannels, ACCUM_SIZE, false, true); // clear on alloc
    accumulator.clear();
    readPosition = 0;
}

void OverlapAddProcessor::reset()
{
    accumulator.clear();
    readPosition = 0;
}

void OverlapAddProcessor::addDecodedBlock(int channel, const float* decodedBlock)
{
    jassert(channel >= 0 && channel < channels);

    // Apply Hann window to decoded block (into temp buffer)
    juce::FloatVectorOperations::multiply(windowedBlock.data(),
                                          decodedBlock,
                                          hannWindow.data(),
                                          BLOCK_SIZE);

    // Add windowed block into accumulator starting at position 0
    // Source: AudioBuffer::addFrom(destCh, destStart, source, numSamples)
    accumulator.addFrom(channel, 0, windowedBlock.data(), BLOCK_SIZE);
}

int OverlapAddProcessor::readSamples(float* output, int channel, int numSamples)
{
    jassert(channel >= 0 && channel < channels);

    int hopsCrossed = 0;
    int samplesRemaining = numSamples;
    int outputPos = 0;

    while (samplesRemaining > 0)
    {
        // How many samples until the next hop boundary?
        int samplesUntilHop = HOP_SIZE - readPosition;
        int samplesToRead = std::min(samplesRemaining, samplesUntilHop);

        // Copy from accumulator to output
        const float* src = accumulator.getReadPointer(channel, readPosition);
        std::memcpy(output + outputPos, src, sizeof(float) * samplesToRead);

        readPosition += samplesToRead;
        outputPos += samplesToRead;
        samplesRemaining -= samplesToRead;

        // Did we cross a hop boundary?
        if (readPosition >= HOP_SIZE)
        {
            shiftAccumulator();
            readPosition = 0;
            ++hopsCrossed;
        }
    }

    return hopsCrossed;
}

void OverlapAddProcessor::shiftAccumulator()
{
    for (int ch = 0; ch < channels; ++ch)
    {
        auto* buf = accumulator.getWritePointer(ch);

        // Shift left by HOP_SIZE: move [HOP_SIZE..ACCUM_SIZE) to [0..BLOCK_SIZE)
        std::memmove(buf, buf + HOP_SIZE, sizeof(float) * (ACCUM_SIZE - HOP_SIZE));

        // Clear the tail (vacated by shift) -- CRITICAL to prevent stale data
        juce::FloatVectorOperations::clear(buf + (ACCUM_SIZE - HOP_SIZE), HOP_SIZE);
    }
}
```

### Integration with TextureProcessor::processBlock

```cpp
void TextureProcessor::processBlock(juce::AudioBuffer<float>& buffer,
                                     juce::MidiBuffer& /*midi*/)
{
    juce::ScopedNoDenormals noDenormals;
    const int numSamples = buffer.getNumSamples();

    // Read from OLA accumulators into output
    for (int ch = 0; ch < 2; ++ch)
    {
        int hopsCrossed = olaProcessor.readSamples(
            buffer.getWritePointer(ch), ch, numSamples);

        // For each hop boundary crossed, we need a new decoded block
        for (int h = 0; h < hopsCrossed; ++h)
        {
            // Check if ANIRA has a completed block ready
            if (hasDecodedBlock(ch))
            {
                const float* decoded = getDecodedBlock(ch);
                olaProcessor.addDecodedBlock(ch, decoded);
            }
            else
            {
                // FALLBACK: repeat last block (see underrun handling below)
                olaProcessor.addDecodedBlock(ch, lastDecodedBlock[ch].data());
            }
        }
    }

    // Post-processing (tilt EQ, etc.) applied to the buffer after OLA output
    // ...
}
```

---

## Inference Timing and Underrun Handling

### Normal Operation Timeline

```
Time (samples):    0        2048      4096      6144      8192
                   |---------|---------|---------|---------|
Audio output:      [  hop 0  ][  hop 1  ][  hop 2  ][  hop 3  ]

Inference starts:  ^B0       ^B1       ^B2       ^B3
Inference ready:      ~B0       ~B1       ~B2       ~B3
                   (1-3ms)   (1-3ms)   (1-3ms)   (1-3ms)

Block B0 added to accumulator at hop 0 boundary
Block B1 added to accumulator at hop 1 boundary (overlaps with B0)
```

At 48kHz, each hop is 2048/48000 = 42.7ms. Inference takes ~1-3ms. There is a comfortable ~40ms margin.

### Underrun Scenario

If inference takes >42.7ms (e.g., CPU spike, other plugins competing), the block won't be ready when the hop boundary is crossed.

### Recommended Fallback Strategy: Repeat Last Block

```cpp
// Fallback: if no new decoded block available, repeat the last one
if (!hasDecodedBlock(ch))
{
    // Re-add the last block -- causes a brief "freeze" effect
    // This sounds like a very short loop (~85ms) which is:
    // - Better than silence (jarring gap)
    // - Better than clicks (missing block boundary)
    // - Barely noticeable for texture audio (non-periodic content)
    olaProcessor.addDecodedBlock(ch, lastDecodedBlock[ch].data());
}
```

**Why repeat-last-block is the best fallback for textures:**
- Textures are stochastic -- a brief 85ms repeat is barely perceptible
- The Hann windowing smooths the transition between repeated and fresh blocks
- Silence would create an audible gap
- Interpolating between old and new blocks adds complexity with minimal benefit

### Alternative Fallback: Fade to Silence

```cpp
// More conservative fallback (use if repeat sounds bad):
if (!hasDecodedBlock(ch))
{
    if (consecutiveUnderruns++ < 3)
    {
        // First few underruns: repeat last block
        olaProcessor.addDecodedBlock(ch, lastDecodedBlock[ch].data());
    }
    else
    {
        // Extended underrun: fade to silence (don't add any block)
        // The accumulator naturally fades out via the Hann window tail
        // This produces a smooth fade-out rather than abrupt silence
    }
}
else
{
    consecutiveUnderruns = 0;
}
```

---

## Thread Safety Considerations

### Thread Boundaries

```
AUDIO THREAD (real-time, processBlock)
  |
  +-- Reads from OLA accumulator (readSamples)
  +-- Adds decoded blocks (addDecodedBlock)
  +-- Checks for ready blocks (atomic load)
  +-- Triggers next inference request (non-blocking push)
  |
  +--(lock-free queue / atomic flag)--+
                                      |
ANIRA BACKGROUND THREAD              |
  |                                   |
  +-- Runs ONNX Runtime inference     |
  +-- Writes decoded block to shared buffer
  +-- Sets atomic flag "block ready"  |
```

### Recommended Pattern: Double-Buffer Handoff

```cpp
// Shared between audio thread and ANIRA callback
struct DecodedBlockSlot
{
    // Two buffers: ANIRA writes to one while audio reads from the other
    std::array<float, 4096> buffers[2];
    std::atomic<int> writeIndex{0};  // ANIRA increments after writing
    std::atomic<bool> ready{false};  // Set by ANIRA, cleared by audio thread

    // Called by ANIRA background thread:
    float* getWriteBuffer() { return buffers[writeIndex.load() & 1].data(); }
    void markReady() { ready.store(true, std::memory_order_release); }

    // Called by audio thread:
    bool isReady() const { return ready.load(std::memory_order_acquire); }
    const float* consume()
    {
        ready.store(false, std::memory_order_release);
        int idx = writeIndex.fetch_add(1, std::memory_order_acq_rel);
        return buffers[idx & 1].data();
    }
};

// One slot per channel (L, R)
DecodedBlockSlot decodedSlots[2];
```

**Important:** The `OverlapAddProcessor` itself does NOT need to be thread-safe. It is only accessed from the audio thread. Thread safety is handled at the handoff point between ANIRA and the audio thread.

### ANIRA Integration Note

ANIRA's `InferenceHandler::process()` operates in-place on audio data passed to it. However, for O-Texture we are NOT using ANIRA's process() in the standard way (audio-in, audio-out). Instead:

1. We push a latent vector (32 floats) to ANIRA as "input"
2. ANIRA runs the decoder ONNX model
3. We receive 4096 audio samples as "output"

This means we use ANIRA's **push/pop API** (decoupled mode), not the in-place `process()` method. The push/pop API is designed for exactly this kind of decoupled inference where input and output have different sizes and semantics.

```cpp
// Push latent vector for inference:
inference_handler.push_data(&latent_data, latent_size);

// Later, pop decoded audio (non-blocking):
if (inference_handler.get_available_samples(tensor_index, channel) >= BLOCK_SIZE)
{
    inference_handler.pop_data(&decoded_data, BLOCK_SIZE);
}
```

**Confidence: MEDIUM** -- The push/pop API exists in ANIRA v2.0.3 based on documentation, but exact signatures and behavior with non-audio tensor shapes (32-float latent input -> 4096-float audio output) need validation during implementation.

---

## JUCE-Specific Implementation Details

### Buffer Allocation in prepareToPlay

```cpp
void TextureProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;

    // OLA system: 2 channels (stereo)
    olaProcessor.prepare(2);

    // Pre-allocate last-block storage for fallback
    for (int ch = 0; ch < 2; ++ch)
        lastDecodedBlock[ch].fill(0.0f);

    // Report total latency: OLA buffer + inference pipeline
    // BLOCK_SIZE (4096) for OLA accumulation + HOP_SIZE (2048) for overlap
    setLatencySamples(6144);
}
```

### Efficient Sample Copying with AudioBuffer

```cpp
// Source: JUCE docs - AudioBuffer::addFrom
// https://docs.juce.com/master/classAudioBuffer.html

// Adding windowed block to accumulator (uses SIMD internally):
accumulator.addFrom(channel,              // destination channel
                    0,                     // destination start sample
                    windowedBlock.data(),  // source pointer
                    BLOCK_SIZE);           // num samples

// Copying from accumulator to output:
buffer.copyFrom(channel,                  // destination channel
                0,                        // destination start sample
                accumulator,              // source AudioBuffer
                channel,                  // source channel
                readPosition,             // source start sample
                numSamples);              // num samples

// Alternative using raw pointers (equivalent, sometimes clearer):
const float* src = accumulator.getReadPointer(channel, readPosition);
float* dst = buffer.getWritePointer(channel);
juce::FloatVectorOperations::copy(dst, src, numSamples);
```

### FloatVectorOperations for SIMD Performance

```cpp
// Source: JUCE docs - FloatVectorOperations
// Multiply block by Hann window (SIMD-accelerated):
juce::FloatVectorOperations::multiply(windowedBlock.data(),  // dest
                                       decodedBlock,          // src1
                                       hannWindow.data(),     // src2
                                       BLOCK_SIZE);           // count

// Add windowed block into accumulator (SIMD-accelerated):
juce::FloatVectorOperations::add(accumulator.getWritePointer(ch),
                                  windowedBlock.data(),
                                  BLOCK_SIZE);

// Clear tail after shift:
juce::FloatVectorOperations::clear(accumulator.getWritePointer(ch) + offset,
                                    HOP_SIZE);
```

---

## State of the Art

| Old Approach | Current Approach | When Changed | Impact |
|--------------|------------------|--------------|--------|
| Manual crossfade (separate fade-in/fade-out) | Accumulator-based OLA with full Hann window per block | Always been standard in DSP, but often implemented wrong in plugin code | Simpler, mathematically correct, no gain artifacts |
| Ring buffer for OLA | Linear accumulator + shift | N/A (both valid) | Linear is simpler for fixed block/hop sizes; ring buffer better for variable or very large buffers |
| Blocking inference on audio thread | ANIRA decoupled inference with lock-free queues | 2024 (ANIRA v1.0) | Eliminates real-time violations from ONNX Runtime |
| JUCE `AudioBuffer::applyGain()` for windowing | `FloatVectorOperations::multiply()` for element-wise | Always available | More explicit, avoids confusion with uniform gain |

---

## Latency Analysis

| Component | Samples | Time (48kHz) | Notes |
|-----------|---------|--------------|-------|
| OLA accumulation | 4096 | 85.3ms | Need one full block before first output |
| Overlap buffering | 2048 | 42.7ms | Half-block overlap region |
| Total reported | 6144 | 128ms | `setLatencySamples(6144)` in `prepareToPlay()` |
| Inference overhead | ~48-144 | ~1-3ms | ANIRA background thread, NOT added to reported latency (absorbed by OLA buffer) |

**Key insight:** The inference latency (1-3ms) is MUCH less than the hop period (42.7ms), so inference always completes well before the next block is needed. The 6144-sample reported latency is structural (OLA design), not inference-limited.

---

## Open Questions

1. **ANIRA push/pop API with non-standard tensor shapes**
   - What we know: ANIRA v2.0.3 has push_data/pop_data methods for decoupled inference
   - What's unclear: Exact behavior when input is 32 floats (latent vector) and output is 4096 floats (audio block). Are these treated as separate tensor indices? Does ANIRA's internal buffering handle the size mismatch?
   - Recommendation: Test during implementation. If push/pop doesn't work for this use case, wrap ONNX Runtime inference manually in a background thread and use a simple SPSC queue for block handoff.

2. **Dual decoder inference timing**
   - What we know: L and R channels each need a decoder inference per hop (2 inferences per 42.7ms)
   - What's unclear: Can ANIRA's thread pool handle 2 concurrent decoder inferences within the hop budget? (2 x 1-3ms = 2-6ms, should be fine with 2 threads)
   - Recommendation: Profile during implementation. ANIRA's thread pool (2 threads) should handle this easily.

3. **processBlock called before first block ready**
   - What we know: First few processBlock calls will output silence (accumulator is zeroed)
   - What's unclear: Exact number of silent callbacks depends on host buffer size and inference startup time
   - Recommendation: This is handled by the 6144-sample reported latency. The DAW compensates. No special handling needed.

---

## Sources

### Primary (HIGH confidence)
- JUCE AudioBuffer API -- [JUCE Docs: AudioBuffer](https://docs.juce.com/master/classAudioBuffer.html) -- method signatures for addFrom, copyFrom, getWritePointer, etc.
- CCRMA Overlap-Add -- [CCRMA: OLA STFT Processing](https://ccrma.stanford.edu/~jos/sasp/Overlap_Add_OLA_STFT_Processing.html) -- COLA constraint definition and OLA framework
- Hann window COLA property -- Mathematical proof via Pythagorean identity (sin^2 + cos^2 = 1), verified against [CCRMA: COLA Constraint](https://ccrma.stanford.edu/~jos/OLA/Constant_Overlap_Add_COLA_Constraint.html)
- O-Texture ARCHITECTURE.md -- Project-specific block size, hop size, latency, and thread model specifications

### Secondary (MEDIUM confidence)
- ANIRA API -- [ANIRA Documentation](https://anira-project.github.io/anira/) -- InferenceHandler process/push/pop methods, prepare() configuration
- ANIRA GitHub -- [anira-project/anira](https://github.com/anira-project/anira) -- JUCE plugin example structure, thread pool design
- JUCE Forum OLA discussions -- [Smooth or overlap-add between frames](https://forum.juce.com/t/smooth-or-overlap-add-between-frames/39453) -- community patterns for OLA in JUCE plugins

### Tertiary (LOW confidence)
- ANIRA push/pop API with non-standard tensor shapes -- inferred from documentation, not verified with actual code for latent-vector-to-audio-block use case

---

## Metadata

**Confidence breakdown:**
- OLA algorithm and Hann COLA property: **HIGH** -- fundamental DSP, mathematically proven
- Accumulator buffer design: **HIGH** -- standard pattern, straightforward C++
- JUCE AudioBuffer integration: **HIGH** -- API verified against official JUCE docs
- Thread safety / ANIRA integration: **MEDIUM** -- ANIRA API documented but push/pop for non-standard tensor shapes not verified
- Fallback strategy: **MEDIUM** -- repeat-last-block is reasonable engineering judgment, not verified with listening tests

**Research date:** 2026-02-14
**Valid until:** No expiration (fundamental DSP algorithms, stable JUCE API)
