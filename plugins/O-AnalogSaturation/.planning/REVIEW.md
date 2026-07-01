---
phase: O-AnalogSaturation-code-review
reviewed: 2026-07-01T04:07:44Z
depth: deep
files_reviewed: 5
files_reviewed_list:
  - plugins/O-AnalogSaturation/Source/PluginProcessor.cpp
  - plugins/O-AnalogSaturation/Source/PluginProcessor.h
  - plugins/O-AnalogSaturation/Source/PluginEditor.cpp
  - plugins/O-AnalogSaturation/Source/PluginEditor.h
  - plugins/O-AnalogSaturation/Source/ui/public/index.html
findings:
  critical: 3
  warning: 5
  info: 3
  total: 11
status: all_resolved
resolution:
  fixed_in: v1.1.4
  fixed: [CR-01, CR-02, CR-03, IN-01]
  open: []
  fixed_in_v1_1_5: [WR-01, WR-02, WR-03, WR-04, WR-05, IN-02, IN-03, "CR-01 MAGNETIC rate addendum"]
---

# O-AnalogSaturation: Code Review Report

**Reviewed:** 2026-07-01T04:07:44Z
**Depth:** deep
**Files Reviewed:** 5
**Status:** issues_found

## Summary

Reviewed the DSP core, APVTS wiring, oversampling/latency handling, and the WebView editor bridge for O-AnalogSaturation v1.1.3 (shipped). The code is structurally clean and RT-conscious in the obvious places (ScopedNoDenormals, atomic parameter reads, explicit NaN/denormal guards in the Jiles-Atherton model). However, deep tracing of the signal path surfaced three correctness defects that affect the audible behavior of every shipped configuration:

1. All per-model tone filters are designed for the base sample rate but executed inside the oversampled path, so at the default MID quality every EQ contour sits an octave below its intended frequency (two octaves at HIGH).
2. The auto-gain envelope smoothing coefficient is a per-sample constant applied once per block, yielding an effective time constant on the order of tens of seconds instead of 100 ms — the feature effectively does not track program material, and its behavior varies with host block size.
3. RMS math divides by `numSamples` with no zero guard; a single 0-sample `processBlock` call (which some hosts issue) permanently poisons the RMS envelopes with NaN.

Warnings cover an audio-thread `setLatencySamples` call, dry-signal coloration through the oversampler, block-boundary gain stepping, a missing bus-layout guard, and a mis-declared tail length. The WebView bridge is correct (event namespace usage is right, relays/attachments ordered safely).

## Structural Findings (fallow)

No `<structural_findings>` block was provided with this review; none to report.

## Narrative Findings (AI reviewer)

## Critical Issues

### CR-01: Per-model tone filters designed at base rate, run at oversampled rate — EQ contours shift an octave (or two) with Quality

> **RESOLVED in v1.1.4** — per-Quality coefficient sets designed at base·1x/2x/4x, active set swapped on Quality change (RT-safe Ptr swap). See CHANGELOG.


**File:** `plugins/O-AnalogSaturation/Source/PluginProcessor.cpp:94-149` (design) and `:259`, `:480-483`, `:536`, `:655-656` (execution)

**Issue:** In `prepareToPlay`, every model's IIR filter is designed against the host `sampleRate`:
- Transformer LF bump 60 Hz / HF sheen 8 kHz (lines 94-99)
- Tube presence 3 kHz (lines 116-117)
- Magnetic head bump 80 Hz / HF rolloff 12 kHz (lines 134-139)

But for MID and HIGH quality, `processBlock` upsamples the buffer (line 258) and calls `processSaturationBlock` (line 259), which invokes `processSample` → the model routines that run these very filters (`transformerLFBumpFilters[channel].processSample`, etc.) on data now clocked at 2x/4x. An IIR designed for `fs` and fed samples at `2·fs` has all of its corner frequencies halved. Result:
- At MID (2x, the shipped default): 60 Hz bump → 30 Hz, 8 kHz sheen → 4 kHz, 3 kHz presence → 1.5 kHz, 12 kHz rolloff → 6 kHz.
- At HIGH (4x): all of the above shifted two octaves down.
- Only LOW quality (no oversampling, `processSaturationDirect`, line 249) produces the intended response.

So the plugin's tonal character audibly changes when the user switches Quality, and the default preset ships with every EQ an octave off. This is a correctness defect, not a nuance.

**Fix:** Design the model filters at the rate they actually run at, or move them out of the oversampled nonlinear path. Cleanest: apply the tone EQ at base rate after `processSamplesDown`. If they must stay inline, maintain per-quality coefficient sets and select by active quality:
```cpp
// In prepareToPlay, design each filter against the oversampled rate used by the
// path that executes it (base rate for LOW, sampleRate*2 for MID, *4 for HIGH),
// and swap the active coefficient set when quality changes — mirroring the
// oversampler selection in processBlock.
const double osRate = sampleRate * (currentQuality == 2 ? 4.0 : currentQuality == 1 ? 2.0 : 1.0);
auto lfBumpCoeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(osRate, 60.0f, 0.7f, ...);
```
Note the same per-sample statefulness applies to the Jiles-Atherton integrator (`M += dM_dH * deltaH`, line 635) and the `deltaH` clamp (±0.3, line 589): those are inherently rate-sensitive too and should be reviewed alongside the filter fix so MAGNETIC sounds consistent across Quality.

