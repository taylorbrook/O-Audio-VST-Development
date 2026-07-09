---
phase: O-Contrabass-stage2-dsp
reviewed: 2026-07-08
depth: deep
files_reviewed: 19
files_reviewed_list:
  - plugins/O-Contrabass/Source/PluginProcessor.cpp
  - plugins/O-Contrabass/Source/PluginProcessor.h
  - plugins/O-Contrabass/Source/PluginEditor.cpp
  - plugins/O-Contrabass/Source/PluginEditor.h
  - plugins/O-Contrabass/Source/OContrabassMPESynthesiser.h
  - plugins/O-Contrabass/Source/BowedContrabassVoice.cpp
  - plugins/O-Contrabass/Source/BowedContrabassVoice.h
  - plugins/O-Contrabass/Source/DSP/WaveguideString.cpp
  - plugins/O-Contrabass/Source/DSP/WaveguideString.h
  - plugins/O-Contrabass/Source/DSP/DispersionFilter.h
  - plugins/O-Contrabass/Source/DSP/BodyResonator.cpp
  - plugins/O-Contrabass/Source/DSP/BodyResonator.h
  - plugins/O-Contrabass/Source/DSP/BowNoiseGenerator.h
  - plugins/O-Contrabass/Source/DSP/SchellengCalibration.h
  - plugins/O-Contrabass/Source/DSP/SubHarmonicBias.h
  - plugins/O-Contrabass/Source/DSP/MasterSaturator.h
  - plugins/O-Contrabass/Source/DSP/MasterLimiter.h
  - plugins/O-Contrabass/Source/DSP/StereoWidth.h
  - plugins/O-Contrabass/CMakeLists.txt
findings:
  critical: 3
  warning: 13
  info: 16
  total: 32
status: warnings_resolved
resolved:
  date: 2026-07-08
  set: [CR-01, CR-02, CR-03, WR-01, WR-02, WR-03, WR-04, WR-05, WR-06, WR-07, WR-08, WR-09, WR-10, WR-11, WR-12, WR-13]   # all Critical + Warning
  verification: 17/17 goldens reproduce byte-identical (10 baselines refreshed for corrected paths) · no acceptance-criteria regressions (note-sequence FAIL→PASS) · auval SUCCEEDED · pluginval-10 SUCCESS (Parameter thread safety + Fuzz parameters)
  design_decisions: {WR-10: "4 voices (EADG bank)", WR-11: "keep legacy MPE, defer zone layout to Stage 3 — doc only"}
  remaining: [IN-01..IN-16]   # Info/nitpick, opt-in
---

# O-Contrabass (Stage 2 DSP): Code Review Report

**Reviewed:** 2026-07-08
**Depth:** deep (parallel three-subsystem review: string-DSP core/RT-safety · body-resonator + master output chain · processor/params/MPE/build)
**Files Reviewed:** 19
**Status:** issues_found

> ⚠️ **Staging note:** O-Contrabass is marked `🚧 Stage 0` in PLUGINS.md but the source carries
> Phase 2.6b markers — it is a **Stage-2 DSP** build with a placeholder editor (WebView UI is
> Stage 3). This is a pre-release review: findings are pre-ship, not regressions in a shipped
> product. The editor exposes no parameter controls yet, so all findings concern the audio engine,
> parameter plumbing, MPE routing, and build config.

> ✅ **Resolution log (2026-07-08) — must-fix tier applied:** CR-01, CR-02, CR-03, WR-01.
> - **CR-01** `BodyResonator.cpp` — per-block `Coefficients::makeBandPass` (heap) → allocation-free
>   `ArrayCoefficients::makeBandPass` std::array overload; warm-up lands in `prepare()`.
> - **CR-02** `PluginProcessor.cpp:143` — `Range<int>(1,16)` → `Range<int>(1,17)` (channel 16 no longer dropped).
> - **CR-03 + WR-01** `PluginProcessor.{h,cpp}` — audio-thread `MessageManager::callAsync` (heap alloc +
>   `this`-capturing teardown UAF) → `AsyncUpdater` + `std::atomic<int> pendingTuningChoice`;
>   `parameterChanged` stores + `triggerAsyncUpdate()`, `handleAsyncUpdate()` applies `setMode` on the
>   message thread, destructor `cancelPendingUpdate()`s. Constructor seeds the initial mode synchronously.
> - **Verified:** 17/17 render goldens byte-identical (harness sets mode directly + identical `makeBandPass`
>   math + no golden on ch16 ⇒ golden-neutral) · `auval -v aumu OCbs OuDv` SUCCEEDED · `pluginval
>   --strictness-level 10` SUCCESS (Parameter thread safety + Fuzz parameters).
> - **Not committed** (pre-release Stage-2 build; commit/tag on request). **Remaining:** WR-02..WR-13, IN-01..IN-16.

