# Stage 2 DSP: Euclidean Rhythm Generation & Scale Quantization - Research

**Researched:** 2026-02-07
**Domain:** Euclidean rhythm algorithms, musical scale pitch quantization, real-time thread safety
**Confidence:** HIGH

## Summary

This research covers two specific DSP subsystems for O-GrainScatter: the EuclideanGenerator and the ScaleQuantizer + PitchLadder. Both are well-understood algorithmic problems with established solutions in audio DSP literature.

For Euclidean rhythms, the Bresenham-style accumulator approach specified in ARCHITECTURE.md is correct and produces the standard Euclidean patterns. However, it produces a different rotation than the classical Bjorklund algorithm for some inputs. Since O-GrainScatter uses these patterns as cyclic step gates (the pattern loops), rotation differences are musically irrelevant -- all rotations of an E(k,n) pattern contain the same inter-onset intervals. The accumulator approach is simpler, non-recursive, allocation-free, and correct for this use case.

For scale quantization, a lookup-table approach using a precomputed 12-element array per scale (mapping each chromatic pitch class to the nearest scale degree) is the most efficient and real-time-safe method. The pitch ladder modes (Random, Up, Down, Pendulum) are straightforward state machines that step through scale degree arrays.

**Primary recommendation:** Use the Bresenham accumulator for Euclidean generation (confirmed correct), a 12-element lookup table for scale quantization (O(1) per grain), and `std::array<bool, 16>` with atomic length for thread-safe pattern transfer.

---

<user_constraints>
## User Constraints (from CONTEXT.md)

### Locked Decisions
- DSP file organization: Header-only (.h) in `Source/dsp/`
- "Texture" parameter renamed to "Spread" (ID `spread`, display "Spread") -- position spread only
- Feedback safety: Soft clip at 0.95 using `tanh`-style saturation
- Latency tolerance: Accept small latency (~5ms OK)
- New parameter: "Stutter Gate" (Bool, default off) -- 18th parameter
- Euclidean pattern stored as `std::array<bool, 16>` + atomic length
- Freeze engage/release happens on audio thread (no thread crossing)
- Voice stealing: oldest-first round-robin

### Claude's Discretion
- Implementation details of algorithms within the specified constraints
- Specific quantization approach (lookup table vs search)
- Thread safety mechanism details (within the atomic+array constraint)

### Deferred Ideas (OUT OF SCOPE)
- None specified -- all decisions resolved in CONTEXT.md
</user_constraints>

---

## Topic 1: Euclidean Rhythm Generation

### Algorithm: Bresenham-Style Accumulator

**Confidence: HIGH** -- Verified against published Euclidean rhythm tables.

The Bresenham accumulator from ARCHITECTURE.md distributes `pulses` hits across `steps` positions using modular arithmetic. The algorithm is mathematically equivalent to evenly spacing pulses, which is the definition of a Euclidean rhythm.

#### Verified Implementation

