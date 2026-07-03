---
phase: GROUP-C-consonant-effects-dsp
reviewed: 2026-07-01T00:00:00Z
depth: deep
files_reviewed: 9
files_reviewed_list:
  - plugins/O-Formant/Source/dsp/ConsonantEngine.h
  - plugins/O-Formant/Source/dsp/FricationFormantBank.h
  - plugins/O-Formant/Source/dsp/LyricsEngine.h
  - plugins/O-Formant/Source/dsp/DelayProcessor.cpp
  - plugins/O-Formant/Source/dsp/DelayProcessor.h
  - plugins/O-Formant/Source/dsp/EQProcessor.cpp
  - plugins/O-Formant/Source/dsp/EQProcessor.h
  - plugins/O-Formant/Source/dsp/ReverbProcessor.cpp
  - plugins/O-Formant/Source/dsp/ReverbProcessor.h
findings:
  critical: 0
  warning: 2
  info: 4
  total: 6
status: issues_found
---

# GROUP-C: Consonant Articulation + Output Effects — Code Review Report

**Reviewed:** 2026-07-01
**Depth:** deep
**Files Reviewed:** 9
**Status:** issues_found

## Summary

Reviewed the Klatt-style consonant path (ConsonantEngine, FricationFormantBank,
LyricsEngine) and the whole-buffer output effects chain (Delay, EQ, Reverb).

The v1.24.2 ConsonantEngine envelope rework is **correct**: the 3-phase
Attack/Hold/Decay state machine cannot advance past 1.0 (attack uses
`jmin(val, 1.0f)` and resets `consonantEnvSample` on every phase transition),
all sample counts are `jmax(1, …)`-guarded so no divide-by-zero, and the burst
transient is correctly summed *outside* the envelope multiplier. Envelope
timing recomputation mid-note (block-rate `updateCoefficients`) degrades
gracefully. No envelope overflow/underflow found.

