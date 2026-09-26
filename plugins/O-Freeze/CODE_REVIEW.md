---
phase: O-Freeze code review
reviewed: 2026-07-01T00:00:00Z
depth: deep
files_reviewed: 6
files_reviewed_list:
  - plugins/O-Freeze/Source/PluginProcessor.cpp
  - plugins/O-Freeze/Source/PluginProcessor.h
  - plugins/O-Freeze/Source/PluginEditor.cpp
  - plugins/O-Freeze/Source/PluginEditor.h
  - plugins/O-Freeze/Source/ui/public/index.html
  - plugins/O-Freeze/CMakeLists.txt
findings:
  critical: 1
  warning: 4
  info: 5
  total: 10
status: issues_found
resolved: [CR-01, WR-01, WR-02, WR-03, WR-04]
resolved_in: v2.0.1
resolved_date: 2026-07-01
deferred: [IN-01, IN-02, IN-03, IN-04, IN-05]
---

# O-Freeze: Code Review Report

**Reviewed:** 2026-07-01
**Depth:** deep
**Files Reviewed:** 6
**Status:** issues_found

> **Resolution (v2.0.1, 2026-07-01):** CR-01, WR-01, WR-02, WR-03, WR-04 fixed. See
> CHANGELOG. IN-01…IN-05 deferred as non-blocking.

## Summary

O-Freeze is a WSOLA overlap-add granular freeze effect with a WebView UI. The RT-safety
substrate is largely sound: all per-grain scratch arrays (`windowValues`, `grainPos0/1`,
`grainFrac`) are fixed `MAX_GRAINS` members, `grains[]` is a `std::array<Grain,32>`, and
`hannWindow`/`rmsBuffer`/`freezeBuffer` are resized only in `prepareToPlay`. No heap
allocation, lock, or file I/O was found in `processBlock`. Circular-buffer index math on
`freezeBuffer` was traced and is out-of-bounds-safe: every negative-risk expression is
biased by `+ freezeBufferLength` or `+ freezeBufferLength * 2` before `%`, and the 2-second
buffer (384,000 samples) generically dominates the worst-case sum of `position + intPart +
jitter + drift`. `hannWindow[startSample]` cannot overrun because grains deactivate at
`startSample >= grainSize` and `grainSize` is `jmin`-clamped to `maxGrainSize == hannWindow.size()`.

The WebView bridge is clean: all 12 relays registered in `PluginEditor.cpp` map 1:1 to the
12 `getSliderState/getToggleState/getComboBoxState` calls in `index.html`, there are zero
`getNativeFunction`/`withNativeFunction` pairs to desync, and CMake correctly sets
`NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`. No dead controls.

The defects are concentrated in DSP correctness and RT budgeting: an audio-thread work
spike that scales with grain size (CR-01), an incorrect multichannel RMS window (WR-01),
a fade-glitch from `LinearSmoothedValue::reset` snapping (WR-02), and a freeze buffer being
overwritten during the release fade (WR-03).

## Critical Issues

### CR-01: WSOLA search performs large synchronous work on the audio thread, scaling with grain size

**File:** `plugins/O-Freeze/Source/PluginProcessor.cpp:485-529`

**Issue:** On every grain trigger, the WSOLA cross-correlation search runs
`searchRange = grainSize / 4` (line 488) and iterates `offset` from `-searchRange` to
`+searchRange` in steps of 4 (line 497), and for each offset runs a `WSOLA_TAIL_SIZE (64)` ×
`numChannels` inner loop with a `freezeBuffer.getReadPointer(ch)[samplePos]` fetch and a
`std::sqrt` (lines 503-518). At the maximum grain size (1000 ms → `grainSize ≈ 192,000` at
192 kHz), `searchRange ≈ 48,000`, giving `(2·48,000/4) × 64 × 2 ≈ 3.1 million` multiply-adds
plus 24,000 `sqrt` calls executed **inside a single sample iteration** of `processBlock`.
This is a synchronous per-callback spike, not amortized. At small host buffer sizes (64–128
samples) this will blow the audio deadline and produce xruns/dropouts — an audible failure,
and RT-safety is this codebase's #1 invariant. The cost is technically bounded (fixed max),
but the bound is far beyond a realtime budget.

