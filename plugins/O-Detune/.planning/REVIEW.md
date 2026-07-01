---
phase: O-Detune-v1.5.2-deep-review
reviewed: 2026-06-30T00:00:00Z
depth: deep
files_reviewed: 5
files_reviewed_list:
  - plugins/O-Detune/Source/PluginProcessor.cpp
  - plugins/O-Detune/Source/PluginProcessor.h
  - plugins/O-Detune/Source/PluginEditor.cpp
  - plugins/O-Detune/Source/PluginEditor.h
  - plugins/O-Detune/Source/ui/public/modules/preset-manager.js
findings:
  critical: 1
  warning: 4
  info: 6
  total: 11
status: issues_found
---

# O-Detune v1.5.2: Deep Code Review

**Reviewed:** 2026-06-30
**Depth:** deep
**Files Reviewed:** 5
**Status:** issues_found

## Summary

O-Detune is a JUCE 8 stereo detune/chorus effect with a WebView UI. The WebView bridge
(relays / attachments / native functions / resource provider) is correct and complete — every
JS-side native call is registered, and `getResource` compares bare paths correctly. The preset
JS module is defensively written and has no functional defects.

The DSP core, however, has one real-time-safety violation and a cluster of **dead parameter /
illusory-smoothing** bugs: three user-facing controls and five `SmoothedValue` members are wired
into the code but never actually consumed by the audio path. The net effect is a knob that does
nothing (Randomization), audible zipper noise on several automated parameters, an "Era" control
that only does a third of what it claims, and broken latency reporting. None crash, but several
are user-observable correctness defects.

---

## Critical Issues

### CR-01: IIR coefficient factories heap-allocate on the audio thread every block

**File:** `plugins/O-Detune/Source/PluginProcessor.cpp:588-594`
**Issue:** `juce::dsp::IIR::Coefficients<float>::makeHighPass/makeLowPass` each construct and return a
heap-allocated `ReferenceCountedObject`. They are called unconditionally on **every** `processBlock`,
regardless of whether `focus_low`/`focus_high` changed — a real-time-safety violation (heap
allocation on the audio thread) plus wasted CPU.
**Failure scenario:** With the plugin instantiated and audio running at a small block size (e.g. 64
samples @ 48 kHz → ~750 blocks/sec), two `Coefficients` objects are allocated and freed every block
even when the Focus knobs are untouched. Under memory pressure or with the allocator contended by
another thread, this can cause a priority-inversion stall on the audio callback → dropouts/glitches.
**Fix:** Only rebuild coefficients when the cutoff actually changes, and reuse the existing
`Coefficients::Ptr` rather than allocating a fresh object each block. Cache the last low/high values:
```cpp
if (focusLow != lastFocusLow) {
    *focusHighPassL.coefficients = *juce::dsp::IIR::Coefficients<float>::makeHighPass(currentSampleRate, focusLow);
    *focusHighPassR.coefficients = *focusHighPassL.coefficients;
    lastFocusLow = focusLow;
}
if (focusHigh != lastFocusHigh) {
    *focusLowPassL.coefficients = *juce::dsp::IIR::Coefficients<float>::makeLowPass(currentSampleRate, focusHigh);
    *focusLowPassR.coefficients = *focusLowPassL.coefficients;
    lastFocusHigh = focusHigh;
}
```
The `makeHighPass/makeLowPass` calls still allocate when a value changes; to be fully RT-safe,
precompute coefficient sets off-thread or accept the allocation only on the (rare) change edge.
Either way, removing the per-block unconditional allocation is required.

---

## Warnings

### WR-01: `random_amt` ("Randomization") parameter has zero effect on the audio

**File:** `plugins/O-Detune/Source/PluginProcessor.cpp:484-488, 524`
**Issue:** `random_amt` is declared, exposed in the UI, loaded (`randomAmt`, line 488), and pushed into
`smoothedRandomAmt` (line 524) — but neither `randomAmt` nor `smoothedRandomAmt` is ever consumed in
the DSP. The per-voice `voiceRandomOffsets[]` used by the unison distribution are seeded once in
`prepareToPlay` from `random` and are independent of this parameter.
**Failure scenario:** A user turns the Randomization knob from 0 % to 100 % (or automates it); the
output is bit-for-bit identical. The control is dead.
**Fix:** Either apply `randomAmt`/`smoothedRandomAmt.getNextValue()` to scale per-voice detune/LFO
variation (e.g. modulate `voiceRandomOffsets` influence or add per-voice pitch jitter proportional to
`randomAmt/100`), or remove the parameter and its UI/preset entries if the feature is unshipped.

