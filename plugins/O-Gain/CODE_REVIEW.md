---
phase: O-Gain-code-review
reviewed: 2026-07-01T00:00:00Z
depth: deep
files_reviewed: 5
files_reviewed_list:
  - plugins/O-Gain/Source/PluginProcessor.h
  - plugins/O-Gain/Source/PluginProcessor.cpp
  - plugins/O-Gain/Source/PluginEditor.h
  - plugins/O-Gain/Source/PluginEditor.cpp
  - plugins/O-Gain/Source/ui/public/index.html
findings:
  critical: 2
  warning: 5
  info: 6
  total: 13
status: issues_found
---

# O-Gain: Code Review Report

**Reviewed:** 2026-07-01
**Depth:** deep
**Files Reviewed:** 5
**Status:** issues_found

## Summary

O-Gain is a stereo gain-staging utility with channel utilities (phase/swap/mono/M-S),
a WebView UI, and a BS.1770 "Learn" auto-gain feature. The parameter/relay/attachment
wiring is correct, member declaration order is right, the resource provider matches the
HTML asset paths, and the native-function bridge (`toggleLearn`) matches between JS
(`getNativeFunction`) and C++ (`withNativeFunction`). State save/load round-trips cleanly.

However, two real-time / safety defects must be fixed before ship:

1. **`finalizeLearn()` executes on the audio thread and calls
   `setValueNotifyingHost()`** — a non-RT-safe host/listener notification path invoked
   directly from `processBlock`.
2. **Learn on silence, near-silence, or a mono instance yields a measured level of
   -100 dB, which drives `gain_offset` straight to +40 dB** — an unexpected full-scale
   boost that is a loudness / hearing-safety hazard.

Several metering defects (frozen peak decay, dead-in-mono VU/learn, non-oversampled
"true peak" mislabeled as dBTP) degrade the feature but are not crash/safety class.

## Critical Issues

### CR-01: `finalizeLearn()` performs non-RT-safe host notification on the audio thread

**File:** `plugins/O-Gain/Source/PluginProcessor.cpp:678-683, 358-409`
**Issue:**
Learn stop is detected inside `processBlock` (audio thread):

```cpp
else if (!isLearnActive && prevLearnActive)
{
    finalizeLearn();               // <-- audio thread
    learnState.store(2, ...);
}
```

`finalizeLearn()` then calls:

```cpp
gainParam->setValueNotifyingHost(normalizedValue);
```

`setValueNotifyingHost()` synchronously notifies the host and all APVTS parameter
listeners (including the `WebSliderParameterAttachment`), a path that can lock and
allocate. Calling it from `processBlock` violates real-time safety and can cause
priority-inversion / dropouts. The whole `finalizeLearn` body (log10, gate loops over up
to 4000 elements) also runs on the audio thread.

**Fix:** Do not mutate the parameter or notify the host from the audio thread. Detect the
"learn just finished" edge on the audio thread, set an `std::atomic<bool> learnFinalizePending`,
and perform the measurement→gain computation and `setValueNotifyingHost()` on the message
thread (e.g. in the editor `Timer`, or an `AsyncUpdater`/`Timer` owned by the processor).
Snapshot the measured value into an atomic for the message thread to consume:

```cpp
// audio thread
else if (!isLearnActive && prevLearnActive)
{
    measuredLevelAtStop.store((float) computeMeasuredLevel(), std::memory_order_release);
    learnState.store(2, std::memory_order_release);
    learnFinalizePending.store(true, std::memory_order_release);
}
// message thread (timer): if learnFinalizePending exchange->false, compute gain + setValueNotifyingHost
```

---

### CR-02: Learn on silence / mono / near-silent input slams `gain_offset` to +40 dB

**File:** `plugins/O-Gain/Source/PluginProcessor.cpp:358-404, 687`
**Issue:**
When Learn measures no usable signal, `measuredLevel` becomes `-100.0`:

- LUFS path: `calculateIntegratedLUFS()` returns `-100.0` when `gatingBlockCount == 0`
  (silence, or **any mono instance** — accumulation at line 687 is gated on
  `numChannels >= 2`, so a mono track accumulates nothing).
- RMS path: `rmsSampleCount == 0` → `measuredLevel = -100.0`.

Then:

```cpp
double gainDB = targetLevel - measuredLevel;   // e.g. -18 - (-100) = +82 dB
// truePeakMax == 0 -> currentTruePeakDB == -100 -> maxSafeGain = -1 - (-100) = +99
gainDB = jlimit(-40.0, 40.0, gainDB);          // -> +40 dB
```

Result: pressing Learn over silence (or on a mono instance, or briefly over a very quiet
passage) and stopping sets the plugin to **+40 dB**, producing a sudden full-scale/blown
output — a genuine loudness and hearing-safety hazard, and data-destructive to a mix.

**Fix:** Guard against invalid/insufficient measurements — leave `gain_offset` unchanged
and surface a low/none confidence to the UI:

```cpp
const bool haveValidMeasurement =
    (learnMeasurementModeAtStart == 0 ? gatingBlockCount > 0 : rmsSampleCount > 0);

if (!haveValidMeasurement || measuredLevel <= -70.0)
{
    learnConfidence.store(0, std::memory_order_relaxed); // none / invalid
    return;                                              // do NOT touch gain_offset
}
```

Also require a minimum learn duration / block count before allowing a gain write.

## Warnings

### WR-01: Peak-meter decay uses a per-sample coefficient applied once per block — meters freeze

**File:** `plugins/O-Gain/Source/PluginProcessor.cpp:628-630, 872-874`
**Issue:**
```cpp
const float peakDecayRate = 1.0f - std::exp(-1.0f / (currentSampleRate * 0.3f)); // ~7e-5
inputPeakDecayL = juce::jmax(peakL, inputPeakDecayL * (1.0f - peakDecayRate));
```
`peakDecayRate` is a per-*sample* time constant (~300 ms), but the multiply is applied
once per *block*. Effective decay is ~0.007% per block (~0.6%/s at 512-sample blocks), so
the peak hold decays essentially never — the meter latches at its highest value and stops
falling. Behavior also varies with block size.

**Fix:** Raise the decay to a per-block coefficient, i.e. multiply the per-sample rate by
`numSamples`, or apply the per-sample decay `numSamples` times:
```cpp
const float perSample   = std::exp(-1.0f / (currentSampleRate * 0.3f));
const float blockDecay   = std::pow(perSample, (float) numSamples);
inputPeakDecayL = juce::jmax(peakL, inputPeakDecayL * blockDecay);
```

### WR-02: VU metering and Learn accumulation are silently dead on mono instances

**File:** `plugins/O-Gain/Source/PluginProcessor.cpp:645, 687`
**Issue:** STEP 3 (VU ballistics) and STEP 4 (Learn K-weight/LUFS accumulation) are both
gated on `numChannels >= 2`. On a mono instance: `vuLevelL/R` never update (VU meter dead),
and Learn accumulates nothing — which then feeds CR-02 (+40 dB). The mono branch comment at
lines 517-521 explicitly acknowledges the stereo utilities are skipped but nothing handles
metering/learn for mono.

**Fix:** Add a mono path that feeds channel 0 into `vuBallisticsL` and the K-weight/LUFS
accumulator (duplicating L into R, or averaging), so VU and Learn function in mono. At
minimum, combine with CR-02's guard so mono learn cannot write a bogus gain.

### WR-03: "True peak" is an un-oversampled digital peak but is labeled dBTP and used as a safety ceiling

**File:** `plugins/O-Gain/Source/PluginProcessor.cpp:697-701, 384-393`; `index.html:657`
**Issue:** `truePeakMax` is the max absolute *sample* value (no oversampling), yet it is
reported as `truePeakDBTP` and used to compute `maxSafeGain = ceiling - currentTruePeakDB`
with a -1 dBTP ceiling. Inter-sample peaks (which can exceed sample peaks by several dB) are
not detected, so the "-1 dBTP" guarantee can be violated after gain is applied. The UI
tooltip ("Highest inter-sample peak detected") over-promises.

**Fix:** Either implement true-peak via ≥4x oversampling (`juce::dsp::Oversampling`) before
peak detection, or relabel the metric as "Sample Peak / dBFS" in code and UI and add
headroom to the ceiling (e.g. -2 to -3 dB) to cover un-measured ISPs.

### WR-04: Integrated-LUFS dual-gate recomputed every 100 ms hop on the audio thread, cost grows with learn duration

