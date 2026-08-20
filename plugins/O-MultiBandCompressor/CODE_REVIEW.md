---
phase: O-MultiBandCompressor-v1.6.0
reviewed: 2026-08-19T00:00:00Z
depth: deep
files_reviewed: 11
files_reviewed_list:
  - plugins/O-MultiBandCompressor/Source/PluginProcessor.cpp
  - plugins/O-MultiBandCompressor/Source/PluginProcessor.h
  - plugins/O-MultiBandCompressor/Source/PluginEditor.cpp
  - plugins/O-MultiBandCompressor/Source/PluginEditor.h
  - plugins/O-MultiBandCompressor/Source/DSP/MultiBandProcessor.h
  - plugins/O-MultiBandCompressor/Source/DSP/CrossoverNetwork.h
  - plugins/O-MultiBandCompressor/Source/DSP/Compressor.h
  - plugins/O-MultiBandCompressor/Source/DSP/EnvelopeDetector.h
  - plugins/O-MultiBandCompressor/Source/DSP/GainComputer.h
  - plugins/O-MultiBandCompressor/Source/ui/public/js/app.js
  - plugins/O-MultiBandCompressor/CMakeLists.txt
findings:
  critical: 1
  warning: 5
  info: 9
  total: 15
status: issues_found
---

# O-MultiBandCompressor v1.6.0: Code Review Report

**Reviewed:** 2026-08-19
**Depth:** deep
**Files Reviewed:** 11
**Status:** issues_found

## Summary

Full review of the shipped v1.6.0. The codebase's recurring failure modes are almost all
already handled, and handled well:

- **FileChooser `launchAsync` SafePointer pattern:** applied correctly to both dialogs,
  including the bare-`return` (no `complete()`) on the teardown path.
- **Native-function bridge parity:** 12 registered in C++, 12 consumed in JS (10 preset +
  2 tooltip). No gaps.
- **RT-safety:** crossover and sidechain coefficient updates use `ArrayCoefficients` with
  seeded storage (v1.6.0 fix); parameter pointers cached in `prepareToPlay`; M/S scratch
  preallocated; spectrum published through a correct lock-free triple buffer.
- **Preset system:** factory presets authored in engineering units and converted through
  each parameter's own `NormalisableRange` (skew honoured); Solo/SC-Listen excluded from
  presets; harness verifies load-order independence.
- **TDZ discipline in app.js:** deferred initializers at the foot of the file, documented.

The one critical finding is that M/S "Both" mode does not do what it says (or what
NOTES.md documents): the stereo-linked detector averaging mid and side reduces to
listening to the **left channel only**, and one shared gain is applied to both channels —
it is not independent mid/side compression. The warnings cover locale-fragile JS
injection (dead meters on comma-decimal hosts), phase mismatch on the passthrough channel
in Mid/Side modes, comb filtering on the dry path of the Mix control, unsmoothed makeup
gain, and a sticky-NaN detector state.

## Critical Issues

### CR-01: M/S "Both" mode is not independent — its detector hears only the left channel

**File:** `plugins/O-MultiBandCompressor/Source/PluginProcessor.cpp:867-885`, `plugins/O-MultiBandCompressor/Source/DSP/Compressor.h:160-166`
**Issue:** In mode 3 the buffer holds [mid, side] and is sent through the same
stereo-linked path as L/R. `Compressor::processStereo` averages the channels for
detection: `(M+S)/2 = ((L+R)+(L−R))/(2·√2) = L/√2`. The detector is therefore the left
channel scaled by 0.707 — a right-only signal produces **zero gain reduction** in "Both"
mode. The single computed gain is then applied identically to mid and side, which is
mathematically the same as applying it to L and R — so the mode is neither independent
M/S compression (NOTES.md promises "up to 8 compressors in Both mode", the UI tooltip
says "processes them independently") nor a sane linked mode. Modes 1 (Mid) and 2 (Side)
are correct — they run a genuinely mono buffer.
**Fix:** Give `Compressor` an unlinked dual-mono path: per-channel `EnvelopeDetector`,
per-channel smoothed GR state, and per-channel sidechain filters (`envelopeDetector[2]`,
`smoothedGainReductionDB[2]`, `scHPF[2]`/`scLPF[2]`, `averageGainReduction[2]`), selected
by a `bool linked` argument. Mode 3 passes `linked = false`; all other paths keep today's
behaviour bit-for-bit (linked path uses element [0] only). Report the deeper of the two
channels' GR to the band meter. This also makes the "8 compressors" claim true —
crossover filters are already per-channel.

