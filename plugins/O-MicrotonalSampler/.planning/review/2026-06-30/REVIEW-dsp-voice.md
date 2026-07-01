---
phase: dsp-voice
reviewed: 2026-06-30T00:00:00Z
depth: deep
files_reviewed: 4
files_reviewed_list:
  - plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp
  - plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.h
  - plugins/O-MicrotonalSampler/Source/MicrotonalSamplerSound.h
  - plugins/O-MicrotonalSampler/Source/SampleMap.h
findings:
  critical: 1
  warning: 3
  info: 3
  total: 7
status: issues_found
---

# DSP / Voice Rendering: Code Review Report

**Reviewed:** 2026-06-30
**Depth:** deep
**Files Reviewed:** 4
**Status:** issues_found

## Summary

Reviewed the audio-thread voice render path: cubic-Hermite varispeed read, 8-sample
equal-power loop crossfade, dual-cell velocity crossfade, the v1.21+ CC dynamic-crossfade
engine (`renderCcCrossfade` / `gatherLayerCells` / `renderTailRampCc`), voice-steal tail
ramps, round-robin variant selection, and note lifecycle.

The RT-safety discipline is generally strong (cached atomic param pointers, no per-sample
allocation, `ScopedNoDenormals`, xorshift PRNG, atomic-shared-ptr snapshot). The
CC-crossfade math is correct and bracket transitions are continuous.

**One BLOCKER:** two early-return failure paths in `startNote` neglect to clear
`ccDynamicsActive`/`dynLayerCount`. Because `renderNextBlock` dispatches on `ccDynamicsActive`
*first* — before the `variantLow`/`adsr` guards that the velocity path relies on — and because
JUCE calls `renderNextBlock` on every voice each block, a failed note-start after a CC-mode note
leaves the voice rendering from dangling `dynLayers` pointers into a just-released `SampleMap`.
This is a use-after-free (and, when the old buffers survive, a stuck phantom voice).

## Critical Issues

### CR-01: `startNote` failure paths leave stale CC-crossfade state → use-after-free / stuck voice

**File:** `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp:434-443` and `:473-482`

**Issue:**
`renderNextBlock` branches on `ccDynamicsActive` **before** the velocity-path safety guard:

```cpp
// renderNextBlock, line 743
if (ccDynamicsActive)
{
    ...
    renderCcCrossfade (out, startSample, numSamples);   // reads dynLayers[k].variant->audio
    return;
}
if (variantLow == nullptr || ! adsr.isActive()) { ...clear... return; }   // never reached in CC case
```

The two `startNote` early-return blocks clear the velocity-path pointers but **not** the CC
state:

```cpp
// line 434 (currentMap null / no cells) and line 473 (no cell for this note)
cellLow = cellHigh = variantLow = variantHigh = nullptr;
currentMidiNote = -1;
clearCurrentNote();
return;                       // ccDynamicsActive / dynLayerCount NOT reset
```

Scenario: a CC-mode note is playing (`ccDynamicsActive == true`, `dynLayerCount > 0`,
`dynLayers[k].variant` points into the current `SampleMap`). A new `startNote` re-snapshots the
map (line 424) but then fails the cell lookup (line 473) — e.g. the user reloaded a library
(ReplaceAll) whose new map has no cell for this note, or cleared all samples (line 434 path).
`prevMap` is a local that held the *old* map alive only for the duration of this call; on return
it is destroyed. If this voice held the last reference (common right after a ReplaceAll, since the
processor has already atomic-stored the new map), the old `SampleMap` — and every
`SampleVariant::audio` buffer `dynLayers` indexes into — is freed.

`ccDynamicsActive` is still `true`. `renderCcCrossfade`'s own guard
(`if (dynLayerCount <= 0 || !adsr.isActive())`, line 870) does **not** fire: `dynLayerCount` is
still non-zero and `adsr` was never reset (the failure path skips `adsr.reset()`), so it is still
active from the prior note. The next `renderNextBlock` dereferences `dynLayers[k].variant->audio->getReadPointer(...)` on freed memory → **use-after-free / crash**. Even when the old buffers
happen to survive (another ref exists), the voice keeps emitting the *old* CC note's audio after a
trigger that was supposed to fail — a stuck phantom voice.

Note the velocity path is immune only because `renderNextBlock` guards on `variantLow == nullptr`
(which *is* cleared here) — the CC path lacks the equivalent because its `ccDynamicsActive` flag
isn't cleared.

**Fix:** clear the CC state in both failure blocks, exactly as `stopNote(_, false)` and the
render-path exits already do:

```cpp
cellLow = cellHigh = variantLow = variantHigh = nullptr;
ccDynamicsActive = false;   // ADD
dynLayerCount    = 0;       // ADD
currentMidiNote  = -1;
clearCurrentNote();
return;
```

