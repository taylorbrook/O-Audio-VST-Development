# Stage 2: DSP - Research

**Date:** 2026-07-23
**Sources:** ARCHITECTURE.md (immutable contract), CONTEXT.md (D4/D5/D6), ROADMAP.md phases 2.1–2.3, in-suite reference code (verified on disk this session), auto-memory patterns

Scope per CONTEXT.md: harness scaffolding from O-simpleGrain, DelayBuffer/GrainScheduler adaptation from O-GrainScatter, probe implementations. The algorithm itself is fixed by ARCHITECTURE.md and is not re-researched here.

---

## 1. Render Harness Scaffolding (Phase 2.1 first deliverable)

**Template:** `plugins/O-simpleGrain/tests/render-harness/` (CMakeLists.txt + main.cpp, verified working, exit-code gated). Copy the structure, strip the synth-specific parts.

### CMake adaptation (differences from the O-simpleGrain harness)

| O-simpleGrain harness | O-ReverseDelay harness |
|---|---|
| Depends on `O-simpleGrain_Samples` + `O-simpleGrain_UIResources` BinaryData targets | **No BinaryData targets exist** — drop both `add_dependencies` entries and link items |
| Compiles `PluginEditor.cpp` (Stage-3 WebView editor) | **Only `Source/PluginProcessor.cpp`** — Stage 1 has no PluginEditor.cpp; `createEditor()` returns `GenericAudioProcessorEditor` from within PluginProcessor.cpp and links against stock JUCE modules |
| `JUCE_WEB_BROWSER=1` | `JUCE_WEB_BROWSER=0` (no WebView anywhere yet) |
| `JucePlugin_IsSynth=1`, `WantsMidiInput=1` | `JucePlugin_IsSynth=0`, `JucePlugin_WantsMidiInput=0`, `JucePlugin_IsMidiEffect=0` |

**Critical: target name ≠ folder name.** The plugin target is **`OuariconReverseDelay`** (CMakeLists.txt line 5), not `O-ReverseDelay`. All harness references must use it:

```cmake
option(OUARICON_BUILD_TESTS "Build O-ReverseDelay render-test harness" OFF)
if(OUARICON_BUILD_TESTS)
    add_subdirectory(tests/render-harness)
endif()
# harness CMakeLists:
add_dependencies(O-ReverseDelay-render-test OuariconReverseDelay)
target_include_directories(O-ReverseDelay-render-test PRIVATE
    $<TARGET_PROPERTY:OuariconReverseDelay,INCLUDE_DIRECTORIES>)
```
(Memory pattern `build_script_target_name_vs_folder` — 11/37 plugins have this mismatch; O-simpleGrain's harness didn't face it because its target == folder.)