**Fix:** Cap the search span independent of grain size (e.g. `searchRange = jmin(grainSize/4,
512)`), and/or decimate the tail correlation. Better: precompute nothing per-sample — the
search only needs to happen once per grain trigger, so hoist it out of the hot path by
bounding the range to a musically-meaningful window (a few ms), e.g.:
```cpp
const int maxSearch = static_cast<int>(currentSampleRate * 0.005); // 5 ms cap
int searchRange = juce::jmin(grainSize / 4, maxSearch);
```

## Warnings

### WR-01: RMS threshold window is the wrong duration for multichannel input and scrambles L/R

**File:** `plugins/O-Freeze/Source/PluginProcessor.cpp:304-322`

**Issue:** The RMS detector writes *both* channels serially into one circular `rmsBuffer`
sized to `rmsSamplesPerWindow = sampleRate * 0.020` (20 ms), advancing `rmsWriteIndex` once
per sample **per channel** (lines 305-315). For stereo, `2 × numSamples` values are pushed
per block into a buffer that only holds 20 ms worth of single-channel samples, so the
effective window is ~10 ms (not 20 ms), and the buffer contents are `[ch0 block…, ch1
block…]` — L and R are temporally interleaved out of order rather than summed per time
instant. The detected level is therefore both time-scale-wrong and channel-scrambled, making
threshold auto-freeze trigger inconsistently versus the documented 20 ms intent.

**Fix:** Sum channels into a mono value per sample *before* pushing one squared value per
time instant:
```cpp
for (int sample = 0; sample < numSamples; ++sample) {
    float mono = 0.0f;
    for (int ch = 0; ch < numChannels; ++ch) mono += buffer.getReadPointer(ch)[sample];
    mono /= juce::jmax(1, numChannels);
    rmsBuffer[rmsWriteIndex] = mono * mono;
    rmsWriteIndex = (rmsWriteIndex + 1) % rmsSamplesPerWindow;
}
```

### WR-02: freezeGain.reset() snaps the gain to its target, discarding an in-progress fade → click on rapid toggle

**File:** `plugins/O-Freeze/Source/PluginProcessor.cpp:400-401, 444-445`

**Issue:** `juce::LinearSmoothedValue::reset(sampleRate, rampTime)` calls
`setCurrentAndTargetValue(target)` internally (verified in JUCE
`juce_SmoothedValue.h:277`) — it forces `currentValue = target`. When freeze is toggled
while a fade is still in progress (e.g. re-engage during the ~1 s release fade), line 400
`freezeGain.reset(...)` snaps `currentValue` to the *old* target (0.0) before line 401
`setTargetValue(1.0f)` starts a fresh ramp from 0. The partial fade level is lost, producing
an amplitude discontinuity (click) exactly in the rapid-toggle case a freeze pedal invites.

**Fix:** Don't `reset()` to change ramp time mid-fade. Set the ramp length once in
`prepareToPlay` and only call `setTargetValue()` in `processBlock`, or snapshot and restore:
```cpp
float held = freezeGain.getCurrentValue();
freezeGain.reset(currentSampleRate, 0.050);
freezeGain.setCurrentAndTargetValue(held);
freezeGain.setTargetValue(1.0f);
```

### WR-03: Freeze buffer is overwritten by live input during the release fade while grains still read it

**File:** `plugins/O-Freeze/Source/PluginProcessor.cpp:436-449, 600-676`

**Issue:** On freeze release (line 436) `bufferFrozen` becomes `false` but grains keep playing
for the fade-out (`freezeGain.getCurrentValue() > 0.001f`, line 606) with a ramp of up to
`grainDurationSec + 0.050` (≈1.05 s at max grain size, line 444). Because `bufferFrozen`
is now false, line 600-602 resumes writing live input into `freezeData[writePosition]` and
line 673-675 resumes advancing `writePosition`. During a long fade the write head advances
through and overwrites the very region the still-active grains are reading (grains read
around `writePosition - grainSize`), so the release tail reads a buffer being mutated under
it — glitchy/corrupted fade-out for large grain sizes.

**Fix:** Freeze the write head for the duration of the release tail. Gate the write/advance
on "not currently rendering grains" rather than `bufferFrozen` alone, e.g. keep a
`bool renderingTail = freezeGain.getCurrentValue() > 0.001f || bufferFrozen;` and suppress
buffer writes while `renderingTail` is true.