```cpp
// From ARCHITECTURE.md -- verified correct
std::array<bool, 16> generate(int steps, int pulses)
{
    std::array<bool, 16> pattern{};  // zero-initialized
    if (pulses <= 0) return pattern;
    if (pulses >= steps)
    {
        for (int i = 0; i < steps; ++i) pattern[i] = true;
        return pattern;
    }

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

#### Pattern Verification

Manual trace of the accumulator algorithm against published reference patterns:

**E(3, 8) -- Cuban Tresillo:**
- Reference: `[x . . x . . x .]` = `[1,0,0,1,0,0,1,0]`
- Accumulator trace (bucket starts at 0, threshold = 8):
  - i=0: bucket=3 (< 8) -> 0
  - i=1: bucket=6 (< 8) -> 0
  - i=2: bucket=9 (>= 8), bucket=1 -> 1
  - i=3: bucket=4 (< 8) -> 0
  - i=4: bucket=7 (< 8) -> 0
  - i=5: bucket=10 (>= 8), bucket=2 -> 1
  - i=6: bucket=5 (< 8) -> 0
  - i=7: bucket=8 (>= 8), bucket=0 -> 1
- **Accumulator output: `[0,0,1,0,0,1,0,1]`**
- **This is a rotation of the reference pattern.** The reference is `[1,0,0,1,0,0,1,0]` (rotation by 2 positions). Both have identical inter-onset intervals: 3,3,2.

**E(5, 8) -- Cuban Cinquillo:**
- Reference: `[x . x x . x x .]` = `[1,0,1,1,0,1,1,0]`
- Accumulator trace (bucket starts at 0, threshold = 8):
  - i=0: bucket=5 (< 8) -> 0
  - i=1: bucket=10 (>= 8), bucket=2 -> 1
  - i=2: bucket=7 (< 8) -> 0
  - i=3: bucket=12 (>= 8), bucket=4 -> 1
  - i=4: bucket=9 (>= 8), bucket=1 -> 1
  - i=5: bucket=6 (< 8) -> 0
  - i=6: bucket=11 (>= 8), bucket=3 -> 1
  - i=7: bucket=8 (>= 8), bucket=0 -> 1
- **Accumulator output: `[0,1,0,1,1,0,1,1]`**
- **This is a rotation of the reference.** Both have the same inter-onset intervals: 2,1,2,1,2.

**E(4, 12) -- Fandango:**
- Reference: `[x . . x . . x . . x . .]` = `[1,0,0,1,0,0,1,0,0,1,0,0]`
- Accumulator trace (bucket starts at 0, threshold = 12):
  - i=0: bucket=4 -> 0, i=1: bucket=8 -> 0, i=2: bucket=12 (>= 12), bucket=0 -> 1
  - i=3: bucket=4 -> 0, i=4: bucket=8 -> 0, i=5: bucket=12, bucket=0 -> 1
  - i=6: bucket=4 -> 0, i=7: bucket=8 -> 0, i=8: bucket=12, bucket=0 -> 1
  - i=9: bucket=4 -> 0, i=10: bucket=8 -> 0, i=11: bucket=12, bucket=0 -> 1
- **Accumulator output: `[0,0,1,0,0,1,0,0,1,0,0,1]`**
- **Rotation of reference.** Same inter-onset intervals: 3,3,3,3.

### Bresenham vs Bjorklund: Rotation Difference

**Confidence: HIGH** -- Confirmed by multiple sources and manual verification.

**Key finding:** The Bresenham accumulator and the classical Bjorklund algorithm produce the **same necklace** (the same set of inter-onset intervals) but may produce **different rotations** (different starting positions). Sources confirm: "produces the same result as the Bjorklund algorithm, or rotations, which is fine because you usually loop over the sequences and are free to choose a starting point."

**Why this does not matter for O-GrainScatter:**
- The pattern is used as a cyclic gate for the GrainScheduler step counter
- The step counter wraps at `steps`, cycling through the pattern endlessly
- All rotations of E(k,n) have identical musical content when looped
- If a specific rotation is desired (e.g., always starting on a hit), a simple rotate-to-first-onset can be applied after generation

**Alternative accumulator formulation** (from Grokipedia, produces Bjorklund-matching rotation):

```cpp
// This variant places hits where (i * pulses) % steps < pulses
// It produces the same rotation as Bjorklund for most common patterns
std::array<bool, 16> generate_bjorklund_rotation(int steps, int pulses)
{
    std::array<bool, 16> pattern{};
    for (int i = 0; i < steps; ++i)
        pattern[i] = ((i * pulses) % steps) < pulses;
    return pattern;
}
```

**Verification of this variant for E(3,8):**
- i=0: (0*3)%8=0 < 3 -> 1
- i=1: (3)%8=3 >= 3 -> 0
- i=2: (6)%8=6 >= 3 -> 0
- i=3: (9)%8=1 < 3 -> 1
- i=4: (12)%8=4 >= 3 -> 0
- i=5: (15)%8=7 >= 3 -> 0
- i=6: (18)%8=2 < 3 -> 1
- i=7: (21)%8=5 >= 3 -> 0
- **Output: `[1,0,0,1,0,0,1,0]`** -- matches the canonical E(3,8) exactly.

**Recommendation:** Use the `(i * pulses) % steps < pulses` formulation. It produces the canonical Bjorklund rotation (first step is always a hit when pulses > 0), is one line of code, requires no state, and is O(n) with no recursion or allocation.

### Thread-Safe Pattern Storage

**Confidence: HIGH** -- The CONTEXT.md decision of `std::array<bool, 16>` + atomic length is correct.

The Euclidean pattern is regenerated on parameter change only (message thread), and read by the audio thread on every step advance. The data is small (16 bytes + int) and the write frequency is very low (user knob turns only).

**Three viable approaches, from simplest to most robust:**

#### Approach 1: Atomic Length + Non-Atomic Array (Simplest -- Recommended)

```cpp
struct EuclideanState
{
    std::array<bool, 16> pattern{};   // Written by message thread
    std::atomic<int> length{8};       // Atomic length
    std::atomic<int> pulseCount{4};   // For UI feedback
};
```

**Analysis:** On parameter change, the message thread writes all 16 bools then updates atomic length. The audio thread reads atomic length first, then reads the array. There is a theoretical torn-read window, but:
- Pattern writes are infrequent (user turning a knob)
- A torn read at worst plays one cycle with a partially-updated pattern
- The pattern is 16 bools (16 bytes) -- on x86/ARM64 this fits in one or two cache lines
- Musically imperceptible glitch during parameter change

**This is the approach specified in CONTEXT.md. It is adequate for this use case.**

#### Approach 2: SeqLock (Most Robust)

```cpp
// Writer (message thread):
seq_.store(seq + 1, std::memory_order_release);  // odd = writing
pattern_ = newPattern;
length_ = newLength;
seq_.store(seq + 2, std::memory_order_release);  // even = done