> ✅ **Resolution log (2026-07-08) — Warning tier applied (WR-02..WR-13):** via
> `/improve-review O-Contrabass`. WR-02 cached the 4 master-chain param atomics; WR-03
> clamped the saturator to ±1.0 (monotonic); WR-04 reordered the chain to
> Sat→Width→Limiter so the ceiling holds; WR-05 resets `mu_s` unconditionally each block;
> WR-06 floors dispersion `I` at 8 (E1) to avoid the k-zero divergence; WR-07 bounds the
> bridge-LP group-delay compensation to the clamped pole; WR-08 uses the host rate for the
> slow-LFO delta; WR-09 resets macro/sub-harmonic/stiffness smoothers at the host rate;
> WR-10 adds 4 voices; WR-11 documented (legacy MPE kept, zone layout deferred to Stage 3);
> WR-12/WR-13 use `setCurrentAndTargetValue` in the two `reset()`s.
> **Verified:** 17/17 goldens reproduce byte-identical (7 unchanged, 10 baselines refreshed
> for the intentionally-corrected paths; each drift maps to a specific fix, all stability
> gates green, note-sequence acceptance FAIL→PASS) · `auval -v aumu OCbs OuDv` SUCCEEDED ·
> `pluginval --strictness-level 10` SUCCESS. **Remaining:** IN-01..IN-16 (opt-in).

## Summary

O-Contrabass is a physical-modeling **bowed contrabass** synth: a digital-waveguide string with
hyperbolic bow-friction excitation (2× oversampled), Schelleng-calibrated bow-force limiting,
dispersion allpass, an 8-mode body resonator, and a mono→stereo master chain (saturator → limiter →
allpass decorrelator/width → output gain), plus the shared Scala tuning engine + VST3 Note
Expression for microtonal playback.

**The numerical core is genuinely well-hardened** — the reviewer looking specifically for waveguide
blow-ups, denormals, and delay-line OOB found *no Critical DSP defects* (see "Handled correctly").
The real defects cluster in three places:

1. **Audio-thread RT-safety** — one per-block heap allocation in the body resonator and one
   heap-allocating `callAsync` on the parameter thread. Both are exactly the anti-patterns this
   codebase has ruled against before (`ArrayCoefficients`; RT-safe async dispatch).
2. **MPE / voice plumbing** — an off-by-one that silently drops MIDI channel 16, a single-voice
   synth that can't play the double-stop drones the parameter set is designed for, and legacy-only
   MPE with no zone layout.
3. **DSP correctness under non-default settings** — a friction-coefficient leak, two host-vs-internal
   sample-rate confusions (LFO + smoother ramp times), and two pitch/tone errors that only bite at
   extreme knob positions (deep bends, dark brightness).

**Recommended fix scope:** all **CR-01..03** + **WR-01..13**. The IN-* items are quality/robustness
cleanups (several are documented deferrals) and are opt-in.

---

## Handled correctly (commonly-broken things that are actually fine)

- **Waveguide stability / BIBO.** Feedback gain and injection are bounded (`rho ≤ 0.85`
  → `1-rho ≥ 0.15`, `injection = min(frictionVelocity, |v_delta|)`), and the per-rail
  `4·tanh(x/4)` saturator hard-bounds string state to ±4 → no blow-up even at extreme bow force.
