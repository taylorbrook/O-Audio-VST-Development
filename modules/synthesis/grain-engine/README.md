# grain-engine

Header-only granular building blocks, namespace `ouaricon::grain`.

| Header | What | Origin |
|---|---|---|
| `WindowLut.h` | 5 windows + tilt + Tukey α; μ₁/μ₂ moments; shape/tilt/loop/forward norms | O-ReverseDelay 1.22.4 (moved verbatim) |
| `GrainScheduler.h` | Fractional-onset clock, block-size invariant, drop-without-shift | O-Freeze 2.8.0 |
| `CounterRng.h` | `f(seed, counter, slot)` splitmix64 RNG + `Draw` | O-TextureForge / Freeze / simpleGrain / GrainScatter (identical cores) |
| `GrainPool.h` | Preallocated slots, fade-on-steal into spare slots, `tryObtain()` refuse | O-Freeze 2.7.0 / O-simpleGrain 1.5.4 / O-GrainScatter 3.0.5 |
| `BandlimitedReader.h` (1.1.0) | Band-limited grain reads: half-band mip ring + 16-tap polyphase sinc + freeze snapshot | new; first consumer O-GrainScatter 3.3.0 |

Only `WindowLut` needs JUCE (`juce_core`). Everything else is plain C++20.

## Add to a plugin

```cmake
ouaricon_add_module(<Target> grain-engine)
```

## Wiring sketch

```cpp
#include "WindowLut.h"
#include "GrainScheduler.h"
#include "GrainPool.h"
#include "CounterRng.h"
using namespace ouaricon::grain;

struct Grain { double age; double frac; double invLen; float pan; /* ... */ };

WindowLut                 windows { 2048 };   // constructor only — never in prepare/process
GrainScheduler            scheduler;          // reset() in prepareToPlay
GrainPool<Grain, 32, 8>   pool;               // prepare (sampleRate) in prepareToPlay
std::array<Onset, 2048>   onsets;             // preallocated
uint64_t                  seed;               // save with the session

// processBlock
auto r = scheduler.process (numSamples, onsets.data(), (int) onsets.size(),
    [&] (uint64_t k) { return GrainScheduler::jittered (interval, jitter,
                                                         CounterRng::uniform (seed, k, kSlotInterval)); });
// r.dropped: count it, don't trust a bound.

for (int i = 0; i < r.count; ++i)
{
    const auto& o = onsets[(size_t) i];
    auto& g = pool.obtain ([] (const Grain& g) { return g.age * g.invLen; });   // steal = furthest along
    g.frac = o.frac;                                                // window starts frac samples in
    g.pan  = CounterRng::uniform (seed, o.index, kSlotPan);
    // ... start rendering g at sample o.offset
}

pool.render ([&] (Grain& g, auto& fade)
{
    // per sample: w = windows.readAt (table, (g.age + g.frac) * g.invLen) * fade.next();
    return /* still inside its window */ true;
});
```

## Band-limited reader (1.1.0)

Pitched-up grains alias unless the source is band-limited first. `MipRing<3>` keeps the ring at full rate plus 1/2, 1/4 and 1/8 rate: a causal 127-tap Kaiser half-band (centre 0.5, odd taps only, −92 dB from 0.2732) runs per pushed sample. A grain latches an octave level at spawn and reads it through a 16-tap sinc whose cutoff set follows the residual rate on that level.

```cpp
#include "BandlimitedReader.h"
using namespace ouaricon::grain;

MipRing<3>    ring;     // prepare (capacity) in prepareToPlay — allocates, and builds sincBank()
MipSnapshot<3> held;    // prepare (maxLength) in prepareToPlay

ring.push (l, r);                                  // per input sample (cascade inside)

// spawn
const int level = latchLevel (rate);               // clamp (floor (log2 rate), 0, 3)
const int set   = SincBank::setFor (std::ldexp (rate, -level));

// per output sample: delay in LEVEL-0 samples behind the head (double)
ring.readStereo (level, delay, set, outL, outR);   // or read (…, ch, …) / readMono (…)

// freeze: capture every level, ending margin(3) = 506 samples behind the head
held.captureFrom (ring, MipRing<3>::margin (3), lengthSamples);
held.readStereo (level, snapshotPos, set, outL, outR);   // snapshot coordinates
```

- **Time alignment.** Level l sample j sits at level-0 time 2^l·j − 63(2^l − 1) + (2^l − 1). Reads map through that exactly (integer and fraction split before the divide), so a grain on level 1 and one on level 0 hit a transient at the same time (≤ 0.05 sample, test B2).
- **Read guard.** A level-l read needs `margin(l)` = 9 / 80 / 222 / 506 samples behind the head. `read*` clamps the delay to it.
- **Sets.** S0 is used for r′ ≤ 1; S1..S6 have upper edges 2^(k/6); S7 covers (2, 4). S0..S6 keep every audible spurious component ≤ −60 dB at 44.1 kHz up to their edge, including level ≥ 1 content in the half-band transition band. S7 holds −60 dB to r′ 2.83 and falls to about −20 dB at r′ 3.99, which is only reached by Doppler > 1.41 near the top of an octave.
- **Passband.** 16 taps trade top-octave bandwidth: the output −1 dB point at 44.1 kHz runs from about 16 kHz just above an octave boundary to about 8.3 kHz just below the next one. `blr::kTaps` is the one constant to raise.
- **Known limit.** A Doppler below about 0.9 on a grain latched to level ≥ 1 lets the half-band transition band (level-0 10–11 kHz per octave) image into the audible band at up to about −30 dB. The level latch is floor(log2 rate) by design.
- **Real time.** `prepare` allocates; `push`, `read*`, `clear`, `captureFrom` and the snapshot reads do not (test B5). Constants come from `research-scripts/emit_constants.py` in the O-GrainScatter band-limited-reader milestone.

## Contracts

- **WindowLut**: tables and moments are built in the constructor. μ₁ = `getMean`, μ₂ = `getMeanSquare`; `getMoments()` adds the half-window parts. Symmetric shapes report `lo == hi` bitwise, so `getTiltNorm` is exactly `1.0f` for them.
- **GrainScheduler**: the interval callback must be a pure function of the onset index. Draw jitter from `CounterRng` keyed on the index, never from a sequential generator.
- **GrainPool**: a live grain's `fade.next()` is exactly `1.0f`. Call it once per rendered sample, including in block-wise renderers.
- **CounterRng**: O-Polystutter's copy uses a different 4-argument hash and is not interchangeable with this one.

## Tests

```bash
cmake -S modules/synthesis/grain-engine/tests -B /tmp/ge-tests -G Ninja -DCMAKE_BUILD_TYPE=Release
ninja -C /tmp/ge-tests && /tmp/ge-tests/grain-engine-tests_artefacts/Release/grain-engine-tests
```

The B* checks cover the reader: unity DC on every phase, level alignment, the half-band and driven sinc stopbands, block-split invariance, no allocation, snapshot reads bit-identical to live reads, and the read-guard margins. The older checks cover μ₁/μ₂ three ways: a double-precision rebuild of the tables, 2²⁰-step Simpson quadrature of each continuous window, and the closed forms. They also pin RNG golden values from the plugin copies, scheduler grid exactness and block-size invariance (1 to 16384), and pool steal continuity (max step 1/240 at 48 kHz, against 1.0 for a hard cut).