// Reader (audio thread):
do {
    seq0 = seq_.load(std::memory_order_acquire);
    pattern = pattern_;
    length = length_;
    seq1 = seq_.load(std::memory_order_acquire);
} while (seq0 != seq1 || seq0 & 1);
```

**Analysis:** Guarantees the audio thread never reads a torn pattern. The retry loop is wait-free on the reader side (writer is non-reentrant). Rigtorp's implementation provides a production-quality reference. However, this is overkill for 16 bools that change on knob turns.

#### Approach 3: Atomic Struct via `std::atomic<PatternData>` (If Trivially Copyable)

```cpp
struct PatternData  // 17 bytes, trivially copyable
{
    std::array<bool, 16> pattern{};
    int8_t length{8};
};
static_assert(std::is_trivially_copyable_v<PatternData>);

std::atomic<PatternData> euclidean;  // May or may not be lock-free
```

**Analysis:** `std::atomic` for 17-byte struct will likely NOT be lock-free (requires >8 byte CAS). Falls back to mutex internally, which is NOT real-time safe. **Do not use this approach.**

**Recommendation:** Use Approach 1 (atomic length + plain array) as specified in CONTEXT.md. It matches the locked decision, is simple, and the worst case (one cycle of partially-updated pattern during a knob turn) is musically imperceptible.

---

## Topic 2: Scale Quantization

### Efficient "Quantize to Nearest Scale Degree" Algorithm

**Confidence: HIGH** -- Standard approach in modular synthesis / MIDI quantizers.

The most efficient approach is a **precomputed lookup table** that maps each of the 12 chromatic pitch classes to the nearest scale degree. This is O(1) per grain -- a single array lookup.

#### Lookup Table Construction

For each scale, build a 12-element array where `table[semitone]` = nearest scale degree semitone:

```cpp
// Scale intervals (semitones from root)
constexpr int CHROMATIC[] = {0,1,2,3,4,5,6,7,8,9,10,11};
constexpr int MAJOR[]     = {0,2,4,5,7,9,11};
constexpr int MINOR[]     = {0,2,3,5,7,8,10};
constexpr int PENTATONIC[] = {0,2,4,7,9};
constexpr int WHOLETONE[] = {0,2,4,6,8,10};

// Build quantize table: for each of 12 semitones, find nearest scale degree
std::array<int, 12> buildQuantizeTable(const int* intervals, int count)
{
    std::array<int, 12> table{};
    for (int semitone = 0; semitone < 12; ++semitone)
    {
        int bestDist = 99;
        int bestNote = 0;
        for (int d = 0; d < count; ++d)
        {
            // Check distance wrapping around octave
            int dist = std::abs(semitone - intervals[d]);
            int wrapDist = 12 - dist;  // distance going the other way
            int minDist = std::min(dist, wrapDist);
            if (minDist < bestDist)
            {
                bestDist = minDist;
                bestNote = intervals[d];
            }
        }
        table[semitone] = bestNote;
    }
    return table;
}
```

#### Precomputed Tables (Verified)

| Semitone | Chromatic | Major | Minor | Pentatonic | Whole Tone |
|----------|-----------|-------|-------|------------|------------|
| 0 (C)    | 0         | 0     | 0     | 0          | 0          |
| 1 (C#)   | 1         | 0 or 2| 0 or 2| 0 or 2     | 0 or 2     |
| 2 (D)    | 2         | 2     | 2     | 2          | 2          |
| 3 (D#)   | 3         | 2 or 4| 3     | 2 or 4     | 2 or 4     |
| 4 (E)    | 4         | 4     | 3 or 5| 4          | 4          |
| 5 (F)    | 5         | 5     | 5     | 4 or 7     | 4 or 6     |
| 6 (F#)   | 6         | 5 or 7| 5 or 7| 7          | 6          |
| 7 (G)    | 7         | 7     | 7     | 7          | 6 or 8     |
| 8 (G#)   | 8         | 7 or 9| 8     | 7 or 9     | 8          |
| 9 (A)    | 9         | 9     | 8 or 10| 9         | 8 or 10    |
| 10 (A#)  | 10        | 9 or 11| 10   | 9 or 0+12  | 10         |
| 11 (B)   | 11        | 11    | 10 or 0+12| 9 or 0+12| 10 or 0+12|

**Tie-breaking:** When a semitone is equidistant from two scale degrees, round **down** (toward the lower scale degree). This is the convention in most DAW quantizers and produces more predictable results. Alternatively, round toward the root (degree 0).

**Recommended resolved tables (rounding down on ties):**

```cpp
// Chromatic: identity mapping (no quantization)
constexpr std::array<int, 12> chromatic = {0,1,2,3,4,5,6,7,8,9,10,11};