### CR-02: Auto-gain envelope coefficient is per-sample but applied per-block — effective time constant is tens of seconds, not 100 ms

> **RESOLVED in v1.1.4** — coefficient now derived per block from actual block length, `exp(-N/(0.1·fs))`. See CHANGELOG.


**File:** `plugins/O-AnalogSaturation/Source/PluginProcessor.cpp:158`, `:323`, `:394`

**Issue:** `autoGainCoeff = exp(-1 / (0.1 * sampleRate))` (line 158) is a **per-sample** one-pole coefficient (≈0.999792 at 48 kHz). But the envelope update runs **once per block**, outside the sample loop:
```cpp
// captureInputRMS (line 323) — executed once per processBlock:
inputRMSEnvelope[channel] = autoGainCoeff * inputRMSEnvelope[channel] + (1.0f - autoGainCoeff) * rms;
```
With one update per ~512-sample buffer, the realized time constant is `blockDuration / (1 - coeff)` ≈ `10.7 ms / 0.000208` ≈ **51 seconds** at 48 kHz/512, versus the intended 100 ms. It also scales with host block size and sample rate, so identical audio yields different auto-gain behavior across hosts. In practice the compensation gain barely moves and never tracks program dynamics — the feature is broken (it is off by default, which has masked this).

**Fix:** Either compute the coefficient for a per-block cadence, or update the envelope per sample. Per-block version:
```cpp
// Time constant expressed in blocks, recomputed whenever block size changes:
const float blocksPerTau = (timeConstantSeconds * sampleRate) / (float) samplesPerBlock;
autoGainCoeff = std::exp(-1.0f / blocksPerTau);
```
Prefer recomputing in `prepareToPlay` from the actual `samplesPerBlock` (and clamp for tiny/huge blocks). If per-sample tracking is desired, move the envelope recursion inside the sample loop.

### CR-03: Division by zero / NaN poisoning when processBlock receives a zero-length buffer

> **RESOLVED in v1.1.4** — early-out on `numSamples <= 0` in both RMS routines; flush guard hardened to `!std::isfinite(env) || env < 1e-8f`. See CHANGELOG.


**File:** `plugins/O-AnalogSaturation/Source/PluginProcessor.cpp:321`, `:392`

