---
phase: O-MultiBandCompressor-code-review
reviewed: 2026-07-01T00:00:00Z
depth: deep
version_reviewed: 1.2.0
files_reviewed: 11
files_reviewed_list:
  - plugins/O-MultiBandCompressor/Source/PluginProcessor.h
  - plugins/O-MultiBandCompressor/Source/PluginProcessor.cpp
  - plugins/O-MultiBandCompressor/Source/PluginEditor.h
  - plugins/O-MultiBandCompressor/Source/PluginEditor.cpp
  - plugins/O-MultiBandCompressor/Source/DSP/MultiBandProcessor.h
  - plugins/O-MultiBandCompressor/Source/DSP/CrossoverNetwork.h
  - plugins/O-MultiBandCompressor/Source/DSP/Compressor.h
  - plugins/O-MultiBandCompressor/Source/DSP/EnvelopeDetector.h
  - plugins/O-MultiBandCompressor/Source/DSP/GainComputer.h
  - plugins/O-MultiBandCompressor/Source/ui/public/js/app.js
  - plugins/O-MultiBandCompressor/CMakeLists.txt
findings:
  critical: 3
  warning: 4
  info: 6
  total: 13
status: issues_found
---

# O-MultiBandCompressor: Code Review Report

**Reviewed:** 2026-07-01
**Depth:** deep
**Version:** 1.2.0 (📦 Installed)
**Status:** issues_found

## Summary

O-MultiBandCompressor is a 4-band feed-forward compressor: Linkwitz-Riley 24 dB/oct
crossovers → per-band compressor (soft knee, peak/RMS blend, sidechain HPF/LPF, solo,
bypass), M/S modes, parallel dry/wet, auto-makeup, and a 2048-pt FFT analyzer. The
architecture is sound and the parts that are usually fragile are correct: editor member
order (relays → webView → attachments), the resource provider, the 56-parameter
relay/attachment wiring, the soft-knee gain-computer (continuous across the knee),
state save/load, and the CMake Windows-WebView2 flags (`NEEDS_WEBVIEW2 TRUE` +
`JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1`) are all correct. All UI element IDs
resolve, so meters and crossover drag are live.

The defects cluster in one theme: **the audio thread allocates memory and does heavy
work every block.** Three real-time-safety violations (CR-01..03) should be fixed
before this is trusted under load — heap allocation on the RT thread is a dropout/xrun
hazard. Beyond that, one thread-safety issue (mutex on the audio thread), a –6 dB
detection error in M/S Mid/Side modes, a crossover flatness issue, and an attack/release
readout mismatch round out the list.

None of these crash; the plugin works. They are correctness/robustness/quality issues.

## Critical Issues

### CR-01: Crossover filters are redesigned from scratch on the audio thread every block

**File:** `Source/PluginProcessor.cpp:375` → `Source/DSP/CrossoverNetwork.h:61-108`
**Issue:**
`processBlock` calls `multibandProcessor.updateCrossoverFrequencies(...)` unconditionally
every block, which calls `CrossoverNetwork::updateCoefficients()`, which runs:

```cpp
auto lp1Coeffs = juce::dsp::FilterDesign<float>::designIIRLowpassHighOrderButterworthMethod(
    xover1Hz, currentSampleRate, 2);   // ×6 (3 crossovers × LP+HP), every block
```

Each `designIIR...ButterworthMethod` call **heap-allocates** a `ReferenceCountedArray`
and runs trig-heavy pole/zero design math — 6× per block, on the audio thread, *even
when the XOVER parameters have not moved*. Heap allocation on the RT thread is the
canonical real-time-safety violation (can block on the allocator → xruns), and the
redundant design work is also a large, pointless CPU cost (~4k filter designs/sec at
64-sample blocks).

**Fix:** Cache the last `xover1/2/3` and early-out when unchanged; when they do change,
design into preallocated `Coefficients::Ptr` objects and swap (RT-safe ref-count op),
or throttle/redesign off the audio thread. This is the same class of bug documented in
[[critical_oversampled_path_filter_rate]] (precompute coeffs, swap on change).

### CR-02: 48 `juce::String` allocations per block reading per-band parameters

**File:** `Source/PluginProcessor.cpp:321-337`
**Issue:**
The per-band parameter read loop builds the parameter ID as a runtime string every block:

```cpp
juce::String prefix = bandPrefixes[band];                                   // alloc
thresholds[band] = parameters.getRawParameterValue(prefix + "_THRESHOLD")->load();  // alloc
ratios[band]     = parameters.getRawParameterValue(prefix + "_RATIO")->load();      // alloc
... // 12 concatenations × 4 bands = ~52 heap-allocating juce::String temporaries/block
```

`juce::String` has no small-string optimization, so each `prefix + "_..."` concatenation
heap-allocates, and each is followed by a hashed map lookup. This runs unconditionally,
every block — another RT-safety violation.

**Fix:** Resolve the 48 `std::atomic<float>*` raw pointers **once** (constructor or
`prepareToPlay`) into a `[band][paramKind]` table using compile-time string-literal IDs,
then index that table in `processBlock`. Same pattern the editor already uses in
`sendCrossoverPositions` (cached `getRawParameterValue`).

### CR-03: Heap-allocated `AudioBuffer` on the audio thread in M/S Mid & Side modes

**File:** `Source/PluginProcessor.cpp:412, 436`
**Issue:**
When `MS_MODE` is Mid or Side, `processBlock` allocates a fresh buffer every block:

```cpp
juce::AudioBuffer<float> midBuffer(1, numSamples);   // heap alloc + free, every block
...
juce::AudioBuffer<float> sideBuffer(1, numSamples);  // heap alloc + free, every block
```

RT-safety violation, active whenever the user selects Mid or Side mode.

**Fix:** Preallocate a mono scratch `AudioBuffer` in `prepareToPlay` sized to
`maximumBlockSize` and `setSize(1, numSamples, keepExisting=false, clearExtra=false,
avoidReallocating=true)` (or just reuse a fixed-size buffer and copy `numSamples`).

## Warning Issues

### WR-01: `std::mutex` locked on the audio thread for spectrum publish

**File:** `Source/PluginProcessor.cpp:568-572` (writer, audio thread) and `577-581`
(reader, UI thread)
**Issue:**
The FFT result is published under a `std::lock_guard<std::mutex>` inside `processBlock`;
`getSpectrumData()` locks the same mutex on the message thread. If the UI thread is
preempted while holding the lock, the audio thread blocks on it → priority inversion and
a possible dropout. Locking on the audio thread is unsafe even for a short critical
section.

**Fix:** Use a lock-free hand-off — an SPSC/double-buffer with an atomic index flip, or
at minimum `try_lock` on the audio side and skip the frame if contended (the analyzer is
non-critical). The v1.2.0 CHANGELOG advertises "mutex-protected thread-safe" as a
feature; it is thread-*correct* but not RT-safe.

### WR-02: M/S Mid & Side modes under-detect by ~6 dB (compress too little)

**File:** `Source/PluginProcessor.cpp:412-431, 436-455`; `Source/DSP/Compressor.h:104-112`;
`Source/DSP/MultiBandProcessor.h:42`
**Issue:**
`bandBuffers` are sized to `getTotalNumOutputChannels()` (= 2) in `prepare`. In Mid/Side
modes the mono `midBuffer`/`sideBuffer` is split into those 2-channel band buffers, so
channel 1 is silent. `Compressor::processStereo` then averages *both* channels for
detection:

```cpp
for (int channel = 0; channel < numChannels; ++channel)   // numChannels == 2
    detectorInput += buffer.getSample(channel, sample);
detectorInput /= static_cast<float>(numChannels);          // (signal + 0) / 2  → −6 dB
```

The detector sees half the true level, so Mid and Side modes apply visibly less gain
reduction than Off/Both for identical threshold/ratio settings. A user A/B-ing modes
will hear the compression "loosen" when switching to Mid or Side.

**Fix:** Detect over active channels only (skip silent channels), or run the mono M/S
path through mono band buffers so `numChannels == 1` in the detector average.

### WR-03: Serial crossover has no all-pass compensation → non-flat sum at rest

**File:** `Source/DSP/CrossoverNetwork.h:113-173`; comment `Source/DSP/MultiBandProcessor.h:136`
**Issue:**
The crossover splits serially (LOW = LP@f1; the remainder is high-passed then split again
at f2, f3). The LOW band accumulates only the f1 phase while the upper bands accumulate
additional phase from f2 and f3. Summing the four bands therefore produces magnitude
ripple around each crossover (worst near f1) **even with every compressor bypassed** —
the plugin is not transparent at unity. The code comment "Linkwitz-Riley guarantees flat
magnitude" holds for a single 2-way LR split, not for this cascaded 4-way topology.