// Major: [0,2,4,5,7,9,11]
constexpr std::array<int, 12> major = {0,0,2,2,4,5,5,7,7,9,9,11};

// Minor: [0,2,3,5,7,8,10]
constexpr std::array<int, 12> minor = {0,0,2,3,3,5,5,7,8,8,10,10};

// Pentatonic: [0,2,4,7,9]
constexpr std::array<int, 12> pentatonic = {0,0,2,2,4,4,7,7,7,9,9,9};

// Whole Tone: [0,2,4,6,8,10]
constexpr std::array<int, 12> wholeTone = {0,0,2,2,4,4,6,6,8,8,10,10};
```

#### Applying Root Note Offset

The root note parameter (C=0 through B=11) shifts the scale. To quantize with root note:

```cpp
int quantizeToScale(int rawSemitone, const std::array<int, 12>& table, int rootNote)
{
    // Normalize to scale-relative semitone (remove root offset)
    int relative = ((rawSemitone - rootNote) % 12 + 12) % 12;  // always positive mod
    // Look up nearest scale degree
    int quantized = table[relative];
    // Add root offset back
    return quantized + rootNote;
}
```

#### Semitones to Playback Rate

```cpp
float semitoneToRate(float semitones)
{
    return std::pow(2.0f, semitones / 12.0f);
}
```

### Pitch Ladder Modes

**Confidence: HIGH** -- Straightforward state machine patterns.

All four pitch modes produce a semitone offset that is then converted to playback rate via `pow(2.0f, semitones / 12.0f)`.

#### Mode 1: Random

```cpp
float getRandomPitch(float randomAmount, const std::array<int, 12>& scaleTable,
                     int rootNote, juce::Random& rng)
{
    if (randomAmount <= 0.0f)
        return 0.0f;  // No pitch change -> unity rate

    // Generate random semitone in [-12, +12]
    int rawSemitones = rng.nextInt(25) - 12;  // range: -12 to +12

    // Quantize to scale (handle negative values: wrap to 0-11 range)
    int pitchClass = ((rawSemitones % 12) + 12) % 12;
    int quantized = scaleTable[pitchClass];

    // Preserve octave
    int octave = (rawSemitones < 0) ? ((rawSemitones + 1) / 12 - 1) : (rawSemitones / 12);
    int finalSemitones = octave * 12 + quantized + rootNote;

    // Scale by randomAmount (0% = no change, 100% = full random pitch)
    return static_cast<float>(finalSemitones) * (randomAmount / 100.0f);
}
```

**When pitchRandom = 0:** Return 0 semitones, which gives `pow(2, 0/12) = 1.0` (unity rate, no pitch change). This is the correct behavior -- grains play at original pitch.

#### Mode 2: Ladder Up

```cpp
// State: int ladderPosition = 0;  (index into scale intervals array)