(Defensively, also consider having `renderNextBlock` fall through to the shared clear path when
`ccDynamicsActive && dynLayerCount == 0`, so the dispatch order can't resurrect this class of bug.)

## Warnings

### WR-01: Loop-crossfade LUT caps at 7/8 → residual discontinuity (click) at every loop wrap

**File:** `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp:37-47, 88-101`

**Issue:**
The 8-sample equal-power loop crossfade indexes an 8-entry LUT built at `x = i/8` for `i ∈ 0..7`:

```cpp
a[i] = equalPowerWeights ((float) i / 8.0f);   // 0, 1/8, ... 7/8  — never reaches 1.0
...
int xIdx = (int) std::floor (pos - fadeStart);  // pos ∈ [lpEnd-8, lpEnd) → xIdx ∈ 0..7
const auto& w = loopXfadeLut()[xIdx];
return outSample * w.first + inSample * w.second;
```

The crossfade runs over `[lpEnd-8, lpEnd)` and should reach full incoming weight at the wrap point
`pos == lpEnd`. But the largest LUT weight is `equalPowerWeights(7/8)` → outgoing weight
`cos(7/8·π/2) ≈ 0.195`, incoming `≈ 0.981`. At the sample where `pos` crosses `lpEnd`,
`wrapLoopPosition` snaps `pos` back near `lpStart` and the outgoing term (still ~0.195 of the loop
*tail*) vanishes instantly. The incoming/post-wrap signal is continuous, so the audible step is
`≈ 0.195 × tail_sample` — a small but real per-cycle click on sustained looped samples (the
sampler's primary use case). The integer LUT also quantizes the fade to 8 steps.

**Fix:** drive the crossfade with a *continuous* phase that reaches 1.0 at the wrap:

```cpp
const float x = (float) (pos - fadeStart) / 8.0f;   // 0 at lpEnd-8, → 1 at lpEnd
const auto  w = equalPowerWeights (x);              // cos/sin per sample — cheap
return outSample * w.first + inSample * w.second;
```

If per-sample `cos`/`sin` is a concern, keep a LUT but size it `[0..1]` inclusive (e.g. 9+ entries
with linear interpolation, or index `x*(LUT_N-1)`), so weight 1.0 is actually reachable.

### WR-02: `prevMap` destruction can free a `SampleMap` (and its audio buffers) on the audio thread

**File:** `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp:392, 682`

**Issue:**
`startNote` copies `currentMap` into local `prevMap` (correct — keeps the steal-tail source alive,
CR-04), then reassigns `currentMap` to the freshly atomic-loaded map. When `startNote` returns,
`prevMap`'s destructor runs on the **audio thread**. If this voice held the last reference to the
old map (typical immediately after a background ReplaceAll: the processor has already swapped its
slot, so only in-flight voices hold the old map), that destructor frees the entire `SampleMap` —
`std::vector<SampleCell>`, every `SampleVariant`, and every `shared_ptr<AudioBuffer<float>>` — a
non-deterministic `free()` of potentially hundreds of MB on the RT thread. In the common case
(steal within the same loaded library) `prevMap == currentMap` and no free occurs, so this only
bites on the reload boundary.

**Fix:** hand retired maps to a message-thread reaper instead of letting the voice drop the last
ref. E.g. push `prevMap` onto a lock-free SPSC "to-release" queue the message thread drains, or
have the processor retain the previous map in a bounded ring until all voices have re-snapshotted.
At minimum, document that the processor must outlive-hold the prior map until the next
message-thread tick so voices never own the last ref.

### WR-03: `computePlayRateForVariant` divides by `hostSR`; a zero rate propagates to an unbounded `wrapLoopPosition` loop

**File:** `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp:113-123, 104-111`

**Issue:**
`computePlayRateForVariant` returns `(desiredFreq / cellRefFreq) * (slotSR / hostSR)` with no
guard on `hostSR = getSampleRate()`. If `hostSR` is ever `0.0` (voice not yet given a playback
sample rate), `playRate` becomes `+Inf`; `pos += Inf` yields `Inf`, and `wrapLoopPosition`'s
`while (pos >= (double) lpEnd) pos -= (double) lpLen;` never terminates → audio-thread hang. Also
`cubicInterp`'s `(int) std::floor(Inf)` is UB. JUCE normally guarantees `prepareToPlay`/
`setCurrentPlaybackSampleRate` before `startNote`, so probability is low, but the failure mode is a
hard lockup rather than a glitch.

**Fix:** clamp defensively at the source:

```cpp
const double sr = (hostSR > 0.0) ? hostSR : 44100.0;
return (desiredFreq / cellRefFreq) * (slotSR / sr);
```

and/or bound `wrapLoopPosition` (`if (!std::isfinite(pos)) { pos = lpStart; return; }`).

## Info

### IN-01: Equal-power crossfade produces ~+3 dB bump for correlated layers

**File:** `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp:31-35` (used at 538, 940, 1035)

**Issue:** `equalPowerWeights` gives constant *power* (`cos²+sin² = 1`). Adjacent velocity layers
of the same note are highly correlated, so at the 50/50 point the summed amplitude is
`cos45+sin45 ≈ 1.414` → a ~+3 dB loudness bump through the crossfade region (both the
velocity-layer crossfade and the CC dynamic morph). Equal-*gain* (linear) weights avoid this for
correlated content but dip for uncorrelated content. If layers are near-identical in phase,
consider a correlation-aware or equal-gain blend; otherwise document the choice.

### IN-02: RR counter advanced for skipped (degenerate) layers in CC gather

**File:** `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp:618-646`

**Issue:** In the CC gather loop, `selectVariantIndex(*c, rrMode)` (line 624) mutates the cell's
persistent round-robin counter *before* the degenerate-variant skip (line 630). A layer whose
selected variant has an empty buffer still consumes an RR step, so the per-cell RR progression can
skew when a library has empty/failed variants. Cosmetic (round-robin ordering only), not audible
correctness. Consider selecting after validating, or restoring the counter on skip.

### IN-03: Stale "squared CC gain" comment vs. actual dB-linear single-layer path

**File:** `plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp:648-653` (and header note ~181)

**Issue:** The comment states a single populated layer "falls back to a squared CC gain on that
layer." The actual single-layer render (`renderCcCrossfade`, `single == true`) applies only the
v1.22 dB-linear `dynGain = decibelsToGain(rangeDb·(d−1))`, not a squared gain. Behavior is fine;
update the comment to match the shipped dB-ramp implementation to avoid misleading future edits.

---

_Reviewed: 2026-06-30_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