**Issue:** Both RMS routines divide by the sample count with no guard:
```cpp
const float rms = std::sqrt(rmsSum / static_cast<float>(numSamples));   // line 321 and 392
```
Some hosts (and JUCE's own graph in edge cases) call `processBlock` with `numSamples == 0`. Then `rmsSum` is `0.0f`, `0.0f / 0.0f` → NaN, and `inputRMSEnvelope[channel]` / `outputRMSEnvelope[channel]` become NaN. The denormal-flush guards use `< 1e-8f` (lines 325, 396), and `NaN < 1e-8f` is false, so the NaN is never cleared — it persists across every subsequent block (`autoGainCoeff * NaN + ... = NaN`). With auto-gain enabled, `compensationGain = inputRMS / outputRMS` is NaN, `juce::jlimit(0.1f, 10.0f, NaN)` returns NaN (all comparisons false), and the output buffer is multiplied by NaN → corrupted/silent output that does not recover until the plugin is re-prepared.

**Fix:** Guard the block length before dividing:
```cpp
if (numSamples <= 0)
    return;                       // early-out at top of captureInputRMS / applyAutoGain
// ...or...
const float rms = numSamples > 0
    ? std::sqrt(rmsSum / static_cast<float>(numSamples)) : 0.0f;
```
Also harden the flush guards to catch non-finite state, e.g. `if (! std::isfinite(env) || env < 1e-8f) env = 0.0f;`.

## Warnings

### WR-01: setLatencySamples() invoked from the audio thread on Quality change

**File:** `plugins/O-AnalogSaturation/Source/PluginProcessor.cpp:221-237`

**Issue:** When `quality != currentQuality`, `processBlock` calls `setLatencySamples(latency)` (line 231). `setLatencySamples` triggers `updateHostDisplay` / `audioProcessorChanged`, which in several hosts takes locks, reallocates graph buffers, or rebuilds the processing graph — none of which is real-time safe from inside the audio callback. This can cause dropouts or glitches at the exact moment the user switches quality, and repeated toggling can stall the audio thread.

**Fix:** Detect the quality change on the audio thread but defer the latency report to the message thread (e.g. set an `std::atomic<int> pendingLatency` and flush it from a `Timer`/`AsyncUpdater`, or from `prepareToPlay`). At minimum, only call `setLatencySamples` when the value actually differs from the last reported latency to avoid redundant host notifications.

### WR-02: Dry signal is colored and delayed by the oversampler in MID/HIGH

**File:** `plugins/O-AnalogSaturation/Source/PluginProcessor.cpp:258-262`, dry/wet mix at `:446`, `:486`, `:539`, `:659`

**Issue:** In MID/HIGH quality, the entire buffer is upsampled, the per-sample routines compute `(dryMix * input) + (wetMix * wetSignal)` at the oversampled rate, then the mixed result is downsampled. The dry component therefore passes through both the up- and down-sampling FIR anti-imaging/anti-aliasing filters and picks up the oversampler's latency and HF phase/amplitude coloration. At low intensity the "dry" path is audibly different between LOW and MID/HIGH, and unity intensity is not bit-transparent. Combined with CR-01 this compounds the tonal inconsistency across Quality settings.

**Fix:** Keep a clean dry copy at base rate and mix it in after `processSamplesDown` (compensating for the oversampler latency so dry and wet stay phase-aligned), so only the nonlinearity and its filters are oversampled.

### WR-03: Auto-gain compensation gain is block-constant — steps at block boundaries

**File:** `plugins/O-AnalogSaturation/Source/PluginProcessor.cpp:400-409`

**Issue:** `compensationGain` is computed once per block and applied as a flat multiplier across the whole buffer (lines 405-408). When the gain changes between blocks it steps discontinuously, producing a zipper/click on transient material. (Today the envelope is so slow from CR-02 that steps are tiny, but fixing CR-02 to the intended 100 ms will expose audible stepping.)

**Fix:** Smooth the applied gain per sample, e.g. `juce::SmoothedValue<float>` ramped toward the target `compensationGain` over the block, rather than a hard per-block multiply.

### WR-04: No isBusesLayoutSupported override — host may instantiate unsupported layouts

**File:** `plugins/O-AnalogSaturation/Source/PluginProcessor.cpp:53-59` (constructor); no override present

**Issue:** The processor declares stereo-in/stereo-out in the constructor but does not override `isBusesLayoutSupported`. The `AudioProcessor` default accepts arbitrary layouts, so a host can negotiate mono or mismatched in/out channel counts. `prepareToPlay` re-derives sizes from `getTotalNumOutputChannels()` so it won't crash, but there is no explicit contract, and any future assumption of 2 channels would break silently.

**Fix:** Add an explicit guard:
```cpp
bool isBusesLayoutSupported(const BusesLayout& layouts) const override {
    const auto& out = layouts.getMainOutputChannelSet();
    if (out != juce::AudioChannelSet::mono() && out != juce::AudioChannelSet::stereo())
        return false;
    return layouts.getMainInputChannelSet() == out;
}
```

### WR-05: getTailLengthSeconds() returns 0 despite IIR ringing and hysteresis memory

**File:** `plugins/O-AnalogSaturation/Source/PluginProcessor.h:34`

**Issue:** The peak/shelf IIR filters (60/80/3k/8k/12k Hz) ring and the magnetic model retains state, so the output does not settle instantly after input stops. Declaring a zero tail can cause hosts to truncate the decay during offline bounce/freeze, clipping the natural die-out.

**Fix:** Return a small non-zero tail (e.g. `return 0.05;` for ~50 ms) reflecting the longest filter decay.

## Info

### IN-01: UI version label out of sync with plugin version

**File:** `plugins/O-AnalogSaturation/Source/ui/public/index.html:530`

**Issue:** The footer reads `v1.1.2` but the shipped plugin (CHANGELOG) is `v1.1.3`. Users see a stale version.

**Fix:** Update to `v1.1.3` and consider sourcing the string from a single build define to prevent future drift.

### IN-02: Inconsistent small-argument thresholds in the Langevin path

**File:** `plugins/O-AnalogSaturation/Source/PluginProcessor.cpp:551` vs `:603`

**Issue:** `langevinFunction` switches to its Taylor series at `|x| < 1e-6f`, but the derivative branch in `processMagneticSample` switches at `|arg| < 1e-4f`. Both are numerically safe, but the mismatched thresholds mean L and L' use different approximations in the 1e-6…1e-4 window, a small inconsistency in the hysteresis integration.

**Fix:** Use a single shared threshold constant for both the function and its derivative.

### IN-03: Leftover development comments and unexplained magic numbers

**File:** `plugins/O-AnalogSaturation/Source/PluginProcessor.cpp:428`, `:466`, `:504`, `:578`

**Issue:** Drive scalars carry stale dev annotations like `// 1.5x stronger (was 4.0)` and hard-coded tuning constants (6.0, 7.5, 4.5, 3.0, hardness 0.7, normalization 1.2 at line 533) appear inline without named constants, unlike the `MAGNETIC_*` set in the header.

**Fix:** Promote the per-model drive/hardness/normalization constants to named `static constexpr` members (as done for the magnetic model) and drop the "(was X)" comments.

---

_Reviewed: 2026-07-01T04:07:44Z_
_Reviewer: Claude (gsd-code-reviewer)_
_Depth: deep_