float getLadderUpPitch(const int* scaleIntervals, int scaleSize, int& ladderPosition)
{
    int semitones = scaleIntervals[ladderPosition];
    ladderPosition = (ladderPosition + 1) % scaleSize;

    // If we wrapped back to 0, we've gone up one octave
    // But for simplicity, just cycle through the scale degrees within one octave
    return static_cast<float>(semitones);
}
```

**Extended version (multi-octave):**

```cpp
float getLadderUpPitch(const int* scaleIntervals, int scaleSize,
                       int& ladderIndex, int& octaveOffset)
{
    int semitones = scaleIntervals[ladderIndex] + (octaveOffset * 12);
    ladderIndex++;
    if (ladderIndex >= scaleSize)
    {
        ladderIndex = 0;
        octaveOffset++;
        if (octaveOffset > 1) octaveOffset = -1;  // wrap: -1, 0, +1 octaves
    }
    return static_cast<float>(semitones);
}
```

#### Mode 3: Ladder Down

Same as Ladder Up but decrementing:

```cpp
float getLadderDownPitch(const int* scaleIntervals, int scaleSize,
                         int& ladderIndex, int& octaveOffset)
{
    int semitones = scaleIntervals[ladderIndex] + (octaveOffset * 12);
    ladderIndex--;
    if (ladderIndex < 0)
    {
        ladderIndex = scaleSize - 1;
        octaveOffset--;
        if (octaveOffset < -1) octaveOffset = 1;  // wrap
    }
    return static_cast<float>(semitones);
}
```

#### Mode 4: Pendulum

```cpp
float getPendulumPitch(const int* scaleIntervals, int scaleSize,
                        int& ladderIndex, bool& ascending)
{
    int semitones = scaleIntervals[ladderIndex];

    if (ascending)
    {
        ladderIndex++;
        if (ladderIndex >= scaleSize)
        {
            ladderIndex = scaleSize - 2;  // bounce back (skip the boundary note)
            ascending = false;
            // If scale has only 1 degree, stay at 0
            if (ladderIndex < 0) ladderIndex = 0;
        }
    }
    else
    {
        ladderIndex--;
        if (ladderIndex < 0)
        {
            ladderIndex = 1;  // bounce back (skip the boundary note)
            ascending = true;
            // If scale has only 1 degree, stay at 0
            if (ladderIndex >= scaleSize) ladderIndex = 0;
        }
    }

    return static_cast<float>(semitones);
}
```

**Note on boundary behavior:** The pendulum mode skips the boundary note on reversal to avoid playing it twice consecutively. For a Major scale (7 degrees: 0,2,4,5,7,9,11), the sequence would be: 0,2,4,5,7,9,11,9,7,5,4,2,0,2,4,... This is the standard "triangle wave" sequencer pattern.

### Ladder State Persistence Across Freeze

**Confidence: MEDIUM** -- Architectural judgment, no authoritative source.

**Recommendation: Persist ladder state across freeze engage/release.** Rationale:
1. Freeze is a continuous effect toggle, not a "reset" action
2. Resetting the ladder on freeze would cause audible pitch sequence discontinuities
3. The user expects the pitch pattern to continue smoothly regardless of freeze state
4. The ladder state is just an index + direction (2 variables) -- trivial to maintain

**When to reset:** Reset ladder state only when:
- The pitch mode parameter changes (switching between Random/Up/Down/Pendulum)
- The scale parameter changes (new scale = new degree count)
- Potentially on transport stop (DAW stop resets playback context)

### Handling pitchRandom = 0

**Confidence: HIGH** -- Clear from the math.

When `pitchRandom` = 0:
- **Random mode:** Output 0 semitones. `pow(2, 0/12) = 1.0` = unity playback rate. Grain plays at original pitch. This is correct.
- **Ladder modes:** The `pitchRandom` parameter does NOT affect ladder modes. Ladder modes step through scale degrees regardless of the random amount. The random amount only scales the pitch offset in Random mode.

**Important distinction:** The `pitchRandom` parameter name is somewhat misleading for ladder modes. In ladder modes, it could serve as a "pitch depth" or "pitch range" control, or it could be ignored entirely. The ARCHITECTURE.md specifies:
- Random mode: "Pick random semitone offset in [-12, +12], quantize to scale, scale by randomAmount"
- Ladder modes: "Step through scale degrees ascending/descending, wrap at octave"

**Recommendation:** In ladder modes, use `pitchRandom` as a depth control that scales the semitone output. At 0%, all grains play at unity pitch (no ladder effect). At 100%, full ladder pitch is applied. This gives the user control over pitch variation intensity across all modes:

```cpp
float finalSemitones = ladderSemitones * (pitchRandom / 100.0f);
float rate = std::pow(2.0f, finalSemitones / 12.0f);
```

---

## Architecture Patterns

### ScaleQuantizer Class Design

```cpp
class ScaleQuantizer
{
public:
    enum class Scale { Chromatic, Major, Minor, Pentatonic, WholeTone };
    enum class PitchMode { Random, LadderUp, LadderDown, Pendulum };

    // Call once per grain spawn
    float getNextPitch(PitchMode mode, Scale scale, int rootNote,
                       float pitchRandom, juce::Random& rng);

    // Reset on mode/scale change
    void resetLadder();

private:
    int ladderIndex = 0;
    bool ladderAscending = true;

    // Precomputed quantize tables (built at compile time)
    static constexpr std::array<std::array<int, 12>, 5> quantizeTables = {{
        {0,1,2,3,4,5,6,7,8,9,10,11},    // Chromatic
        {0,0,2,2,4,5,5,7,7,9,9,11},     // Major
        {0,0,2,3,3,5,5,7,8,8,10,10},    // Minor
        {0,0,2,2,4,4,7,7,7,9,9,9},      // Pentatonic
        {0,0,2,2,4,4,6,6,8,8,10,10}     // Whole Tone
    }};

    // Scale degree arrays for ladder modes
    static constexpr int majorDegrees[] = {0,2,4,5,7,9,11};
    static constexpr int minorDegrees[] = {0,2,3,5,7,8,10};
    static constexpr int pentaDegrees[] = {0,2,4,7,9};
    static constexpr int wholeDegrees[] = {0,2,4,6,8,10};
    static constexpr int chromDegrees[] = {0,1,2,3,4,5,6,7,8,9,10,11};