### WR-02: Illusory smoothing — five `SmoothedValue` members set a target but are never read

**File:** `plugins/O-Detune/Source/PluginProcessor.cpp:516-524` (targets) vs. actual DSP reads
**Issue:** `smoothedWobbleRate` (516), `smoothedWobbleDepth` (517), `smoothedUnisonDetune` (518),
`smoothedUnisonSpread` (523), and `smoothedRandomAmt` (524) have `setTargetValue()` called every
block, but no `getNextValue()`/`getCurrentValue()` consumes them. The DSP instead reads the **raw**
atomic parameter values directly: `effectiveRate`/`wobbleRate` (line 539/675), `scaledWobbleDepth`
from raw `wobbleDepth` (535), `unisonDetune` raw (724), and `unisonSpread` raw (710). The smoothing is
inert — it neither smooths anything nor is free (the `reset`/`setTargetValue` calls are wasted), and
it actively misleads any maintainer into thinking these params are de-zippered.
**Failure scenario:** Automate `wobble_depth` or `unison_detune` quickly (e.g. a fast filter-sweep-style
automation lane). Because the raw value is applied per-block with no ramp, the modulation depth jumps
in block-sized steps → audible zipper/stepping artifacts in the pitch modulation.
**Fix:** Actually consume the smoothed values in the per-sample loops (replace raw `wobbleDepth`,
`unisonDetune`, `unisonSpread` reads with `getNextValue()`), or delete the unused `SmoothedValue`
members and their `reset`/`setTargetValue` calls to stop misrepresenting the smoothing behaviour.

### WR-03: `getLatencySamples()` override is dead — host never sees the ~50 ms wet delay

**File:** `plugins/O-Detune/Source/PluginProcessor.h:45`, `PluginProcessor.cpp:256`
**Issue:** In JUCE 8 `AudioProcessor::getLatencySamples()` is **non-virtual**; the custom
`int getLatencySamples() const { return latencySamples; }` shadows it but is never called through the
base pointer the host uses. `setLatencySamples()` is never called anywhere (confirmed via grep), so the
reported latency stays 0 while the wobble/unison engines impose a ~50 ms (`centerDelayMs`) delay on the
wet path. `latencySamples` (computed at line 256) is thus dead.
**Failure scenario:** Two-fold. (1) Host PDC never compensates the plugin, so the detuned/wet signal
lands ~50 ms late relative to other tracks. (2) The `DryWetMixer` applies no wet-latency compensation
(`setWetLatency` is never called), so at any partial `mix` the undelayed dry sums with the 50 ms-delayed
wet → slapback/comb-filtering instead of a tight chorus/detune.
**Fix:** Compensate the dry path against the wet delay with
`dryWetMixer.setWetLatency(centerDelaySamples)` in `prepareToPlay`, and report the resulting total
latency to the host with `setLatencySamples(N)` (not the shadowing getter). Remove the dead
`getLatencySamples()` override and the `latencySamples` member if not otherwise needed.

### WR-04: `wobble_era` only scales depth — the advertised darkening/drift is never applied

**File:** `plugins/O-Detune/Source/PluginProcessor.cpp:527-535`
**Issue:** `EraPreset` carries `{ depthMult, darkness, drift }` and the table comments promise "darker",
"brighter", "more/less drift" per era. Only `depthMult` is consumed (line 535); `darkness` and `drift`
are never read anywhere in `processBlock`.
**Failure scenario:** Switching Wobble Era 60s ↔ 80s changes only the modulation depth (1.2× vs 0.8×);
the tonal/darkening and drift character the UI/comments imply never materialize. Users perceive the Era
control as a near-duplicate of the Depth knob.
**Fix:** Apply `darkness` to the Focus low-pass cutoff (or a dedicated era tone filter) and `drift` to a
slow random detune offset, or trim the struct to `depthMult` only and correct the misleading comments.

---

## Info

### IN-01: `wobbleLFO` (`juce::dsp::Oscillator`) is fully dead