- **NaN / denormal guards.** `ScopedNoDenormals` on both the voice render and master chain; `−1e-20`
  leak; `isfinite` resets on the bridge output and dispersion coefficient. High-Q modal ring-down is
  protected.
- **Delay-line bounds.** Rails clamped `[4, 8190]` against 8192 capacity with Lagrange-3rd's 4-tap
  margin respected; no OOB, no pointer-wrap bug. Division guards are present throughout
  (`jmax(1.0f, …)`, `jmax(denom, 1e-9f)`, ROSIN inverse floor).
- **Mono-bus safety.** Both the voice write (`if (numOutChans >= 2)`, `BowedContrabassVoice.cpp:807`)
  and `StereoWidth::processBlock` (`if (getNumChannels() < 2) return;`) guard the mono path — no
  out-of-bounds despite `isBusesLayoutSupported` accepting mono.
- **Build config is correctly pre-staged for Windows.** `NEEDS_WEBVIEW2 TRUE` **and**
  `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` are both present (the blank-WebView trap is
  pre-empted for Stage 3); `IS_SYNTH`/`NEEDS_MIDI_INPUT` correct; no BinaryData-namespace collision.
- **Latency reporting** uses `setLatencySamples()` in `prepareToPlay` (never overrides the
  non-virtual `getLatencySamples`) — idiomatic for JUCE 8.
- **State round-trip** is the canonical APVTS `copyState→XML→binary` / `fromXml→replaceState`, and
  restoring `TUNING_SYSTEM` re-fires the listener so tuning mode is reapplied on load.
- **CC11 (Expression) dispatch** reaches the custom `handleController` override (verified against
  JUCE `MPESynthesiser::handleMidiEvent`) — not a dead control.

---

## CRITICAL

### CR-01 — ✅ RESOLVED — Per-block heap allocation in the body resonator coefficient recompute (RT-safety)
**File:** `Source/DSP/BodyResonator.cpp:94-96` (called every block via `processBlock` → `recomputeCoefficients`, line 103)

```cpp
auto coeffs = juce::dsp::IIR::Coefficients<float>::makeBandPass (currentSampleRate, fc, qEff);
*modes[i].coefficients = *coeffs;
```

`Coefficients<float>::makeBandPass` (the `Ptr`-returning overload) does `return *new Coefficients(...)`
— a heap allocation. It runs **8× per block** (`kNumModes`) inside `recomputeCoefficients()`, which
`processBlock` calls unconditionally every block. Each voice owns its own `BodyResonator`, so with N
sounding voices this is **8·N `new`/`delete` cycles per audio block on the RT thread** — the exact
anti-pattern this codebase ruled against (memory: use `ArrayCoefficients::makeXXX` (stack), not
`Coefficients::makeXXX` (heap), for per-block coeff updates; shipped O-Formant v1.25.1 WR-08).

**Failure scenario:** A sustained chord (once WR-10 adds voices) at a 128-sample block =
dozens of heap allocations every ~2.7 ms → allocator contention / page faults → audio-thread priority
inversion → dropouts/xruns, non-deterministic under load.

**Prescribed fix:** Assign through the allocation-free `std::array` overload:
```cpp
*modes[i].coefficients =
    juce::dsp::IIR::ArrayCoefficients<float>::makeBandPass (currentSampleRate, fc, qEff);
```
`Coefficients::operator=(std::array)` reuses capacity after the first (prepare-time) call — zero
allocation in `processBlock`. Do a warm-up assignment in `prepare()` so the one-time
`ensureStorageAllocated` lands off the audio thread.

---

### CR-02 — ✅ RESOLVED — Legacy-mode channel range drops MIDI channel 16 (off-by-one)
**File:** `Source/PluginProcessor.cpp:143`

```cpp
synth.enableLegacyMode(/*pitchbendRange*/ 24, juce::Range<int>(1, 16));
```

`juce::Range<int>` is **end-exclusive** (`contains(x)` ⇔ `start ≤ x < end`). `MPEInstrument` gates
every note/expression message with `legacyMode.channelRange.contains(channel)`, and JUCE's own default
is `Range<int>(1, 17)` (channels 1–16). `Range<int>(1, 16)` covers only channels **1–15**; channel 16
is excluded. The inline comment claims "channels 1..16 — covers omni MIDI input," but it does not, and
the debug `jassert(allChannels.contains(channelRange))` passes (1–15 ⊂ 1–16), so it is silent even in
debug builds.

