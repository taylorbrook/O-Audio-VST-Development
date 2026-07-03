---
phase: GROUP-B-voice-source-dsp
reviewed: 2026-07-01T00:00:00Z
depth: deep
files_reviewed: 13
files_reviewed_list:
  - plugins/O-Formant/Source/dsp/LFGlottalSource.h
  - plugins/O-Formant/Source/dsp/GlottalWavetable.h
  - plugins/O-Formant/Source/dsp/GlottalTableGenerator.cpp
  - plugins/O-Formant/Source/dsp/GlottalTableGenerator.h
  - plugins/O-Formant/Source/dsp/FormantBiquad.h
  - plugins/O-Formant/Source/dsp/FormantFilterBank.h
  - plugins/O-Formant/Source/dsp/CascadeFormantBank.h
  - plugins/O-Formant/Source/dsp/NasalPoleZero.h
  - plugins/O-Formant/Source/dsp/AspirationNoise.h
  - plugins/O-Formant/Source/dsp/PitchGlide.h
  - plugins/O-Formant/Source/dsp/VibratoLFO.h
  - plugins/O-Formant/Source/dsp/VowelData.h
  - plugins/O-Formant/Source/dsp/VowelMorpher.h
findings:
  critical: 0
  warning: 4
  info: 6
  total: 10
status: issues
---

# GROUP B: Voice-Source & Formant DSP — Code Review Report

**Reviewed:** 2026-07-01
**Depth:** deep
**Files Reviewed:** 13
**Status:** issues_found

## Summary

This group is DSP-heavy and, on the whole, defensively written: all resonators use
pole radius `r < 1` (hard-clamped to `0.9999`), bandpass Q is clamped to `[0.5, 25]`,
formant frequencies are clamped to `[20, Nyquist-100]`, `FormantBiquad::processSample`
has a per-sample NaN/Inf state guard, and coefficient objects are returned as
stack `std::array<float,6>` (no heap allocation on the audio thread). Table generation
is correctly isolated in an offline path (`GlottalTableGenerator`, called at init).
I traced the filter math and found **no unconditional blow-up** under realistic
parameters — hence **0 Critical**.

However, several robustness gaps remain that produce audible defects or memory-unsafe
reads under edge conditions the code does not defend against:

1. `VowelMorpher` inverse-distance weighting can overflow to `Inf` → `NaN` formant
   frequencies. The downstream NaN guard converts this to a *sticky silent filter*, not
   a blow-up, but it is a real dropout.
2. `LFGlottalSource`'s phase wrap uses a single `if` (not `while`), so a per-sample
   phase advance `>= 1.0` (reachable at low sample rates with high MIDI notes) leaves
   `phase >= 1.0` and reads the wavetable **out of bounds** via the unchecked
   `GlottalWavetable::getSample`.
3. The `FormantBiquad` NaN guard resets state but leaves poisoned coefficients, so a
   single NaN coefficient update silences the filter until the next *valid* block-rate
   update.
4. Per-voice feedback filters have no explicit denormal handling and rely entirely on
   the caller wrapping `processBlock` in `ScopedNoDenormals`.

## Warnings

### WR-01: VowelMorpher IDW weight can overflow to Inf → NaN formant frequencies