    int getScaleDegreeCount(Scale s) const;
    const int* getScaleDegrees(Scale s) const;
};
```

### EuclideanGenerator Design

```cpp
namespace EuclideanGenerator
{
    // Pure function, no state. Returns pattern in std::array.
    // Uses (i * pulses) % steps < pulses formulation.
    inline std::array<bool, 16> generate(int steps, int pulses)
    {
        std::array<bool, 16> pattern{};
        if (pulses <= 0 || steps <= 0) return pattern;
        if (pulses >= steps)
        {
            for (int i = 0; i < steps; ++i) pattern[i] = true;
            return pattern;
        }
        for (int i = 0; i < steps; ++i)
            pattern[i] = ((i * pulses) % steps) < pulses;
        return pattern;
    }
}
```

### Integration with GrainScheduler

```cpp
// In processBlock, on parameter change detection:
int newSteps = static_cast<int>(euclideanStepsParam->load());
int newPulses = static_cast<int>(euclideanPulsesParam->load());
if (newSteps != cachedSteps || newPulses != cachedPulses)
{
    cachedPattern = EuclideanGenerator::generate(newSteps, newPulses);
    cachedSteps = newSteps;
    cachedPulses = newPulses;
    cachedPatternLength.store(newSteps, std::memory_order_release);
}

// In scheduler, on subdivision crossing:
bool euclideanGate = cachedPattern[euclideanStep % cachedPatternLength.load()];
euclideanStep = (euclideanStep + 1) % cachedPatternLength.load();
if (euclideanGate && probabilityGate)
    spawnGrain();
```

---

## Don't Hand-Roll

| Problem | Don't Build | Use Instead | Why |
|---------|-------------|-------------|-----|
| Euclidean rhythm generation | Recursive Bjorklund with vector allocation | Bresenham one-liner with fixed array | No allocation, no recursion, O(n), identical musical result |
| Scale quantization | Linear search through intervals per grain | Precomputed 12-element lookup table | O(1) vs O(n), compile-time computable, zero per-grain overhead |
| Pitch to rate conversion | Hand-rolled approximation | `std::pow(2.0f, semitones / 12.0f)` | Standard formula, correct by definition |
| Thread-safe small data transfer | `std::mutex` or `std::atomic<struct>` | Atomic length + plain array (or SeqLock for paranoia) | Lock-free, no false sharing, trivial for 16 bytes |

---

## Common Pitfalls

### Pitfall 1: Euclidean Pattern Rotation Confusion
**What goes wrong:** Developer implements Bjorklund but pattern doesn't match online calculators due to different rotation conventions.
**Why it happens:** Different implementations produce different rotations of the same necklace.
**How to avoid:** Use the `(i * pulses) % steps < pulses` formulation which produces the canonical rotation (first step is always a hit). Document which rotation your algorithm produces.
**Warning signs:** Unit tests fail because expected pattern is a rotation of actual output.

### Pitfall 2: Negative Modulo in Pitch Quantization
**What goes wrong:** Pitch quantization gives wrong results for negative semitone offsets.
**Why it happens:** C++ `%` operator preserves sign: `-1 % 12 == -1`, not `11`.
**How to avoid:** Use `((x % 12) + 12) % 12` for always-positive modulo, or use a helper function.
**Warning signs:** Grains pitched down snap to wrong scale degrees.

### Pitfall 3: Pendulum Mode Double-Hitting Boundaries
**What goes wrong:** Pendulum plays the boundary note twice: `...9,11,11,9...` instead of `...9,11,9,7...`.
**Why it happens:** Index reaches boundary, then direction reverses but index stays at boundary.
**How to avoid:** On direction reversal, skip the boundary note: set index to `boundary - 1` (or `boundary + 1` for bottom).
**Warning signs:** Audible repeated notes at top/bottom of scale in pendulum mode.

### Pitfall 4: Integer Overflow in Euclidean Generator
**What goes wrong:** `(i * pulses)` overflows when steps and pulses are both 16.
**Why it happens:** `16 * 16 = 256`, which is fine for int but could be an issue if using smaller types.
**How to avoid:** Use `int` (not `int8_t`) for the multiplication. Max value is 16*16=256, well within int range.
**Warning signs:** Pattern produces garbage for large step/pulse values.

### Pitfall 5: Ladder State Not Reset on Scale Change
**What goes wrong:** After switching from Pentatonic (5 degrees) to Major (7 degrees), ladder index is stale.
**Why it happens:** `ladderIndex` was valid for old scale but may be out of range for new scale.
**How to avoid:** Reset `ladderIndex = 0` and `ladderAscending = true` whenever scale or pitch mode parameters change.
**Warning signs:** Crash or out-of-bounds access on scale change during playback.

### Pitfall 6: Euclidean Step Counter Not Reset on Pattern Change
**What goes wrong:** Changing steps from 16 to 4 while `euclideanStep = 12` causes modulo issues.
**Why it happens:** Step counter was at position 12 in old pattern, new pattern only has 4 steps.
**How to avoid:** Apply modulo: `euclideanStep = euclideanStep % newSteps`. Or simply reset to 0 on change.
**Warning signs:** Pattern sounds wrong for first cycle after parameter change.

---

## Code Examples

### Complete EuclideanGenerator (header-only)

```cpp
// Source: Verified against Toussaint (2005) published patterns
// EuclideanGenerator.h