**Failure scenario:** Any note or per-note pitchbend/pressure/CC routed to MIDI channel 16 (omni DAW
output, or an MPE controller cycling round-robin through member channels 2–16) is silently discarded —
no voice starts, no audio. ~1/15 of MPE member-channel notes vanish.

**Prescribed fix:** `synth.enableLegacyMode(24, juce::Range<int>(1, 17));`

---

### CR-03 — ✅ RESOLVED — `MessageManager::callAsync` on the audio thread heap-allocates (RT-safety)
**File:** `Source/PluginProcessor.cpp:316` (in `parameterChanged`)

```cpp
juce::MessageManager::callAsync ([this, mode]() { tuningEngine.setMode (mode); });
```

The inline comment asserts `callAsync` is "RT-safe to invoke from the audio thread" — this conflates
*thread-safe* with *real-time-safe*. `callAsync` constructs a `std::function` from a capturing lambda
and internally `new`s a message object → **heap allocation**. APVTS `parameterChanged` fires
*synchronously on the setter's thread* (no thread hop), and when a host automates `TUNING_SYSTEM` the
VST3/AU wrapper applies the value on the audio thread — so `callAsync` runs on the audio thread. The
prior "fix" correctly moved the mutex-holding `setMode` off-thread but left an allocating dispatch
on-thread.

**Failure scenario:** pluginval's "Parameter thread safety" test (cited in the comment as pluginval-10)
drives every parameter from the audio thread; each *change* of `TUNING_SYSTEM` hits `callAsync` →
audio-thread allocation → priority inversion / possible glitch, and is precisely the violation class
that test is designed to fail on.

**Prescribed fix:** Give the processor an `AsyncUpdater` (or `juce::Timer`) + `std::atomic<int>
pendingTuningMode`. In `parameterChanged`, store the mode atomically and `triggerAsyncUpdate()`
(RT-safe: reuses a preallocated message). Apply `setMode` in `handleAsyncUpdate()`. **This also
resolves WR-01.**

---

## WARNING

### WR-01 — ✅ RESOLVED — Pending `callAsync` lambda captures `this` with no lifetime guard (teardown UAF)
**File:** `Source/PluginProcessor.cpp:316`

The lambda captures `this` and dereferences the `tuningEngine` member; there is no
`SafePointer`/`WeakReference`, and the destructor (`:152-155`) only removes the parameter listener — it
never drains or cancels the queued dispatch.

**Failure scenario:** User changes tuning system, then the host destroys the processor before the
queued message is delivered → the lambda calls `tuningEngine.setMode()` on freed memory → use-after-free
(matches the SafePointer/async-teardown pattern already in project memory).

**Prescribed fix:** The `AsyncUpdater` from CR-03 fixes this — call `cancelPendingUpdate()` in the
destructor.

### WR-02 — Uncached per-block APVTS lookups on the audio thread
**File:** `Source/PluginProcessor.cpp:236, 240, 244, 249`

Each block calls `parameters.getRawParameterValue("MASTER_SAT_AMOUNT"/"LIMITER_CEILING_DB"/"WIDTH"/
"OUTPUT_GAIN")`, which resolves to an O(log n) `std::map<StringRef,…>` walk 4× per block. It does not
allocate or lock (not a hard RT violation), but it is the anti-pattern the project's own guidance flags.

**Prescribed fix:** Cache the four `std::atomic<float>*` as members, resolved once in the constructor /
`prepareToPlay`, and `->load()` them in `processBlock`.

### WR-03 — Saturator transfer function folds back for |in| > 1.0
**File:** `Source/DSP/MasterSaturator.h:72` and `:92`

```cpp
const float xClamp = juce::jlimit (-1.5f, 1.5f, in);
const float wet    = xClamp - xClamp * xClamp * xClamp / 3.0f;
```