## Warnings

### WR-01: `String::formatted("%f")` builds the meter/crossover JS — dead UI on comma-decimal locales

**File:** `plugins/O-MultiBandCompressor/Source/PluginEditor.cpp:524, 553, 580`
**Issue:** All three 30 Hz pushes (`sendGainReductionMeters`, `sendInputOutputMeters`,
`sendCrossoverPositions`) inject JS built with `juce::String::formatted("%f", …)`.
JUCE routes that through `vswprintf`, which honours `LC_NUMERIC` — on a host that sets a
comma-decimal locale (several Windows hosts do), the script becomes
`updateGainReductionMeters(0,500000, …)` → JS syntax error on every tick → GR meters,
I/O meters, crossover lines, and band-range headers all silently freeze. The plugin ships
on Windows, where this is a live path. `sendSpectrumData` already does it right with
`juce::String(value, 3)`, which is locale-independent.
**Fix:** Replace the three `formatted` calls with `juce::String(value, N)` concatenation,
matching `sendSpectrumData`.

### WR-02: Mid/Side modes recombine a phase-rotated channel with an unrotated one

**File:** `plugins/O-MultiBandCompressor/Source/PluginProcessor.cpp:817-866`
**Issue:** In Mid mode the mid channel passes the full crossover, whose 4-band sum is the
all-pass product AP(f1)·AP(f2)·AP(f3) (three 2nd-order all-passes, 0→−1080° of phase);
the side channel bypasses everything. Around each crossover the processed channel sits
≈180° out with the passthrough channel, so the decode `L=(M′+S)/√2, R=(M′−S)/√2`
mirrors the stereo image (L and R effectively swap) in those regions even at 1:1 ratios.
Same for Side mode with roles reversed.
**Fix:** Run the passthrough channel through a matching chain of three all-passes (same
`ArrayCoefficients::makeAllPass` design at f1/f2/f3, per-channel state, updated alongside
`CrossoverNetwork::updateCoefficients`). Six extra biquads; restores phase coherence
between mid and side.

### WR-03: Dry path of the Mix control is not phase-matched — combing in parallel mode

**File:** `plugins/O-MultiBandCompressor/Source/PluginProcessor.cpp:760, 907`
**Issue:** `pushDrySamples` captures the input before the crossover; the wet path carries
the AP(f1)·AP(f2)·AP(f3) phase rotation. At intermediate Mix settings the two cancel
wherever the chain's phase passes −180° (mod 360°) — multiple notches across the
spectrum, deepest near 50%. The shipped **Parallel Crush** preset (35% mix) bakes this
in; "parallel compression" is a headline feature.
**Fix:** Apply the same three all-passes to the dry signal before `pushDrySamples`
(shares the design work of WR-02 — one "phase-match chain" utility used for both). Dry
and wet then share the phase rotation and sum coherently at every Mix setting.

### WR-04: Per-band Makeup and Auto-Makeup transitions are unsmoothed — zipper/clicks

**File:** `plugins/O-MultiBandCompressor/Source/DSP/Compressor.h:204-209`
**Issue:** `makeupDB` is latched once per block and converted straight to linear gain per
sample; turning a Makeup knob steps the band's gain at block boundaries (audible zipper
at high gain). Toggling AUTO_MAKEUP is worse: `autoMakeupDB` jumps instantaneously from 0
to `−averageGainReduction·0.8` (potentially several dB) — a hard click. The GR itself is
fine (attack/release ballistics smooth it), as are Input/Output gain (20 ms ramps) and
Mix (DryWetMixer smooths internally); makeup is the one unsmoothed gain in the chain.
**Fix:** One-pole-smooth `totalMakeupDB` per sample inside the loop (a ~10–20 ms
coefficient, one extra multiply-add), or a `SmoothedValue` per band set from the block
value. Also covers the auto-makeup toggle for free.

### WR-05: A single NaN input permanently poisons the RMS detector state

**File:** `plugins/O-MultiBandCompressor/Source/DSP/EnvelopeDetector.h:70-78`
**Issue:** One non-finite sample from upstream (a blown-up plugin earlier in the chain)
enters `rmsSum` and the ring buffer; `rmsSum` is NaN forever after (subtracting the NaN
sample back out yields NaN). `GainComputer` then returns 0 GR for every sample
(`std::min(0.0f, NaN)` → 0), so the band silently stops compressing until the next
`prepareToPlay`. This is the codebase's documented envelope-follower sticky-NaN pattern.
**Fix:** Guard at the entry of `processSample`: `if (!std::isfinite(input)) input = 0.0f;`
plus a self-heal on the accumulator: `if (!std::isfinite(rmsSum)) { reset-window; }`.
Two branches per sample, trivially predicted.