**File:** `plugins/O-Formant/Source/dsp/VowelMorpher.h:48-53`
**Issue:** The Shepard/IDW weight is `weights[v] = 1.0f / std::pow(dist, focus)` with no
upper bound. The only guard is a hard snap when `dist < 1e-6f` (line 36). For any
`dist` in the open interval just above `1e-6` combined with a large `focus`, the term
`std::pow(dist, focus)` underflows toward `0`, so `1.0f / pow(...)` overflows to `+Inf`.
Concrete failure: `dist = 2e-6`, `focus = 7` → `pow(2e-6, 7) ≈ 1.3e-40` (subnormal /
underflows to 0 in float) → `1/pow = Inf`. `weightSum` then becomes `Inf`,
`invSum = 1/Inf = 0`, and `weights[v] = Inf * 0 = NaN`. Every `outFreq[f] = std::exp(NaN)
= NaN` is then handed to `FormantFilterBank::updateCoefficients` / `CascadeFormantBank`,
which feed NaN into `makeBandPass`/`makeResonator` → NaN biquad coefficients. The
`FormantBiquad` NaN guard (FormantBiquad.h:34) then forces the filter output to `0`
every sample and *never recovers* until a subsequent block writes finite coefficients
(i.e. until the cursor moves off the pathological spot). Result: an audible per-formant
dropout while the cursor hovers just outside a vowel node at high `focus`.
Additionally, `weightSum` is used as a divisor at line 53 with no zero/finite check.
**Fix:** Clamp each weight and/or enlarge the snap epsilon relative to `focus`:
```cpp
float d = std::max (dist, 1e-3f);              // floor distance before pow
float w = 1.0f / std::pow (d, focus);
weights[v] = std::min (w, 1.0e12f);            // cap to keep sum finite
weightSum += weights[v];
...
float invSum = (weightSum > 0.0f && std::isfinite (weightSum))
                 ? 1.0f / weightSum : 0.0f;
```
Also consider clamping `cursorX/cursorY` to `[0,1]` on entry (see IN-02).

### WR-02: LFGlottalSource phase wrap is single-`if` → out-of-bounds wavetable read

**File:** `plugins/O-Formant/Source/dsp/LFGlottalSource.h:78-82, 107-108` (read via `GlottalWavetable::getSample`, GlottalWavetable.h:37-40)
**Issue:** `phase += phaseIncrement * cyclePhaseIncrementMod;` followed by
`if (phase >= 1.0) phase -= 1.0;`. A single subtraction only corrects a phase in
`[1.0, 2.0)`. If one sample advances phase by `>= 1.0` (i.e. `phaseIncrement >= 1.0`,
which is `f0 >= sampleRate`), the post-wrap phase can remain `>= 1.0`. Then
`samplePos = phase * 2048` gives `idx0 >= 2048`, and `getSample(level, rd, idx0)` /
`getSample(..., idx0 + 1)` index `data[]` past the 2049-sample frame with **no bounds
check** (getSample is `noexcept` raw indexing). `setFrequency` performs no clamp, so
this is reachable at low sample rates: at `sr = 8000`, a MIDI note ~120+ gives
`f0 ≈ 8372 > sr` → `phaseIncrement > 1.0` → OOB read (heap over-read into the adjacent
Rd frame or past the vector). Even at 44.1 kHz it is a latent memory-safety hole with no
guard.
**Fix:** Use a `while` loop and/or clamp the increment:
```cpp
phase += phaseIncrement * cyclePhaseIncrementMod;
while (phase >= 1.0) { phase -= 1.0; updateCyclePerturbations(); }
```
and clamp in `setFrequency`: `phaseIncrement = std::min (0.5, (double) f0 / sampleRate);`

### WR-03: FormantBiquad NaN guard leaves poisoned coefficients → sticky silence

**File:** `plugins/O-Formant/Source/dsp/FormantBiquad.h:33-42`
**Issue:** When `z1`/`z2` become non-finite the guard resets *state* (`z1=z2=0`) and
returns `0`, but it does not repair `b0..a2`. If the coefficients themselves are NaN/Inf
(e.g. from WR-01, or any transient NaN frequency during automation), every subsequent
sample recomputes `output = b0*input + z1 = NaN`, re-trips the guard, and returns `0`
*indefinitely* until `setCoefficients` is called again with finite values. Because
`updateCoefficients` only writes coefficients directly when NOT smoothing
(FormantFilterBank.h:103, CascadeFormantBank.h:121) and `process()` only recomputes
while smoothing, a filter can remain silent across many blocks. This turns a
one-sample transient into a persistent per-formant dropout.
**Fix:** Validate coefficients at the point of assignment and fall back to a pass-through /
previous-good set:
```cpp
void setCoefficients (const std::array<float,6>& c) noexcept
{
    if (! (std::isfinite(c[0]) && std::isfinite(c[1]) && std::isfinite(c[2])
        && std::isfinite(c[4]) && std::isfinite(c[5])))
        return;                 // keep last-known-good coefficients
    b0 = c[0]; b1 = c[1]; b2 = c[2]; a1 = c[4]; a2 = c[5];
}
```