`f(x)=x−x³/3` has `f'(x)=1−x²` ≤ 0 for `|x|>1`; the correct clamp for this cubic is ±1.0 (output
plateaus at ±2/3). Clamping at ±1.5 admits the fold-back region: `f(1.0)=0.667`, `f(1.5)=0.375` — so as
input rises 1.0→1.5 the wet output *falls*.

**Failure scenario:** The saturator is first in the master chain and receives the raw summed voice
output, which exceeds 1.0 with polyphony. A crescendo from |x|=1.0→1.5 produces a *quieter*,
fold-back-distorted output at the loudest input — audible level dip + inharmonic fold-back on peaks
(full effect at `amount=1`).

**Prescribed fix:** Clamp to ±1.0 (`juce::jlimit(-1.0f, 1.0f, in)`), or use a transfer function that
stays monotonic past 1.0 (`tanh` / explicit soft-clip plateau).

### WR-04 — Width > 1 is unbounded and defeats the limiter ceiling
**File:** `Source/DSP/StereoWidth.h:105` (amplifying stage); chain order `Source/PluginProcessor.cpp:236-245`

`StereoWidth` runs **after** `MasterLimiter` (Sat→Lim→**Width**→Gain). The side path
`side = (left-right)*0.5*w` (`w` up to 2.0) has no bound, so `w>1` pushes peaks back above the ceiling
the limiter just enforced.

**Failure scenario:** Post-limiter `L=+ceiling, R=−ceiling`, `w=2` → `mid=0, side=ceiling` →
`outL=+2·ceiling` ≈ +5.7 dBFS at the −0.3 dBFS default. The ceiling guarantee is void and output
hard-clips downstream. The decorrelator intentionally makes L≠R, so real signals hit this whenever the
width knob is raised.

**Prescribed fix:** Reorder so `StereoWidth` precedes the limiter, or re-apply the ceiling / clamp after
the width stage.

### WR-05 — `mu_s` (static friction) leaks its biased value after SUB_HARMONICS returns to 0
**File:** `Source/BowedContrabassVoice.cpp:453` (inside the `if (subAmount != 0.0f …)` block); contrast the *unconditional* `setRosin` at `:413`

`sub_harmonics::applyBias` widens `mu_s` (up to 1.0 at subAmount=1) and the voice pushes it via
`frictionModel.setStaticFrictionCoefficient(mu_s_pre)`. When `SUB_HARMONICS` returns to 0 the entire
biasing block is skipped and **`mu_s` is never restored to the bass default 0.85** — it stays elevated
(harsher/stickier tone) for every subsequent note until `prepareToPlay` re-runs. `v_0` recovers because
`setRosin(rawRosin)` runs unconditionally at `:413`; there is no equivalent reset for `mu_s`. This also
breaks the stated HR-9 "bit-exact when sub-harmonics off" contract and persists across note-offs.

**Prescribed fix:** Reset `mu_s` to the bass default unconditionally each block (mirror `:413`), e.g.
`frictionModel.setStaticFrictionCoefficient(0.85f)` before the gate, letting the bias branch overwrite
it when active.

### WR-06 — Dispersion coefficient sign-flips on deeply-bent low notes
**File:** `Source/DSP/DispersionFilter.h:79-92` (`computeAllpassCoefficient`), invoked at `Source/BowedContrabassVoice.cpp:544-548`