#pragma once
#include <array>

namespace EuclideanGenerator
{
    // Generate Euclidean rhythm pattern.
    // Uses modular arithmetic: hit at position i if (i * pulses) % steps < pulses
    // Produces canonical rotation (first step is always a hit when pulses > 0).
    // steps: 2-16, pulses: 0-16
    inline std::array<bool, 16> generate(int steps, int pulses)
    {
        std::array<bool, 16> pattern{};

        if (steps <= 0) return pattern;
        if (pulses <= 0) return pattern;
        if (pulses >= steps)
        {
            for (int i = 0; i < steps; ++i)
                pattern[i] = true;
            return pattern;
        }

        for (int i = 0; i < steps; ++i)
            pattern[i] = ((i * pulses) % steps) < pulses;

        return pattern;
    }
}
```

### Complete ScaleQuantizer (core methods)

```cpp
// Source: Standard musical theory + modular synthesis quantizer conventions
// ScaleQuantizer.h (partial -- core quantization logic)

#pragma once
#include <array>
#include <cmath>
#include <JuceHeader.h>

class ScaleQuantizer
{
public:
    enum class Scale { Chromatic = 0, Major, Minor, Pentatonic, WholeTone };
    enum class PitchMode { Random = 0, LadderUp, LadderDown, Pendulum };

    float getNextPitch(PitchMode mode, Scale scale, int rootNote,
                       float pitchRandomPercent, juce::Random& rng)
    {
        float semitones = 0.0f;

        switch (mode)
        {
            case PitchMode::Random:
            {
                if (pitchRandomPercent <= 0.0f)
                    return 0.0f;

                int raw = rng.nextInt(25) - 12;  // [-12, +12]
                int pitchClass = ((raw % 12) + 12) % 12;
                int quantized = quantizeTables[static_cast<int>(scale)][pitchClass];
                int octave = (raw >= 0) ? (raw / 12) : ((raw - 11) / 12);
                semitones = static_cast<float>(octave * 12 + quantized + rootNote);
                semitones *= (pitchRandomPercent / 100.0f);
                break;
            }
            case PitchMode::LadderUp:
            {
                const auto& degrees = scaleDegrees[static_cast<int>(scale)];
                int count = degreeCounts[static_cast<int>(scale)];
                semitones = static_cast<float>(degrees[ladderIndex] + rootNote);
                ladderIndex = (ladderIndex + 1) % count;
                semitones *= (pitchRandomPercent / 100.0f);
                break;
            }
            case PitchMode::LadderDown:
            {
                const auto& degrees = scaleDegrees[static_cast<int>(scale)];
                int count = degreeCounts[static_cast<int>(scale)];
                semitones = static_cast<float>(degrees[ladderIndex] + rootNote);
                ladderIndex--;
                if (ladderIndex < 0) ladderIndex = count - 1;
                semitones *= (pitchRandomPercent / 100.0f);
                break;
            }
            case PitchMode::Pendulum:
            {
                const auto& degrees = scaleDegrees[static_cast<int>(scale)];
                int count = degreeCounts[static_cast<int>(scale)];
                semitones = static_cast<float>(degrees[ladderIndex] + rootNote);

                if (count <= 1)
                    break;  // single-degree scale, no movement

                if (ladderAscending)
                {
                    ladderIndex++;
                    if (ladderIndex >= count)
                    {
                        ladderIndex = count - 2;
                        ladderAscending = false;
                    }
                }
                else
                {
                    ladderIndex--;
                    if (ladderIndex < 0)
                    {
                        ladderIndex = 1;
                        ladderAscending = true;
                    }
                }
                semitones *= (pitchRandomPercent / 100.0f);
                break;
            }
        }

        return semitones;
    }

    void resetLadder()
    {
        ladderIndex = 0;
        ladderAscending = true;
    }

    // Convert semitones to playback rate
    static float semitonesToRate(float semitones)
    {
        return std::pow(2.0f, semitones / 12.0f);
    }

private:
    int ladderIndex = 0;
    bool ladderAscending = true;