**Fix:** Apply all-pass compensation to the lower bands (each lower band passes through
all-passes matching the higher crossovers), or restructure as a phase-matched
tree/parallel bank. Quality/transparency defect, not a crash — but it is the kind of
thing mastering users measure.

### WR-04: Attack/Release value readouts don't match the actual parameter value

**File:** `Source/ui/public/js/app.js:91-100` (display) vs `664-671` (correct skew math)
**Issue:**
The APVTS Attack/Release ranges use skew 0.3, so JUCE maps
`value = min + (max−min)·norm^(1/skew)`. But the slider's display formatter uses a *pure
log* interpolation instead:

```js
const ms = Math.pow(10, norm*(Math.log10(200)-Math.log10(0.1)) + Math.log10(0.1)); // 0.1·(200/0.1)^norm
```

At `norm = 0.5` this label reads **~4.5 ms** while the DSP actually runs **~20 ms**. The
crossover code (`freqToNormalized`/`normalizedToFreq`) already uses the correct
`pow(norm, 1/skew)` inverse — only the Attack/Release readouts (both skewed) are wrong.
Threshold/Ratio/Knee/Makeup are linear (skew 1.0) and display correctly.

**Fix:** Derive the displayed value from the same skew-aware mapping (or read the
denormalized value from the slider state) so the label matches the DSP.

## Info / Minor

### IN-01: Per-sample `getSample`/`setSample` in the crossover and compressor hot loops
`CrossoverNetwork::processSplit` and `Compressor::processStereo` use bounds-checked
`getSample`/`setSample` per sample/channel. Caching `getReadPointer`/`getWritePointer`
(as the FFT loop already does) is faster and idiomatic. Perf only.

### IN-02: Attack/Release coefficients recomputed every block
`setAttackTimes`/`setReleaseTimes` (`MultiBandProcessor.h:148-162`) are called
unconditionally each block, and `Compressor::setAttackTime` recomputes *both* `exp()`
coefficients every call (~8 `exp` per block) even when unchanged. Cache last values,
recompute on change.

### IN-03: `inputGain`/`outputGain` use instant (unramped) gain
`Source/PluginProcessor.cpp:342, 500` — `juce::dsp::Gain` defaults to no ramp, so
automating Input/Output Gain can zipper. Consider `setRampDurationSeconds(~0.02)`.

### IN-04: Dead members
`EnvelopeDetector::peakValue`/`rmsValue` (self-labeled "not used"),
`MultiBandProcessor::maxSamplesPerBlock`/`channelCount`, and the file-scope `spec` are
unused. Remove to reduce noise.

### IN-05: Resource provider matches on basename only
`Source/PluginEditor.cpp:225-233` strips the directory and matches the filename, so two
assets sharing a basename in different folders would collide. Currently unique (latent
risk only) — match on the full relative path to be safe.

### IN-06: Spectrum analyzer uses linear bin→X mapping under a log-styled grid
`Source/PluginProcessor.cpp:542-565` downsamples 1024 FFT bins to 64 by *linear* grouping
and linearly averages magnitude before dB conversion, while the UI grid/labels imply log
frequency. Low frequencies are compressed into a sliver and the average smears energy.
Cosmetic/analyzer-fidelity only.

## What's Correct (verified)

- Editor member declaration order: relays → webView → attachments ✓
- Resource provider path handling + MIME types + navigation in `parentHierarchyChanged` ✓
- All 56 relays registered and attached; all referenced UI element IDs exist ✓
- Soft-knee gain computer is continuous at both knee boundaries ✓
- RMS detector uses a running-sum sliding window (O(1) per sample) ✓
- Linkwitz-Riley = 2 cascaded 2nd-order Butterworth (correct 4th-order build) ✓
- State save/load round-trips via APVTS XML ✓
- CMake: `NEEDS_WEBVIEW2 TRUE` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` (Windows-ready) ✓
- Crossover drag skew math (`pow(norm, 1/0.3)`) matches JUCE's `NormalisableRange` ✓

## Suggested Fix Order

1. **CR-01, CR-02, CR-03** — RT-safety; one focused pass (cache coeffs, cache param
   pointers, preallocate M/S scratch). Highest value.
2. **WR-01** — swap the spectrum mutex for a lock-free hand-off.
3. **WR-02** — M/S detection −6 dB (audible correctness).
4. **WR-04** — attack/release readout (quick JS fix).
5. **WR-03** — crossover all-pass compensation (larger DSP change; schedule separately).
6. **IN-\*** — sweep opportunistically.