**File:** `plugins/O-Detune/Source/PluginProcessor.h:65`, `PluginProcessor.cpp:275-278`
**Issue:** `wobbleLFO` is prepared, initialised, frequency-set, and reset in `prepareToPlay`, but never
used in `processBlock` — the wobble LFO is generated by the hand-rolled `generateLFO`. Dead member and
wasted setup.
**Fix:** Remove `wobbleLFO` and its `prepareToPlay` block.

### IN-02: Several dead state members / unused function parameter

**File:** `plugins/O-Detune/Source/PluginProcessor.h:93,98,106-107`; `PluginProcessor.cpp:362`
**Issue:** `feedbackStateL/R`, `randomRefreshCounter`, and `noiseLastQuarter` are reset in
`prepareToPlay` but never read/updated in the DSP. `generateLFO`'s `lastQuarter` parameter (and the
`noiseLastQuarter` argument passed at line 672) is unused inside the function body.
**Fix:** Delete the unused members and drop the `lastQuarter` parameter from `generateLFO`.

### IN-03: `unisonBuffer.makeCopyOf(buffer)` immediately followed by `clear()`

**File:** `plugins/O-Detune/Source/PluginProcessor.cpp:703-704`
**Issue:** The copy at line 703 is discarded by the `clear()` at 704 (the buffer is only used as a
zeroed accumulator). The copy is pure wasted work each block.
**Fix:** Replace with a size-only guarantee, e.g. `unisonBuffer.setSize(numChannels, numSamples, false, false, true); unisonBuffer.clear();` (setSize is a no-op when already sized), or just `unisonBuffer.clear()` since it is pre-sized in `prepareToPlay`.

### IN-04: Unison delay clamp upper bound exceeds the unison delay-line capacity (latent)

**File:** `plugins/O-Detune/Source/PluginProcessor.cpp:770` vs `266,285`
**Issue:** `delayTime` is clamped to `centerDelaySamples * 1.5` (= 75 ms) but the unison delay lines are
sized to `maxDelaySamples` (= 60 ms). The clamp is currently unreachable — actual `delayTime` peaks near
53.6 ms given max `unison_detune` (50 cents → ±3.6 ms around the 50 ms center) — so no overflow occurs
today. It is a latent trap: any future increase to detune range or `voiceModDepth` would push
`setDelay()` past the buffer maximum.
**Fix:** Either size the unison buffers to `centerDelaySamples * 1.5` or clamp `delayTime` to the actual
`maxDelaySamples`, so the clamp and the allocation agree.

### IN-05: Width smoothing does not advance when the "not default" guard skips the block

**File:** `plugins/O-Detune/Source/PluginProcessor.cpp:827-842`
**Issue:** When `currentWidth ≈ 100` the per-sample loop (and thus `smoothedWidth.getNextValue()`) is
skipped, so the smoother's internal position is not advanced. On the block where width next diverges,
the ramp resumes from a stale `getCurrentValue()`, which can produce a small step. Minor.
**Fix:** Advance/settle the smoother even on the skip path, or gate on the target value rather than the
current value.

### IN-06: `makeCopyOf(..., true)` / pre-sized buffers assume host block ≤ `samplesPerBlock`

**File:** `plugins/O-Detune/Source/PluginProcessor.cpp:345-346, 658, 703`
**Issue:** `wobbleBuffer`/`unisonBuffer` are pre-sized to `samplesPerBlock`. If a host ever delivers a
block larger than the prepared `samplesPerBlock` (some hosts do on the first/last block or when settings
change without re-prepare), `makeCopyOf(buffer, true)` would reallocate on the audio thread. Low
likelihood given JUCE's contract, but worth a guard.
**Fix:** Clamp processing to `min(numSamples, prepared size)` or defensively re-check buffer capacity;
alternatively rely only on `buffer`'s own storage.

---

## Summary Table

| Severity | Count | Findings |
|----------|-------|----------|
| Critical | 1     | CR-01 (per-block IIR heap allocation on audio thread) |
| Warning  | 4     | WR-01 (dead Randomization knob), WR-02 (illusory smoothing / zipper), WR-03 (dead latency reporting + uncompensated dry/wet), WR-04 (Era darkening/drift never applied) |
| Info     | 6     | IN-01 (dead wobbleLFO), IN-02 (dead state members), IN-03 (wasted copy), IN-04 (latent clamp/buffer mismatch), IN-05 (width smoother stall), IN-06 (block-size assumption) |
| **Total**| **11**| |

---

_Reviewed: 2026-06-30_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