### WR-04: prepareToPlay computes grainSize without the maxGrainSize clamp used in processBlock

**File:** `plugins/O-Freeze/Source/PluginProcessor.cpp:171` (vs. clamped form at line 271)

**Issue:** `prepareToPlay` does `grainSize = static_cast<int>(sampleRate * grainSizeMs /
1000.0)` with **no** `jmin(..., maxGrainSize)`, whereas `processBlock` (line 271) clamps.
The initial Hann-window build loop `for (int i = 0; i < grainSize; ++i) hannWindow[i] = …`
(lines 176-180) writes `hannWindow` sized to `maxGrainSize`. This is currently benign only
by coincidence: `maxGrainSize = sampleRate * 1.0` and the GRAIN_SIZE parameter max is exactly
1000 ms, so `grainSize == maxGrainSize` at the extreme. If the GRAIN_SIZE range is ever
raised above 1000 ms (or `maxGrainSize` lowered), this becomes an out-of-bounds heap write on
the first restored preset. A latent OOB gated only by two constants staying in sync.

**Fix:** Mirror the clamp for defense-in-depth:
```cpp
grainSize = juce::jmin(static_cast<int>(sampleRate * grainSizeMs / 1000.0), maxGrainSize);
```

## Info

### IN-01: getRawParameterValue called with string IDs every processBlock

**File:** `plugins/O-Freeze/Source/PluginProcessor.cpp:247-254, 341-347`

**Issue:** ~14 parameters are re-resolved by string ID on every audio callback. JUCE's
`getRawParameterValue` is a HashMap lookup (no allocation/lock, so RT-safe), but it is
avoidable churn on the hot path.

**Fix:** Cache the `std::atomic<float>*` pointers once in the constructor / `prepareToPlay`
and read `->load()` in `processBlock`.

### IN-02: Value-edit input box rounds all non-dB parameters to integers

**File:** `plugins/O-Freeze/Source/ui/public/index.html:917`

**Issue:** `input.value = config.unit === 'dB' ? currentValue.toFixed(1) : currentValue.toFixed(0)`
means double-click-to-type shows and (via `parseFloat`) round-trips only integer precision for
LFO Rate (0.01–10 Hz), Grain Size (…ms), Detune (…ct), and % params. Typing "0.5" Hz works,
but the pre-filled value for 0.5 Hz displays as "1", and Detune's "5.0 ct" display can't be
matched in the editor. Precision/UX mismatch versus the display formatters.

**Fix:** Choose decimals per-parameter (e.g. reuse the `formatValue` precision), or use
`toFixed(2)` for sub-unit ranges like LFO Rate and Detune.

### IN-03: Round-robin grain-slot reuse can restart a still-active grain

**File:** `plugins/O-Freeze/Source/PluginProcessor.cpp:476, 543`

**Issue:** `grains[nextGrainIndex]` is overwritten on trigger and `nextGrainIndex` advances
`% numGrains`. With jittered trigger intervals (line 547-548) and per-grain detune
(line 539) altering completion timing, a slot's grain may not have finished when its turn
comes back around, causing an abrupt mid-grain restart (click). This is an inherent
fixed-voice-pool tradeoff, noted for awareness.

**Fix:** Optionally skip triggering onto an active slot, or size the pool with headroom over
`numGrains` and pick the least-recently-active free slot.

### IN-04: mono /= numChannels has no zero guard

**File:** `plugins/O-Freeze/Source/PluginProcessor.cpp:512, 663**

**Issue:** If a host ever reports 0 output channels, `numChannels` becomes 0 and the WSOLA /
tail-capture `mono /= static_cast<float>(numChannels)` divides by zero → NaN propagating into
`freezeBuffer` reads. Practically unreachable for the declared stereo bus, but unguarded.

**Fix:** `mono /= static_cast<float>(juce::jmax(1, numChannels));` (and early-out when
`numChannels == 0`).

### IN-05: Logger::writeToLog in the resource provider

**File:** `plugins/O-Freeze/Source/PluginEditor.cpp:179`

**Issue:** `juce::Logger::writeToLog("Resource not found: " + url)` on the message thread for
every unmatched URL is log noise (browsers probe favicon.ico, sourcemaps, etc.). Harmless but
not RT-relevant; consider gating behind `JUCE_DEBUG` or removing.

---

_Reviewed: 2026-07-01_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