`k = k1 + k2·I + k3·I²` has a real zero near piano-key index `I ≈ 2.33` (`f0 ≈ 30 Hz`), which is
*inside* the allowed `[20, 5000] Hz` range. `STRING_STIFFNESS` defaults to 0.30 so dispersion is active
by default. Bending/detuning the E string (41.2 Hz) toward ~30 Hz drives `f0` through the zero: `−C/k`
diverges and `jlimit` snaps the coefficient between `+0.99`/`−0.99` across block boundaries. The coeff
is not smoothed, so a glide produces an audible click + severe transient inharmonicity (`jlimit` +
`isfinite` prevent NaN/crash, so it's an artifact, not a blow-up).

**Prescribed fix:** Clamp the `f0` fed to the dispersion coefficient to the formula's validity envelope
(≳ E1 / `I ≥ 8`), and/or floor `|k|` away from zero, and/or smooth the coefficient per block.

### WR-07 — Pitch drifts sharp at low BRIGHTNESS (group-delay over-compensation)
**File:** `Source/DSP/WaveguideString.cpp:78-79` (`updateDelayLengths`) and `:108-111` (`setDelaySamples`)

`filterGroupDelay = sampleRate / (2π·brightnessHz)` grows unbounded as brightness drops, but the actual
bridge-LP pole is clamped to ≤ 0.95, so the real one-pole group delay saturates at ≈19 samples once
`brightnessHz < ~720 Hz`. Below that the code subtracts *more* delay than the filter adds → loop too
short → pitch goes sharp (≈1.3 semitones at `BRIGHTNESS=80 Hz` on E1). Pitch should not depend on the
brightness knob (default 4500 Hz is fine; only dark settings mistune).

**Prescribed fix:** Derive `filterGroupDelay` from the *clamped* pole (`bridgeP/(1-bridgeP)`), or clamp
`filterGroupDelay` to the same bound the pole clamp enforces.

### WR-08 — Slow-bow LFO runs at exactly half the set rate (host/internal rate confusion)
**File:** `Source/BowedContrabassVoice.cpp:468-469`

`slowLfoPhase` is advanced per block by `twoPi · rawSlowLfoRate · (numSamples / sr_internal)`.
`numSamples` is host-rate, so the block duration is `numSamples / hostSampleRate`; dividing by
`sr_internal` (= 2×host) halves the increment. `SLOW_LFO_RATE=0.3 Hz` actually swells at 0.15 Hz, and
the intended pressure-lag timing is skewed. (Per-sample vibrato at `:645` is correct — it advances in
the 2× loop with `1/sr_internal`.)

**Prescribed fix:** Use the host rate for the per-block delta (`numUp / sr_internal`, or store a
`sr_host` member).

### WR-09 — Per-block smoothers ramp 2× too slowly (reset at `sr_internal`, advanced by host `numSamples`)
**File:** `Source/BowedContrabassVoice.cpp:270` (`macroSmoothed`), `:284` (`subHarmonicsSmoothed`); `Source/DSP/WaveguideString.cpp:44` (`stiffnessSmoothed`)

`reset(sr_internal, 0.020)` sets `stepsToTarget = 88200·0.020 = 1764`, but these smoothers are advanced
by `numSamples` (host) steps per block, so they complete in 1764 *host* samples = 40 ms, not 20 ms
(sub-harmonics 30→60 ms, stiffness 20→40 ms) — always exactly 2×, independent of host rate. Direction is
benign (more smoothing) but the ramp times contradict the tuning the code comments assume. (`detuneSmoothed`
is correct — advanced once per 2× sample.)

**Prescribed fix:** Reset these three with the *host* sample rate (they are consumed per block at
host-rate step counts), or advance them by `numSamples * 2`.

### WR-10 — Synth has a single voice → effectively monophonic, contradicts the multi-string/drone design
**File:** `Source/PluginProcessor.cpp:139`

Only one `BowedContrabassVoice` is ever added, yet the parameter set is built for polyphony:
`ACTIVE_STRINGS` (1–4), four per-string detune params (`DETUNE_E/A/D/G`), and drone features
(`INFINITE_SUSTAIN`, `SUB_HARMONICS`).

**Failure scenario:** Play two open strings for a double-stop drone (the intended core use case) — the
second note-on steals the single voice; only one note ever sounds. The header comment still reads
"single E1 voice … Multi-voice is Phase 2.2," a lagging staging artifact.

**Prescribed fix:** Add N voices sized to the intended polyphony before ship:
`for (int i=0;i<numVoices;++i) synth.addVoice(new BowedContrabassVoice(&parameters,&tuningEngine));`

### WR-11 — MPE synth never establishes a zone layout (legacy mode only)
**File:** `Source/PluginProcessor.cpp:143`

`setZoneLayout` is never called; the instrument is permanently in legacy mode. Legacy mode routes
per-channel pitchbend/pressure/CC74 to notes on that channel (so one-note-per-channel MPE mostly works)
but **ignores the master zone and MPE Configuration Messages**.

**Failure scenario:** A ROLI Seaboard in lower-zone mode sends global pitchbend/CC on the master channel
for a whole-instrument glissando; notes on member channels don't respond. RPN-based zone
reconfiguration (MCM) is ignored. For a plugin billed as MPE, the master-zone loss should be a conscious,
documented decision.

**Prescribed fix (if true MPE desired):** Call `synth.setZoneLayout()` with a default lower zone; keep
legacy mode as an explicit non-MPE fallback.

### WR-12 — `StereoWidth::reset()` calls the wrong `SmoothedValue` overload
**File:** `Source/DSP/StereoWidth.h:61`

```cpp
widthSmoothed.reset (1.0f);   // binds to reset(int numSteps) → 1
```

There is no `SmoothedValue::reset(FloatType)`. `1.0f` converts to `int 1`, binding `reset(int numSteps)`
— this does *not* reset width to the default 1.0 (the apparent intent); it sets `stepsToTarget=1`
(destroying the 20 ms ramp) and jumps to the current target. Currently masked because `reset()` is only
reached via `releaseResources()` (always followed by `prepareToPlay`), but it is silently wrong today and
becomes a live click bug if `reset()` is ever wired to `AudioProcessor::reset()` (transport locate).

**Prescribed fix:** `widthSmoothed.setCurrentAndTargetValue (1.0f);` (matches how `MasterLimiter::reset()`
correctly does it).

### WR-13 — `MasterSaturator::reset()` clobbers the 30 ms ramp
**File:** `Source/DSP/MasterSaturator.h:53`

```cpp
amountSmoothed.reset (0);   // stepsToTarget = 0 → jump to current target
```

Same root cause and same lifecycle-masking as WR-12: destroys the ramp configured in `prepare()`; latent
zipper on `amount` automation if ever invoked standalone.

**Prescribed fix:** `amountSmoothed.setCurrentAndTargetValue (0.0f);` (or re-run `reset(sr, 0.030)`).

---

## INFO

- **IN-01 — `outputGainSmoothed.reset(1.0f)` wrong overload.** `PluginProcessor.cpp:213`: `1.0f` binds
  `reset(int numSteps)`, not the intended value-set. Harmless (`prepareToPlay` reseeds), but use
  `setCurrentAndTargetValue(1.0f)` — same class as WR-12/13.
- **IN-02 — Redundant double buffer clear.** `PluginProcessor.cpp:221-227`: the loop clearing
  output-beyond-input channels is dead for a 0-input synth; the following `buffer.clear()` already
  covers it. Drop the loop.
- **IN-03 — `__attribute__((weak))` is not MSVC-portable.** `PluginProcessor.cpp:28`: Clang/GCC-only; MSVC
  ignores it (the weak-override harness seam won't apply). Benign for production, but guard with
  `#if !defined(_MSC_VER)` before enabling Windows CI.
- **IN-04 — Mono bus advertised; width/decorrelation silently disabled on mono.**
  `PluginProcessor.cpp:158-162` accepts a mono main output. Both the voice write (`:807`) and
  `StereoWidth` (`:82`) guard mono safely (no OOB), but the stereo image collapses. Acceptable, or
  restrict `isBusesLayoutSupported` to stereo-only for clarity.
- **IN-05 — Zero-latency limiter overshoots the ceiling on transients.** `MasterLimiter.h:95-124`:
  feedforward, no look-ahead, 3 ms attack → peaks pass above `threshold` for up to ~attack time. The
  `-0.3 dBFS` default implies a true-peak intent this topology can't guarantee. Documented tradeoff.
- **IN-06 — Saturator cubic has no oversampling → aliasing above fs/3.** `MasterSaturator.h`: 3rd-order
  harmonics alias. Low impact for sub-500 Hz content; bright bow-noise content aliases.
- **IN-07 — Allpass decorrelation combs when summed to mono.** `StereoWidth.h:99-105`:
  `L+R = mono + allpass(mono)` colors bow noise near the 800 Hz allpass centre on mono systems. Inherent
  to the decorrelation approach.
- **IN-08 — Per-voice hard clip during crossfade.** `BowedContrabassVoice.cpp:803-804`: `jlimit(-1,1)`
  after `*0.35`; during an equal-power crossfade two near-saturated strings can sum to ≈1.98 pre-clip →
  distortion at extreme bow force. Safety clamp; consider more headroom or a soft clip.
- **IN-09 — Long-term intra-loop DC in infinite-sustain drone.** `WaveguideString.h:26-32`: with
  `INFINITE_SUSTAIN ≥ 0.95`, `denormalLeak=0` and no in-loop DC blocker, asymmetric friction can slowly
  bias the tanh saturator over minutes. Downstream 35 Hz HP protects the DAC; documented deferral.
- **IN-10 — Duplicated friction constants/formula in the sub-harmonic-bias path.**
  `BowedContrabassVoice.cpp:430-432`: `v_0_pre = 0.1·exp(-4.6·rawRosin)` re-implements
  `HyperbolicFriction::setRosin`, and `mu_s_pre=0.85` / `mu_d_const=0.25` hard-code the bass defaults set
  at `:238-239`. Centralize so the module and this path can't desync.
- **IN-11 — Dead members.** `slowLfoSpeedSmoothed`/`slowLfoPressureSmoothed` (`.h:171-172`, reset
  `.cpp:272-275`) and `currentMaxBlockSize` (`.h:132`) are never read. Remove.
- **IN-12 — Stale doc comments.** `WaveguideString.h:24` documents the in-loop saturator as
  `x/sqrt(1+x²)` but `.cpp:207-209` implements `4·tanh(x/4)`; `MasterLimiter.h:128-129` references a
  phantom `envR` member. Update both.
- **IN-13 — Unguarded APVTS deref in `noteStarted`.** `BowedContrabassVoice.cpp:76`:
  `parameters->getRawParameterValue("REFERENCE_PITCH")->load()` lacks the `parameters == nullptr` guard
  that `updateParametersFromAPVTS` (`:818`) uses. Documented "defensive only," but inconsistent.
- **IN-14 — Minor cleanups.** `BodyResonator.cpp:89-92` recomputes `gainLinear[i]` every block though it
  depends only on `currentSize` (gate behind a dirty flag alongside CR-01);
  `MasterSaturator.h:72/92` duplicates the waveshaper across `processSample`/`processBlock` (extract a
  `shape(float)` helper); `BowedContrabassVoice.cpp:746-748` advances smoothers `numSamples-1` steps
  (30 ms ramps run marginally long).
- **IN-15 — Test-only `setValueNotifyingHost` inside `prepareToPlay`.** `PluginProcessor.cpp:180-196`:
  the `OCBS_DISABLE_DECORRELATOR` block mutates user params on every prepare. Compiled out of production;
  ensure it is never shipped enabled.
- **IN-16 — Stale `currentFrequency` in dispersion compensation.** `WaveguideString.cpp:110`:
  `currentFrequency` is only updated in `trigger()`, so the dispersion group-delay term lags actual pitch
  during a bend (≤ a few samples out of ~2000; negligible but technically incorrect).

---

## Fix-scope recommendation

| Tier | Findings | Rationale |
|------|----------|-----------|
| **Must-fix before ship** | ✅ CR-01, CR-02, CR-03, WR-01 (resolved 2026-07-08) | RT-safety violations (pluginval gates) + silent note loss + teardown UAF |
| **Should-fix (correctness/audible)** | WR-03..WR-09 | Audible under real settings: fold-back, ceiling bypass, friction leak, pitch/tone errors, rate confusions |
| **Should-fix (behavior/design)** | WR-10, WR-11 | Monophony blocks the drone use-case; MPE zone decision |
| **Latent (fix opportunistically)** | WR-12, WR-13 | Wrong `reset()` overload — currently masked, live bug if `reset()` is rewired |
| **Opt-in cleanup** | IN-01..IN-16 | Quality, portability, dead code, documented deferrals |

**Resolve with:** `/improve-review O-Contrabass` (targets CR-* + WR-* by default; IN-* opt-in).
