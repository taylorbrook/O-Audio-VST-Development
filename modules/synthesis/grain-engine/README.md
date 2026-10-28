# grain-engine

Header-only granular building blocks, namespace `ouaricon::grain`.

| Header | What | Origin |
|---|---|---|
| `WindowLut.h` | 5 windows + tilt + Tukey α; μ₁/μ₂ moments; shape/tilt/loop/forward norms | O-ReverseDelay 1.22.4 (moved verbatim) |
| `GrainScheduler.h` | Fractional-onset clock, block-size invariant, drop-without-shift | O-Freeze 2.8.0 |
| `CounterRng.h` | `f(seed, counter, slot)` splitmix64 RNG + `Draw` | O-TextureForge / Freeze / simpleGrain / GrainScatter (identical cores) |
| `GrainPool.h` | Preallocated slots, fade-on-steal into spare slots, `tryObtain()` refuse | O-Freeze 2.7.0 / O-simpleGrain 1.5.4 / O-GrainScatter 3.0.5 |

Only `WindowLut` needs JUCE (`juce_core`). The other three are plain C++20.

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

These cover μ₁/μ₂ three ways: a double-precision rebuild of the tables, 2²⁰-step Simpson quadrature of each continuous window, and the closed forms. They also pin RNG golden values from the plugin copies, scheduler grid exactness and block-size invariance (1 to 16384), and pool steal continuity (max step 1/240 at 48 kHz, against 1.0 for a hard cut).