Delay-line and reverb buffer arithmetic is bounds-safe at all realistic sample
rates: the reverb `DelayLine` uses power-of-two `& mask` wrap (safe for negative
read indices under C++20 two's complement), tank/diffusion/predelay/shimmer
buffers are all sized in `prepare()` with headroom that covers max size + LFO
excursion, and the FDN loop gain is `< 1` with a `[-2, 2]` tank clamp, so no
runaway/NaN was found. `juce::ScopedNoDenormals` is asserted at
`PluginProcessor.cpp:745`, covering the reverb/delay tails, and no per-sample
allocations exist in ConsonantEngine/Reverb/Delay.

Two defects worth fixing: (1) the delay buffer is a fixed 192000 samples but the
`delayTime` parameter allows 2.0s, which overruns the buffer above 96 kHz and
silently aliases to a wrong (shorter) delay; (2) the EQ recomputes biquad
coefficients on the audio thread via `makeLowShelf/makePeakFilter/makeHighShelf`,
which heap-allocate on every parameter change — an RT-safety violation during
automation. Neither crashes, so neither is Critical, but both should be fixed.

## Narrative Findings (AI reviewer)

## Warnings

### WR-01: Delay buffer under-sized for 2.0 s at sample rates above 96 kHz

**File:** `plugins/O-Formant/Source/dsp/DelayProcessor.h:29-30`,
`plugins/O-Formant/Source/dsp/DelayProcessor.cpp:45,87-88`

**Issue:** The delay lines are constructed with a fixed maximum of `192000`
samples:
```cpp
juce::dsp::DelayLine<float, Lagrange3rd> delayL { 192000 };
```
`prepare()` does not change that maximum (JUCE's `DelayLine::prepare` only sets
sample rate / channel count). `setTime()` computes
`delaySamples = seconds * currentSampleRate` with **no clamp**, and the
`delayTime` parameter range is `0.001 … 2.0 s`
(`PluginProcessor.cpp:337`). At 96 kHz, 2.0 s = 192000 samples (exactly the
usable max, `totalSize − 2`); at 176.4 kHz / 192 kHz, delay times above
~1.09 s / ~1.0 s exceed the buffer. I traced JUCE 8.0.9
`popSample → setDelay`: in a Release build the `jassert(isPositiveAndNotGreaterThan(...))`
is compiled out and the read index is taken `% totalSize`, so there is **no
out-of-bounds read** — but the effective delay silently aliases to a wrong,
much shorter time. In a Debug build the assert fires. Net effect: at pro sample
rates the delay time knob becomes wrong/discontinuous above ~1 s.

**Fix:** Clamp the requested delay to the line's capacity, and/or size the buffer
from the true worst case. Minimal fix in `setTime`:
```cpp
void DelayProcessor::setTime (float seconds)
{
    float requested = seconds * currentSampleRate;
    delaySamples = juce::jmin (requested,
                               static_cast<float> (delayL.getMaximumDelayInSamples()));
}
```
Better: size the lines for the max supported rate in `prepare()`
(`delayL.setMaximumDelayInSamples((int)(2.0 * spec.sampleRate) + 4)` requires
switching to a runtime-sized line), so 2.0 s is honored at every rate.

### WR-02: EQ recomputes IIR coefficients on the audio thread (heap allocation)

**File:** `plugins/O-Formant/Source/dsp/EQProcessor.cpp:50-73`

**Issue:** Inside `process()` (called per block from `processBlock`), whenever a
gain/freq changes the code does:
```cpp
*lowShelf.state = *FilterCoeffs::makeLowShelf (currentSampleRate, 200.0f, 0.707f, …);
```
`juce::dsp::IIR::Coefficients<float>::makeLowShelf/makePeakFilter/makeHighShelf`
each construct and return a **reference-counted `Coefficients` object on the
heap** (the temporary is allocated and then freed when the returned `Ptr` goes
out of scope). During parameter automation or a knob drag, `midGain`/`midFreq`/
`low`/`high` change every block, so this allocates and frees on the audio thread
on most blocks — an RT-safety violation that risks priority inversion / dropouts.
(Note: `eqMidFreq` range 200–8000 Hz stays below Nyquist at all supported rates,
so filter stability itself is fine — only the allocation is the problem.)

**Fix:** Avoid per-block heap traffic. Options, cheapest first:
- Compute the raw biquad coefficients into a stack array and assign into the
  existing `state->coefficients` in place (no `make*` temporary), or
- Recompute at a decimated rate / only when the delta exceeds a threshold, or
- Prepare new coefficient objects on the message thread and hand them to the
  audio thread via a lock-free single-slot swap (the pattern used elsewhere in
  this codebase, e.g. the oversampled-path `Coefficients::Ptr` swap noted in
  O-AnalogSaturation v1.1.4).

## Info

### IN-01: Dead members in ReverbProcessor

**File:** `plugins/O-Formant/Source/dsp/ReverbProcessor.h:105,124-125` and
`ReverbProcessor.cpp:218,236-238,258`

**Issue:** `tankState` (an 8-float array) is zeroed in `prepare()`/`reset()` but
never read or written in `process()`. `prevSize` and `prevDamping` are declared
and set to `-999.0f` in `prepare()` but never compared — damping is recomputed
unconditionally every sample (`dampCoeff = damping * 0.7f`), and size is gated by
the separate `prevSizeForDelays`. These are leftover/unused state that suggest an
incomplete change-detection pattern.

**Fix:** Remove `tankState`, `prevSize`, `prevDamping`. If per-sample damping
recompute is undesired, gate it with `prevDamping` instead of deleting.

### IN-02: Reverb diffusion delay length is not sample-rate scaled at read time

**File:** `plugins/O-Formant/Source/dsp/ReverbProcessor.cpp:158,205,208`

**Issue:** `applyInputDiffusion` reads with the raw constant
`readNearest(kDiffusionDelays[stage])` (e.g. 142), while the backing buffer is
sized from the SR-scaled `delayLen = kDiffusionDelays[stage] * srRatio + 1`
(+32 headroom). At and above 44.1 kHz the power-of-two buffer is large enough
that the read stays in real (in-bounds) history, so there is no OOB — but the
diffusion delay time is effectively fixed in samples rather than in seconds, and
at sub-32 kHz rates the read would exceed the intended logical length. This is a
tuning inconsistency, not a safety bug.

**Fix:** Read a scaled value (store `scaledDiffusionDelays[stage]` in `prepare`)
so the diffusion time is SR-consistent, matching how `tankDelays`/`scaledDelays`
are handled.

### IN-03: LyricsEngine::peekCurrent reads shared array without the spinlock

**File:** `plugins/O-Formant/Source/dsp/LyricsEngine.h:89-100`

**Issue:** `peekCurrent()` reads `syllables[idx]` without taking `syllableLock`,
whereas `advanceAndGet()` uses a `ScopedTryLock` and `setSyllables()` holds the
lock. This is currently safe *only* because both the sole writer
(`setSyllables`, called from `nativeFunction`) and the sole caller of
`peekCurrent` (`PluginEditor.cpp:625`) run on the message thread, so they never
race on the array. That invariant is undocumented and fragile — if `peekCurrent`
is ever called from the audio thread it becomes a torn-read data race on a
9-field struct.

**Fix:** Add a comment asserting "message thread only," or take the spinlock in
`peekCurrent` for defense in depth (it is not on the audio path).

### IN-04: DelayProcessor mono path writes the shared channel pointer twice

**File:** `plugins/O-Formant/Source/dsp/DelayProcessor.cpp:69,93-94`

**Issue:** When the block is mono, `rightData` aliases `leftData`; the loop then
writes `leftData[i] = wetL; rightData[i] = wetR;`, so the left channel is
overwritten by the right delay line's output. Because both lines receive
identical input and delay, `wetR ≈ wetL`, so audible impact is negligible — but
the intent is unclear and the left-line pop is discarded.

**Fix:** Guard the second write, e.g. `if (block.getNumChannels() > 1)
rightData[i] = wetR;`, and use `wetL` for the mono output.

---

_Reviewed: 2026-07-01_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