### WR-04: No denormal flushing in per-voice feedback filter state

**File:** `plugins/O-Formant/Source/dsp/FormantBiquad.h:27-42`, `CascadeFormantBank.h:191-208` (resonators `r ≈ 0.997`), `AspirationNoise.h:95-96` (one-pole tilt), `NasalPoleZero.h:158-160`
**Issue:** The formant resonators use a pole radius of ~`0.997` (bw=40, sr=44.1k →
`r = exp(-pi*40/44100) = 0.9972`), giving very long decaying tails. When input goes
silent (note release, gaps) the biquad state (`z1`,`z2`) and the aspiration tilt state
(`tiltPrev`) decay through the denormal range. None of these headers flush denormals;
they rely entirely on the caller wrapping the audio callback in
`juce::ScopedNoDenormals`. If any per-voice processing runs outside that scope (e.g. a
voice-render helper or offline table probe), x86 hardware will trap on denormals and
spike CPU. This is called out explicitly in the review brief ("denormals in decaying
filter tails").
**Fix:** Either guarantee `ScopedNoDenormals` in the voice/processBlock entry (document
it) or add a cheap flush in `processSample`:
```cpp
z1 += 1.0e-20f; z1 -= 1.0e-20f;   // or: if (std::abs(z1) < 1e-15f) z1 = 0.0f;
```

## Info

### IN-01: Mipmap level uses floor + crossfade → residual aliasing above Nyquist

**File:** `plugins/O-Formant/Source/dsp/LFGlottalSource.h:86-90`
**Issue:** `levelFloat = log2(max(freq,baseFreq)/baseFreq)`, then `level0 = (int)levelFloat`
(floor) is crossfaded with `level1`. The `level0` component retains
`1024 >> level0` harmonics, which for a floored level exceeds the alias-free harmonic
count for the current `f0`. Concretely at `f0 = 220 Hz`, `baseFreq ≈ 21.5 Hz`,
`levelFloat ≈ 3.35` → `level0 = 3` keeps `1024>>3 = 128` harmonics → `128*220 = 28160 Hz
> 22050 Hz` Nyquist. The `(1-levelFrac)` share of that level leaks mild aliasing.
**Fix:** Bias toward the safe level (use `ceil`, or add a small guard so the retained
level never exceeds `Nyquist/f0` harmonics). Low priority — this is a common wavetable
tradeoff and audibly minor.

### IN-02: VowelMorpher assumes cursor is pre-clamped to [0,1]

**File:** `plugins/O-Formant/Source/dsp/VowelMorpher.h:24-34`
**Issue:** `compute()` does not clamp `cursorX/cursorY`. If an XY control ever passes
values outside `[0,1]`, all `dist` grow and (with high `focus`) compound WR-01. Also the
IDW extrapolation outside the vowel convex hull is undefined-ish.
**Fix:** `cursorX = std::clamp(cursorX, 0.0f, 1.0f);` (and Y) at function entry.

### IN-03: Per-sample transcendental coefficient recompute during transitions

**File:** `plugins/O-Formant/Source/dsp/FormantFilterBank.h:117-129`, `CascadeFormantBank.h:147-168`, `NasalPoleZero.h:85-90`
**Issue:** While a formant is smoothing, `process()` recomputes `makeBandPass` /
`makeResonator` every sample for up to 5 filters (each `sin`/`cos`/`exp`). This is
RT-safe (no allocation) and bounded to the transition window, but it is heavy during
vowel morphs (up to 120 ms). Not a correctness bug; flagged per the brief's
"transcendental recomputation that should be cached."
**Fix (optional):** Interpolate biquad coefficients directly, or recompute on a decimated
grid (every N samples) and interpolate between coefficient sets.

### IN-04: updateCoefficients uses passed `sr`, process() uses member `sampleRate`

**File:** `plugins/O-Formant/Source/dsp/FormantFilterBank.h:69, 107` vs `125`; same pattern `CascadeFormantBank.h:79 vs 156`
**Issue:** `updateCoefficients` takes a `double sr` argument and uses it for
`makeBandPass`, while `process()` uses the member `sampleRate` set in `prepare()`. If a
caller ever passes a `sr` that differs from the prepared rate, the steady-state and
transitioning coefficients are computed at different rates → detuned formants.
**Fix:** Drop the redundant `sr` parameter and use the member `sampleRate` consistently,
or `jassert (sr == sampleRate)`.

### IN-05: AspirationNoise::reset sets breath current to 0.1 (not target)

**File:** `plugins/O-Formant/Source/dsp/AspirationNoise.h:105`
**Issue:** `reset()` calls `breathSmoothed.setCurrentAndTargetValue(0.1f)`, overriding any
target set by `setBreathiness`. After reset the breathiness jumps to 0.1 and then ramps
(20 ms) toward the real target on the next `setBreathiness`, which can produce a small
attack-time timbre glitch if `reset()` is called on note-on after the target was set.
**Fix:** Reset to the current target instead: `breathSmoothed.setCurrentAndTargetValue
(breathSmoothed.getTargetValue());` (or leave breath state untouched in `reset`).

### IN-06: solveAlpha Newton iteration can overflow exp() during search (offline)

**File:** `plugins/O-Formant/Source/dsp/GlottalTableGenerator.cpp:64-98`
**Issue:** `std::exp(alpha*Te)` with an unbounded Newton step can overflow to `Inf` on
intermediate iterations for extreme `Rd`, and `f/df` can step wildly since `df` is a
coarse `da=0.01` forward difference. This is offline (init-time) and `renderLFPeriod`
sanitizes non-finite samples afterward (lines 173-177), so it does not reach the audio
thread — but a divergent `alpha` silently yields a degenerate period.
**Fix (optional):** Bound `alpha` per iteration (e.g. `alpha = jlimit(0.0f, 200.0f/Tp,
alpha)`) and break on non-finite `f`. Cosmetic given the downstream sanitize + normalize.

---

## Notes on things checked and cleared (skeptical pass)

- **Resonator/bandpass stability:** `CascadeFormantBank::makeResonator` and
  `NasalPoleZero::makeResonator` clamp `r <= 0.9999` and `a2 = r^2 < 1`, poles strictly
  inside the unit circle — stable even chained 5-deep. `makeBandPass` Q is clamped
  `[0.5, 25]` and freq to `[20, Nyquist-100]`; inherently stable. **No blow-up found.**
- **Divide-by-zero:** `PitchGlide::setTime` guards `timeMs>0 && sampleRate>0`;
  `VibratoLFO` guards `jmax(1, delaySamples)`; `AspirationNoise` guards
  `driftUpdateInterval >= 1`; formant Q uses `max(bw, 1.0f)`. All clear.
- **Allocation on audio thread:** All coefficient factories return stack
  `std::array<float,6>`; no `Coefficients::Ptr`/heap in the hot path. Table generation
  is offline. Clear.
- **Wavetable guard sample:** `kFrameSize = 2049`, guard at index 2048 = copy of
  sample[0], set AFTER `generateMipmaps` overwrites [0..2047]. `idx0+1` reaches the guard
  correctly under normal phase. Clear (except the WR-02 OOB edge).

---

_Reviewed: 2026-07-01_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