**File:** `plugins/O-Gain/Source/PluginProcessor.cpp:789-791, 305-352`
**Issue:** During Learn, `calculateIntegratedLUFS()` runs on every hop (10x/sec) and loops
over all accumulated gating blocks twice (up to 4000). The per-hop audio-thread cost grows
linearly with learn length. While not O(n²), this is unbounded work on the real-time thread
tied to a user-controlled duration and risks late buffers on long learns.

**Fix:** The running integrated value is only needed for display; compute it at a lower rate
(e.g. once per second) or maintain incremental gated sums so the per-hop update is O(1).
Full recompute can be deferred to `finalizeLearn` (which, per CR-01, should run off the
audio thread anyway).

### WR-05: Metering snapshot sent to JS is read as ~17 independent atomics — cross-field tearing

**File:** `plugins/O-Gain/Source/PluginEditor.cpp:150-173`
**Issue:** `timerCallback` reads 17 atomics individually into one `updateMeters(...)` call.
Fields can come from different `processBlock` iterations (e.g. `learnState` from block N,
`learnConfidence`/`integratedLUFS` from N-1), producing transiently inconsistent UI states
(e.g. "DONE" shown with a stale/`--` integrated value). Non-crashing but visibly jittery for
the Learn panel.

**Fix:** For the Learn panel specifically, publish a single seqlock-guarded struct or a
generation counter the UI checks, so the displayed learn result is internally consistent.
Plain peak/RMS meters can remain independent atomics.

## Info

### IN-01: Dead/empty branch in `toggleLearn` native function

**File:** `plugins/O-Gain/Source/PluginEditor.cpp:76-81`
**Issue:** The `if (!newState && processorRef.learnState.load() == 1) { /* comment only */ }`
block does nothing. Remove it or replace with the actual intent; as written it is dead code.

### IN-02: `learnState` never returns to idle (0)

**File:** `plugins/O-Gain/Source/PluginProcessor.cpp:682`
**Issue:** After a learn completes, `learnState` stays `2` until the next learn starts (`1`);
it is never reset to `0`. The button therefore reads "DONE" indefinitely. Intentional per the
UI, but there is no path to clear the completed state (e.g. on parameter edit), which may
confuse users after they manually adjust gain. Consider clearing to idle on manual
`gain_offset`/`trim` change.

### IN-03: Meter mode "LUFS" (index 3) displays RMS, not loudness

**File:** `plugins/O-Gain/Source/ui/public/index.html:976-981`
**Issue:** Selecting the LUFS meter mode shows input/output RMS as a fallback. The mode label
promises LUFS the meter never delivers (momentary LUFS is only computed during Learn).
Either drive the meter from `momentaryLUFS` when available or relabel the option.

### IN-04: `withBackend(Backend::webview2)` hard-coded without a platform guard

**File:** `plugins/O-Gain/Source/PluginEditor.cpp:45`
**Issue:** `webview2` is a Windows backend; it is requested unconditionally. It is ignored on
macOS (WebKit is always used), so this is currently harmless, but it is a cross-platform smell
and couples the editor to a Windows-only enum. Guard with `#if JUCE_WINDOWS` or leave as
`defaultBackend` and select via the platform-specific options block already present.

### IN-05: M/S "Decode" alone applies +6 dB relative to "Encode"

**File:** `plugins/O-Gain/Source/PluginProcessor.cpp:561-574`
**Issue:** Encode uses `*0.5`, Decode uses full-scale sum/difference. Encode→Decode round-trips
to unity (correct), but selecting Decode on a normal L/R signal boosts by +6 dB. This is a
valid convention but is a likely user-surprise; document it in the UI tooltip or apply matched
`*0.5`/`*0.5` scaling on both sides plus a compensating output trim.

### IN-06: Magic numbers throughout DSP/metering

**File:** `plugins/O-Gain/Source/PluginProcessor.cpp` (e.g. 430 `0.02`, 440 `300.0f`, 469-470
`0.4`/`0.1`, 474 `4000`, 628 `0.3f`, 813-818 confidence thresholds); `index.html:949-950`
(-60..0 meter range), `998-1001` (-0.5 clip threshold)
**Issue:** Numerous unlabeled constants (ramp time, VU ballistics, LUFS block/hop, gating
capacity, decay, confidence thresholds, meter range) reduce maintainability. Promote to named
`constexpr` constants with unit comments.

---

_Reviewed: 2026-07-01_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