**JucePlugin_* macros** (harness compiles the processor without juce_add_plugin's generated header):

```cmake
JucePlugin_Name="O-ReverseDelay"
JucePlugin_Manufacturer="Ouaricon Audio Development"
JucePlugin_ManufacturerCode=0x4f754476   # 'OuDv' (dev branding)
JucePlugin_PluginCode=0x4f527644         # 'ORvD'
JucePlugin_VersionString="1.0.0"
JucePlugin_VersionCode=0x10000           # 65536 — matches Stage-1 verified AU version
```

Do NOT hand-roll a second version constant elsewhere (O-simpleGrain WR-04 drift lesson) — if the plugin CMakeLists gains a version variable, derive from it.

The processor uses `#include <JuceHeader.h>`; the `$<TARGET_PROPERTY:...,INCLUDE_DIRECTORIES>` borrow makes the plugin's generated JuceHeader visible to the harness, same as O-simpleGrain line 31–33.

### Harness main.cpp skeleton (reusable pieces from O-simpleGrain main.cpp)

Directly liftable helpers (verified at `tests/render-harness/main.cpp`):
- `setParam(apvts, id, real)` — engineering-units param set via `convertTo0to1` (lines 63–69)
- `rms(x, off, len)`, `peakAbs(x)`, `allFinite(x)` (71–89)
- `continuityFraction(...)` — short-window RMS envelope, amplitude-independent (96–118)
- `autocorrPitchHz(...)` — not needed here (no pitch), skip
- `Spectrum` struct — Hann-windowed `juce::dsp::FFT` magnitude + `centroid(lo,hi)` (152+) — **reuse for the damping-generation probe**

Driving an *effect* (vs the synth harness): fill the input buffer with the test signal before each `processBlock` call, no MIDI needed (pass an empty `MidiBuffer`). Render loop shape:

```cpp
ReverseDelayProcessor proc;
proc.setPlayHead(&mockPlayHead);              // or leave null for COMPAT-02 pass
proc.prepareToPlay(fs, 512);
for (block...) { fillInput(buf); proc.processBlock(buf, emptyMidi); appendOutput(buf); }
```

### Mock AudioPlayHead (needed for FUNC-02, new vs O-simpleGrain)

`AudioProcessor::setPlayHead(AudioPlayHead*)` is public JUCE API; `getPosition()` returns `Optional<PositionInfo>` (verified JUCE 8.0.14, `juce_AudioPlayHead.h`, getBpm at line 313):

```cpp
struct MockPlayHead : juce::AudioPlayHead
{
    juce::Optional<double> bpm { 120.0 };
    bool playing = true;
    juce::Optional<PositionInfo> getPosition() const override
    {
        PositionInfo pi;
        pi.setBpm(bpm);                 // setBpm({}) simulates a host with no tempo
        pi.setIsPlaying(playing);
        return pi;
    }
};
```

Three sync test configurations: (a) bpm=120 → 500 ms spacing check; (b) `pi.setBpm({})` → fallback; (c) `proc.setPlayHead(nullptr)` (never call setPlayHead) → fallback. COMPAT-02 acceptance names the no-playhead case; test (b) is a cheap extra since hosts report absent BPM both ways.

---

## 2. Probe Implementations (the new work in this harness)

### 2.1 Single-grain reversed-ramp probe (FUNC-01 direction)

Isolate one grain: `mix=100`, `feedback=0`, `density=0` (overlap=1 → back-to-back grains, no overlap), `width=0`, free mode `delayTime=500`, `grainSize=200`.

Input: a **linear amplitude ramp on a fixed-frequency carrier** is harder to misread than a raw ramp (a raw 0→1 ramp has DC content the mix stage passes anyway). Simplest robust probe: input = raw linear ramp 0→1 over ~1 s, then silence. Take the wet output segment corresponding to one grain (locate via its RMS burst), and compute the **sign of the de-windowed slope**:

- Fit: for grain samples `w[n]`, the underlying source is `w[n]/hann(n/G)` (guard `hann > 0.1` to avoid edge blowup). Linear-regress the de-windowed values against `n`; assert slope **negative** (input ramp was rising; a reversed grain plays it falling).
- Cross-check (cheap, no de-windowing): `corr(w, reverse(sourceRegion)·hann) > corr(w, sourceRegion·hann)` by a factor ≥ 5. Catches an accidentally forward read even if slope-fit is noisy.

Off-by-one/inverted read law (ARCHITECTURE risk: "symptom is forward echoes or double-speed pitch") also shows up here: a `D+n` (not `D+2n`) offset bug produces a **frozen** (non-advancing) read → near-constant de-windowed values → slope ≈ 0; assert `|slope| > threshold` too, not just sign.

### 2.2 Impulse envelope-slope probe (FUNC-01 reverse bloom)

Input: single unit impulse at a known sample `T`, then silence. Settings: defaults but `mix=100`, `feedback=0`.

Per ARCHITECTURE §Algorithm Details, the impulse appears in every grain spanning it, with window amplitude increasing toward the reversed position. Probe:

- Collect wet frame-RMS envelope (reuse `continuityFraction`'s framing, ~5 ms frames) over the window `[T + D − G, T + D + G]` where the bloom lands.
- Locate envelope peak `P`. Assert **pre-peak energy ramps**: split `[bloomStart, P]` into halves; `RMS(second half) > 2 × RMS(first half)`.
- Negative reference (chunked-block behavior): a gated block would put near-flat energy across the region — also assert `env(firstFrame) < 0.25 × env(peakFrame)` (no hard leading edge).

### 2.3 Click detector (DSP-01, QUAL-01 sweeps)

Detect discontinuities without false-positiving on signal content: use a **first-difference threshold on a smooth input**. Input = 220 Hz sine at −12 dBFS (max legitimate sample-to-sample step for a sine: `2π·f/fs·A` ≈ 0.007 at 48 kHz — comfortably below click amplitudes).

```
maxStep = max |y[n] − y[n−1]| over the wet render
assert maxStep < kSineStepBound + 10^(−60/20)   // −60 dBFS margin per DSP-01
```

Run it (a) at defaults, (b) during a density 0→100→0 automation sweep, (c) in 2.3 during the all-parameter sweep (each param ramped across its full range over ~2 s of render, one at a time, others at defaults). Also assert `allFinite` on every render.

Note: at density 0 (overlap = 1) back-to-back Hann grains have zero-valued boundaries — sample-to-sample steps at grain joins are inherently tiny. The detector needs no grain-boundary carve-outs.

### 2.4 Density-flatness probe (DSP-01, tuning target ±1 dB)

Input: steady 220 Hz sine. For density ∈ {0, 25, 50, 75, 100}: render ≥ 2 s after 1 s settle, measure wet RMS. Assert `max/min ≤ 10^(1/20)` (±1 dB window across all steps, after tuning the compensation constant). This is the harness assertion that **freezes** the tuned overlap-compensation constant (CONTEXT.md D5: tune until green, then the assertion pins it).

### 2.5 Feedback-generation probes (FUNC-03)

- **feedback=0 → exactly one generation:** impulse input; assert wet energy outside `[T + D − G, T + D + G + smoothing margin]` is < −80 dBFS relative to the in-window energy (i.e. no echo at `T + 2D`).
- **Damping loss per generation:** impulse (broadband) input, `feedback=60`, `highCut=4000`, `lowCut=200`, `mix=100`. Echoes arrive near `T + kD`. FFT each echo window with the `Spectrum` helper; assert `centroid(gen k+1) < centroid(gen k)` for HF loss, and low-band energy fraction (`20–150 Hz`) falls generation-over-generation for LF loss. Two generations of comparison suffice.
- Alternating-direction regenerations (gen 2 forward) are **intended** (ARCHITECTURE decision) — probes must not assert on echo direction beyond generation 1.

### 2.6 Stability render (DSP-03)

60 s render, `feedback=100`, default damping (lowCut 100 / highCut 8000), input = 2 s of pink-noise-ish excitation then silence (excite the loop, then let it self-sustain). Assert: every output sample finite, `peakAbs < 1.0` (tanh bound guarantees the loop; the equal-power mix keeps output below ceiling), and wet RMS in the last 10 s is not growing (`RMS(50–55 s) ≥ RMS(55–60 s) × 0.99` need not hold exactly — assert *bounded*: last-10 s peak ≤ 1.0 and no NaN, per acceptance wording "below ceiling, zero NaN/Inf").

### 2.7 Sync-spacing probe (FUNC-02)

Impulse train input (one impulse, silence), mock playhead 120 BPM, Sync + 1/4. Measure first echo latency = position of wet envelope peak − T. Assert `|latency − 0.5 s·fs| ≤ blockSize` (±1 block per acceptance). Free-mode variant: same measurement at `delayTime ∈ {150, 500, 1200}` ms asserting continuous control.

### 2.8 Width probe (FUNC-04) + mono-sum identity check

- `width=0`: render noise burst; assert per-block `|RMS_L − RMS_R| / RMS < 0.01` (centered dual-mono) and L/R correlation ≈ 1.
- `width=100`: assert inter-channel correlation of the wet tail drops below ~0.9 and per-grain L/R RMS asymmetry exists (frame-level `|L−R|` energy well above the width-0 case).
- **Mono→stereo layout check (CONTEXT.md open item):** run a pass with a 1-in/2-out bus config; capture L=R=input; assert wet level matches the stereo-input run at width 0 within 0.5 dB (the 0.5·(L+R) mono-sum must degrade to identity — no ±6 dB surprise).

---

## 3. Component Adaptation Details

### 3.1 Capture ring buffer (from `O-GrainScatter/Source/dsp/DelayBuffer.h`)

Keep: `prepare(fs, 3.5f)` sizing, **alloc-free `clear()`** (the WR fix — no `setSize` outside prepare), `pushSample(L,R)` write.

Change:
- **Drop Lagrange interpolation.** v1.0 grain reads are integer-stepped (ARCHITECTURE: "keep integer reads for the hot path"). Replace `readSample(ch, float delaySamples)` with an absolute-index read.
- **Absolute-position bookkeeping instead of relative-delay reads.** GrainScatter's per-sample `delaySamples = positionOffset + elapsed − readPosition` dance exists to cancel write-head drift for *forward* streaming taps. The reverse read law is cleaner in absolute coordinates: maintain a monotonically increasing `int64 totalWritten` alongside the ring; a grain latches `readStartAbs = totalWritten − D` at spawn and reads `capture[(readStartAbs − n) mod size]`. No per-sample drift math, no wrap ambiguity, and the D+2n offset growth falls out for free (write head advances +1 while readAbs steps −1).

```cpp
float readAbs (int ch, juce::int64 absIndex) const noexcept
{
    const int idx = (int) (((absIndex % bufferSize) + bufferSize) % bufferSize);
    return buffer.getSample (ch, idx);
}
```

Mono-sum grain source (D4): `0.5f * (readAbs(0, i) + readAbs(1, i))` — one function, called per grain-sample.

### 3.2 Grain POD (from `O-simpleGrain/Source/dsp/Grain.h`, heavily simplified)

O-simpleGrain's double-precision readPos, rate, and per-grain AA one-pole all exist for *pitch-shifted* reads — none apply here (reverse speed is exactly 1.0, integer stepping). Slot:

```cpp
struct ReverseGrain
{
    bool         active   = false;
    juce::int64  readAbs  = 0;      // next capture index to read (steps −1 per sample)
    int          n        = 0;      // samples emitted
    int          G        = 0;      // latched length (samples)
    float        invG     = 0.0f;   // 1/G for window phase
    float        gain     = 0.0f;   // 1/sqrt(overlap), latched
    float        gL = 0.70710677f, gR = 0.70710677f;  // equal-power pan, latched
    int          age      = 0;      // for steal-oldest
};
```

Per-sample render: `s = 0.5·(capL+capR); e = hannLut.read(n·invG); wetL += s·e·gain·gL; wetR += s·e·gain·gR; --readAbs; ++n;` — LUT lerp + ~6 mul-adds, matching the ARCHITECTURE cost budget.

Pool: `std::array<ReverseGrain, 32>`, GrainPool's find-inactive round-robin + steal-oldest scan (GrainPool.h lines 124–150) is the proven shape — reuse the loop, drop everything spatial/freeze.

### 3.3 Scheduler (from `O-GrainScatter/Source/dsp/GrainScheduler.h` free-mode)

`processBlockFree` (lines 27–45) is the exact shape: per-sample countdown emitting `SpawnRequest{sampleOffset}`. Changes:
- Interval from ARCHITECTURE, not the exponential map: `overlap = 1 + (density/100)·7; intervalSamples = max(1, (int)(G/overlap))`.
- No probability gate, no Euclidean.
- Spawn cap: pool is 32, and max sustained overlap is 8 — cap spawns per block at 32 (`kMaxSpawnsPerBlock` pattern, WR-04). With G ≥ 50 ms and overlap ≤ 8, natural spawn rate ≤ 1 per 6.25 ms ≈ 2–3 per 512-block — the cap is pure safety.
- Latch at spawn time: resolve D (sync/free), G, pan (width RNG + alternating sign), gain (from current overlap) into the SpawnRequest or directly into the slot at the spawn offset.

Sync-mode note: O-GrainScatter's `processBlockSync` (subdivision-crossing triggers) is **not** the model here — O-ReverseDelay's sync only changes the *value of D*, not the spawn timing (ARCHITECTURE: "No conditional routing: syncMode only selects the source of D"). The scheduler always runs the free-countdown; sync is a per-block D computation: `beats·60000/bpm` clamped [50, 2000], `jmax(1.0, bpm)` divide hardening (IN-06, GrainScheduler.h line 107).

### 3.4 Hann LUT (from `O-simpleGrain/Source/dsp/WindowLuts.h`)

Trim to Hann-only: single `std::vector<float>` (2048), same build-once constructor, same clamp+lerp `read(phase)` (drop the shape index). The 5-shape class carries a UI-mirroring contract (IN-05) that must not travel with the copy.

### 3.5 Damping filters + feedback return

The ARCHITECTURE §Algorithm Details code block is the implementation (ArrayCoefficients in-place assignment, `lc != lastLowCut` guard gating only the recompute). Suite footguns already encoded there; the memory patterns to honor in code review:
- `pattern_arraycoefficients_rt_safe_iir` — `*state = arrayCoeffs` assignment, never `Coefficients::makeXXX` on audio thread, never memcpy 6 raw over 5 normalised.
- `pattern_conditional_coeff_update_leaks_enabled_flag` — no enabled flag exists in v1.0; keep it that way.
- `pattern_biquad_nan_guard_sticky_silence` — non-finite guard must `filter.reset()` both filters AND zero the feedback block, keeping last-known-good coefficients.
- highCut clamp `jlimit(500, 0.49·fs)` **per block after smoothing** (44.1 kHz: 20 kHz > 0.49·44100 = 21.6 kHz is fine, but 0.49 clamp still applied for any fs).

Filter classes: `juce::dsp::IIR::Filter<float>` × 4 (HP/LP × 2 ch), `prepare({fs, blockSize, 1})` each, `processSample` per sample or `process` on the feedback scratch block. Block-level processing of the feedback return buffer is simplest: `fbBuf = wet; fbBuf *= fbGain(smoothed per sample); hpL/hpR.process; lpL/lpR.process; tanh each sample; guard; then pushSample(input + fb)`.

### 3.6 Processing order & buffers (per ARCHITECTURE §Processing Order — REQUIRED)

Preallocated in `prepareToPlay`: capture ring (3.5 s × 2 ch), `wetScratch` (2 × maxBlock), `fbScratch` (2 × maxBlock). Per block: (1) resolve D; (2) smoothers + coeffs; (3) schedule spawns; (4) render grains → wetScratch; (5) fbScratch = damp(tanh path) of wetScratch; (6) `pushSample(in + fb)` loop; (7) equal-power mix wetScratch with untouched input → output. Wet never renders in-place over input.

Intra-block hazard check (ARCHITECTURE integration note): min read offset D ≥ 50 ms ≫ block, so block-level read-then-write ordering is safe — no per-sample interleave needed.

---

## 4. Pitfalls Checklist (for PLAN.md to carry into execute)

1. **Reverse read law off-by-one** → single-grain probe asserts slope sign AND magnitude (catches frozen-read `D+n` bug, not just forward-read).
2. **Harness silently breaks when Stage 3 adds WebView** (`pattern_render_harness_breaks_on_webview_editor`) — already noted in ROADMAP 3.1; nothing to do in Stage 2 except keep PluginProcessor.cpp editor-include-free (Stage-1 header comment already enforces).
3. **Target name mismatch** — every harness CMake reference uses `OuariconReverseDelay`.
4. **`ScopedNoDenormals`** in processBlock — mandatory, long feedback decay sweeps denormal range.
5. **Param reads once per block** via `getRawParameterValue()->load()`; choice params cast from float.
6. **Smoothed params:** feedback, mix, lowCut, highCut (~20 ms). NOT smoothed (latched per grain): delayTime/D, grainSize, density, width — smoothing them would be wrong (latching is the click-free mechanism).
7. **Density 0 edge:** `overlap = 1`, interval = G → gain = 1.0; amplitude ripple is intended character, not a bug — density-flatness probe measures RMS which stays flat even with ripple (RMS of back-to-back Hann = RMS of compensated overlap-add within tuning tolerance; if ±1 dB proves unreachable at the 0-end, the tuned compensation constant may need a shaped curve — tune in harness, then freeze).
8. **Feedback tap point:** post grain-gain, pre width/mix — width/mix must never enter the loop (automating them can't destabilize; also the FUNC-03 probes assume it).
9. **Grain spawn during the same block as a D change:** latch D per spawn from the block's resolved value — fine per contract; no sub-block D interpolation.
10. **pluginval-10 ×3 locally** before phase 2.3 sign-off (`pattern_ci_pluginval10_catches_latent_nan` — feedback NaN class has shipped in this suite twice).
11. **Tail length:** `getTailLengthSeconds()` currently returns Stage-1 default — Stage 2 should return a real tail (e.g. `2.0 + feedback-dependent` or a conservative constant like 10 s) so hosts don't truncate the reverse tail on bounce. Check acceptance: not required by any Stage-2 requirement, but offline-render behavior in DAWs depends on it; flag for plan.

---

## 5. Module Reuse Assessment

No `/module-add` candidates for Stage 2. DelayBuffer / WindowLuts / GrainPool / GrainScheduler are per-plugin headers (patterns to copy-and-simplify), not registered shared modules. OuariconPresetManager is a Stage-4 concern (BRIEF). The copies should live in `plugins/O-ReverseDelay/Source/dsp/` following suite convention.

## 6. JUCE API Verification (all previously verified against local 8.0.14 in ARCHITECTURE.md)

- `juce::dsp::IIR::ArrayCoefficients<float>::makeHighPass/makeLowPass` — `juce_IIRFilter.h` ✓
- `juce::AudioPlayHead::PositionInfo::getBpm()` → `Optional<double>` — `juce_AudioPlayHead.h:313` ✓
- `AudioProcessor::setPlayHead` — public, used by harness mock ✓ (O-simpleGrain harness precedent for direct-processor driving)
- `juce::SmoothedValue`, `juce::dsp::FFT` (harness Spectrum), `juce::ScopedNoDenormals` — standard ✓
- `juce::juce_dsp` already linked in plugin CMakeLists ✓ (line 34)

## 7. Recommended Plan Shape (input to /plugin-plan)

Phase 2.1 task order: harness CMake + main skeleton (renders silence through the Stage-1 passthrough, exits 0) → capture ring + Hann LUT + grain POD/pool → scheduler + reverse read law → mix stage → probes 2.1–2.5 (feedback=0 subset) green → commit. Phase 2.2: feedback path + probes 2.5–2.6 → commit. Phase 2.3: sync engine + width + probes 2.7–2.8 + all-param sweep + pluginval-10 ×3 → Standalone audition (D6) → commit.