## Info / Improvements / Simplifications

### IN-01: DSP crossover ordering clamp is invisible to the UI

`CrossoverNetwork::updateCoefficients` (CrossoverNetwork.h:113-116) enforces
`x2 ≥ x1+100`, `x3 ≥ x2+100`; the JS drag handler enforces the same gap — but host
automation and generic-editor edits bypass both, and the UI then displays the raw
parameter while the DSP runs the clamped value (e.g. XOVER2 automated to 200 Hz with
XOVER1 at 450 Hz shows 200, runs 550). Mirror the clamp in
`updateCrossoverPositions`/`updateBandRanges` in app.js so the display always shows the
effective frequencies.

### IN-02: FFT runs with no editor open

The per-sample FIFO fill plus a 2048-point FFT ~23×/s (PluginProcessor.cpp:928-986) runs
whether or not anyone is looking. Gate it on an atomic `editorAttached` flag set/cleared
by the editor's constructor/destructor — standard pattern, saves the CPU in the common
many-instances-closed-UI session.

### IN-03: Spectrum tooltip says "input spectrum"; the tap is post-output-gain

app.js:892 promises "Real-time input spectrum"; the FIFO is filled from the buffer after
compression and output gain (PluginProcessor.cpp:930). Either fix the tooltip ("output
spectrum") or move the tap pre-processing — for placing crossovers, an input tap is
arguably the more useful display.

### IN-04: Two `std::pow(10, x/20)` per sample per band

Compressor.h:208-209 — 8 transcendental pairs per sample across 4 bands. With WR-04's
smoothing in place the makeup half updates per sample anyway, but both conversions can
use the cheaper `std::exp(x * 0.11512925f)`; or convert once per block when auto-makeup
is off. Measurable on older machines, not urgent.

### IN-05: `sendCrossoverPositions` re-resolves parameters by string every tick

PluginEditor.cpp:568-570 does three map lookups at 30 Hz. Resolve the three
`getRawParameterValue` pointers once in the editor constructor.

### IN-06: Six main knob readouts still restate their ranges in JS

app.js:102-135 hand-roll min/max (and the attack/release skew) that `bindScaledSlider`
(v1.5.0) already gets from C++ via `propertiesChanged`. Deliberately deferred at the time;
migrating threshold/ratio/attack/release/knee/makeup to `bindScaledSlider` removes the
last duplicated range constants (`XOVER_RANGES` must stay — the drag code needs ranges
before propertiesChanged lands).

### IN-07: `rmsSum` float drift over long sessions

The sliding sum (EnvelopeDetector.h:70-71) accumulates float rounding error indefinitely.
Accumulate in `double`, or rebuild the sum from the buffer each time `rmsWritePosition`
wraps (once per 10 ms window, amortised negligible).

### IN-08: Stale RMS window survives a same-rate re-prepare

`prepare` resizes the ring but `std::vector::resize` to the same size keeps old contents
while `rmsSum` resets to 0 — the first window after re-prepare subtracts stale squares
(clamped to 0, so only a brief under-read). Add `std::fill(rmsBuffer.begin(),
rmsBuffer.end(), 0.0f)` in `prepare` (EnvelopeDetector.h:47).

### IN-09: Spectrum placeholder still says "(Phase 5.3)"

app.js:427-428 — visible in any host that doesn't call `processBlock` until transport
runs. Either drop the dev-phase caption or delete `initializeSpectrumPlaceholder`
entirely and let the live path draw the empty grid (it already draws the same grid).

## Verification Notes

- CR-01 algebra: M+S = ((L+R)+(L−R))/√2 = 2L/√2 → detector = (M+S)/2 = L/√2. Confirmed
  against `processStereo`'s channel-average loop with `numActiveChannels = 2` in mode 3.
- WR-01 confirmed against JUCE 8.0.14 source: `String::formattedRaw` uses
  `vswprintf`/`_vsnwprintf` (locale-sensitive); `juce::String(float, digits)` does not.
- WR-04: `juce::dsp::DryWetMixer` smooths its proportion internally
  (`SmoothedValue<…> dryVolume, wetVolume`) — Mix is explicitly NOT affected.
- Bridge parity, SafePointer usage, preset skew handling, prev/next-vs-dropdown ordering,
  and the v1.6.0 sidechain-enable fix were all checked and are correct as shipped.