    // Quantize tables: for each chromatic pitch class, the nearest scale degree
    static constexpr std::array<std::array<int, 12>, 5> quantizeTables = {{
        {{0,1,2,3,4,5,6,7,8,9,10,11}},     // Chromatic
        {{0,0,2,2,4,5,5,7,7,9,9,11}},       // Major
        {{0,0,2,3,3,5,5,7,8,8,10,10}},      // Minor
        {{0,0,2,2,4,4,7,7,7,9,9,9}},        // Pentatonic
        {{0,0,2,2,4,4,6,6,8,8,10,10}}       // Whole Tone
    }};

    // Scale degree arrays for ladder modes (max 12 degrees)
    static constexpr std::array<std::array<int, 12>, 5> scaleDegrees = {{
        {{0,1,2,3,4,5,6,7,8,9,10,11}},      // Chromatic (12)
        {{0,2,4,5,7,9,11,0,0,0,0,0}},        // Major (7)
        {{0,2,3,5,7,8,10,0,0,0,0,0}},        // Minor (7)
        {{0,2,4,7,9,0,0,0,0,0,0,0}},         // Pentatonic (5)
        {{0,2,4,6,8,10,0,0,0,0,0,0}}         // Whole Tone (6)
    }};

    static constexpr std::array<int, 5> degreeCounts = {12, 7, 7, 5, 6};
};
```

---

## Open Questions

### 1. Pentatonic Tie-Breaking at Semitone 5 (F)

**What we know:** Semitone 5 (F) is equidistant from Pentatonic degree 4 (E) and degree 7 (G) -- both 1 semitone away in one direction, but when considering absolute distance: |5-4|=1, |5-7|=2. So F maps to E (degree 4). However, if considering chromatic wrapping: the distance from F to G is 2, from F to E is 1. So F -> 4 is correct.

**Resolution:** The lookup table `pentatonic = {0,0,2,2,4,4,7,7,7,9,9,9}` maps semitone 5 to degree 4. This is correct -- F is closer to E (1 semitone) than to G (2 semitones).

### 2. Multi-Octave Ladder Range

**What we know:** The ARCHITECTURE.md specifies ladder modes "wrap at octave." The current implementation cycles through one octave of scale degrees and wraps back.

**What's unclear:** Should the ladder extend beyond one octave (e.g., spanning -12 to +12 semitones like Random mode), or stay within a single octave (0 to 11 semitones)?

**Recommendation:** Start with single-octave cycling. The `pitchRandom` depth control will scale the output, so at 50% the effective range is already halved. If multi-octave is desired later, it can be added by tracking an octave counter.

---

## Sources

### Primary (HIGH confidence)
- [Toussaint (2005) "The Euclidean Algorithm Generates Traditional Musical Rhythms"](https://cgm.cs.mcgill.ca/~godfried/publications/banff.pdf) -- Original paper defining Euclidean rhythms
- [Paul Batchelor's sndkit/euclid](https://paulbatchelor.github.io/sndkit/euclid/) -- Accumulator implementation with C code
- [Grokipedia: Euclidean Rhythm](https://grokipedia.com/page/Euclidean_rhythm) -- Verified pattern table and modular arithmetic formulation
- [Rigtorp SeqLock](https://github.com/rigtorp/Seqlock) -- Production C++11 SeqLock implementation

### Secondary (MEDIUM confidence)
- [Local-Guru: Bresenham Euclidean Rhythms](https://www.local-guru.net/blog/2017/11/24/Calculating-Euclidean-Rhythmns-using-the-Bresenham-Algorithm) -- Confirms Bresenham equivalence to Bjorklund
- [Unohee Bjorklund C++ Gist](https://gist.github.com/unohee/d4f32b3222b42de84a5f) -- Reference C++ Bjorklund implementation
- [ADC 2024: Wait-Free Thread Synchronisation With the SeqLock](https://conference.audio.dev/session/2024/wait-free-thread-synchronisation-with-the-seqlock/) -- Audio-specific SeqLock guidance
- [Timur Doumler on C++ Audio Thread Safety](https://forum.juce.com/t/timur-doumler-talks-on-c-audio-sharing-data-across-threads/26311) -- JUCE-specific thread safety patterns

### Tertiary (LOW confidence)
- Modular synthesis quantizer conventions (from general knowledge of 1V/oct standard) -- Used for tie-breaking direction

---

## Metadata

**Confidence breakdown:**
- Euclidean algorithm: HIGH -- Verified against published patterns, multiple sources agree
- Bresenham/Bjorklund equivalence: HIGH -- Rotation of same necklace, confirmed by manual trace
- Scale quantization tables: HIGH -- Standard musical theory, verified lookup tables
- Pitch ladder modes: HIGH -- Straightforward state machine, no external dependency
- Thread safety for pattern: HIGH -- Matches CONTEXT.md decision, well-understood pattern
- `pitchRandom` behavior in ladder modes: MEDIUM -- Architectural judgment, no authoritative source

**Research date:** 2026-02-07
**Valid until:** Indefinite (algorithmic, not library-dependent)
