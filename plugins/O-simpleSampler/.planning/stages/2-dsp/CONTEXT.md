# Stage 2 (DSP) — CONTEXT

**Plugin:** O-simpleSampler
**Stage:** 2 of 4 — DSP
**Phase:** discuss → complete
**Date:** 2026-06-25
**Source:** Interactive discuss session (2 decisions resolved) + ARCHITECTURE.md / ROADMAP.md / parameter-spec.md / Stage 1 VERIFICATION.md

---

## Goal

Turn the silent 16-voice shell from Stage 1 into a playable, professional-sounding
educational sampler: a buffered source read through **Repitch** (fractional-read
varispeed) and **Stretch** (synchronous-granular SOLA) engines, isolated by a
start/end region with looping, shaped by a per-voice amp ADSR + VCA, coloured by a
Vintage macro and a resonant LP filter, with audio-thread viz taps and an offline
render-harness correctness gate.

Stage 2 is internally **3 phases**:
- **2.1 Core playable sampler** — Repitch + region (start/end) + amp ADSR + built-in `.wav` decode → **first audio**.
- **2.2 Full tone chain** — loop (fwd/ping-pong + equal-power crossfade) + reverse + **Stretch** (SOLA) + Vintage (S&H + bit-crush) + resonant LP filter.
- **2.3 Hardening** — anti-alias hardening + viz taps + voice-stealing + RT-safety + offline render-harness (Stage-2 correctness gate).

## Requirements covered (this stage)

FUNC-01, FUNC-02, FUNC-04, FUNC-05, FUNC-06, FUNC-08, DSP-01…DSP-07, PERF-01, QUAL-01.

---

## Decisions resolved in discuss

### D1 — Built-in source set: **piano.wav only, for now**
- Embed **only `piano.wav`** (copied from `plugins/O-simpleGrain/Source/samples/piano.wav`)
  to unblock Phase 2.1 "first audio". It is in-repo and proven to decode; pitched
  content is ideal for validating Repitch tuning across the keyboard.
- The full curated 4-slot built-in set (and per-sample default roots) is **deferred to
  Stage 4 polish** — do not block Stage 2 on sourcing new audio assets.
- **Impact on Stage 1 placeholders:** the `sourceSample` combo currently advertises
  4 placeholder names (piano/vocal/flute/vinyl). For Stage 2 the *decodable* built-in
  is piano only; the engine must select/decode piano and seed its default root. Other
  combo entries may remain as labels but need not decode until Stage 4 (decide in plan:
  either collapse the combo to a single "Piano" entry for now, or keep 4 labels with
  only piano wired — prefer the lowest-risk option that keeps APVTS/state stable).
- **Default root for piano.wav:** determine the sample's actual pitch during execute
  (inspect/probe) and seed `rootKey` accordingly; do not hard-code C3 (60) without
  confirming the file's pitch.

### D2 — Execute scope: **checkpoint after Phase 2.1**
- The execute phase implements **Phase 2.1 only** (Repitch + region + amp ADSR +
  piano decode → first audio), builds VST3+AU+Standalone, runs `auval` + pluginval,
  then **STOPS** for a DAW play-test before committing to 2.2/2.3.
- 2.2 (full tone chain) and 2.3 (hardening + render-harness) are subsequent execute
  passes (each its own discuss→…→verify mini-cycle or a continued Stage-2 execute,
  decided at the 2.1 checkpoint).
- Rationale: matches the phased roadmap and the user's design-conscious DAW test rhythm;
  "first audio" is the natural validation checkpoint.

---

## Phase 2.1 scope (immediate execute target)

**Goal:** A polyphonic, MIDI-playable sampler — piano source read through Repitch,
isolated by start/end, shaped by amp ADSR + VCA, tuned relative to Root Key.

**Components (from ARCHITECTURE / ROADMAP):**
- `SamplerSound`, `SamplerSynthesiser` (16-voice), `SamplerVoice : juce::SynthesiserVoice`
  (copy O-simpleFM/O-simpleGrain voice skeleton; **custom**, NOT `juce::SamplerVoice`).
- **Source buffer + region read head:** shared `juce::AudioBuffer<float>`; `[startSamp,endSamp)`
  region; `readPos += keyRatio` (Repitch); `keyRatio = 2^((note − rootKey + tune + fine/100)/12)`.
  Params: `start`, `end` (string IDs `"start"`/`"end"`; C++ identifiers `regionStart`/`regionEnd`),
  `rootKey`, `tune`, `fine`.
- **Anti-alias read:** 4-pt Lagrange (`Source/dsp/LagrangeInterpolation.h`, port from O-simpleGrain)
  + rate-tracking one-pole (`fc=0.5·fs/rate`) for `rate>1`. DSP-02.
- **Built-in embedding + decode:** second `juce_add_binary_data` target (`NAMESPACE BinaryData`,
  per the CMake TODO from Stage 1); `createReaderFor(MemoryInputStream)` → resample to engine
  rate **off-thread** → atomic publish. Per-sample default root seeds `rootKey`. FUNC-02.
- Amp `juce::ADSR` → VCA + voice lifetime; `velToAmp` velocity scaling. Processor reads APVTS
  once/block → `voice->setParams(...)`; `ScopedNoDenormals`; `SmoothedValue` on output.

**Test criteria (2.1):**
- [ ] Loads in DAW as an **instrument**, MIDI routes, plays 16-voice polyphonically (no crash).
- [ ] Root Key plays at original pitch; notes above/below transpose by varispeed (Repitch).
- [ ] Start/End change the played region; playback begins at Start, ends at End.
- [ ] `tune`/`fine` transpose independent of the keyboard.
- [ ] Built-in piano selects, decodes, plays; selecting it seeds its default root.
- [ ] **No aliasing/buzz at high notes** (render-harness aliasing probe deferred to 2.3, but no obvious artefacts).
- [ ] No clicks on note-on/off; no denormal CPU stalls on long releases.
- [ ] Build clean (VST3+AU+Standalone); `auval` SUCCEEDED; pluginval@5 SUCCESS.

---

## Carry-forward from Stage 1 (constraints)

- **21-param APVTS is frozen** — DSP must read existing param IDs; do not rename/add APVTS
  params in Stage 2 (would break state checksums + Stage 1 contract). String IDs `"start"`/`"end"`
  map to C++ `regionStart`/`regionEnd` (bare `end` shadows `juce::end` under `using namespace`).
- Engine constants already defined in PluginProcessor.h: `kMaxVoices=16`, `kMaxGrainsPerVoice=4`,
  `kRootNote=60`, `kMaxSourceSeconds=30`, `kStretchGrainMs=60`, `kNumBuiltIns=4`.
- **Dual-NAMESPACE binary-data split** documented as a Stage-1 CMake TODO: UI resources →
  `NAMESPACE UIBinaryData` (Stage 3), samples → `NAMESPACE BinaryData` (this stage). Two
  `juce_add_binary_data` targets must NOT collide on the `BinaryData` namespace
  (project memory: O-simpleGrain Stage 3.1 duplicate-symbol regression).
- State persistence (APVTS tree + custom `SOURCE/identity` child, default `embedded:piano`)
  exists — keep `identity` semantics consistent with the piano built-in.

## Key DSP decisions (from Stage 0, unchanged — for research/plan to confirm)

- Repitch = continuous fractional-read varispeed; Stretch = synchronous-granular SOLA
  (time 1× + per-grain resample, Hann overlap-add) reusing O-simpleGrain `GrainScheduler`/`GrainVoice`.
- Anti-alias: 4-pt Lagrange + rate-tracking one-pole; **no oversampling; zero latency** (`setLatencySamples(0)`).
- Loop: equal-power (sin/cos) crossfade + ping-pong + zero-cross snap (off-thread). Vintage:
  S&H decimation + bit-crush, **bypass at 0**, before the filter. Filter: per-voice
  `juce::dsp::StateVariableTPTFilter` LP + closed-form magnitude curve; lead-voice drives the curve.
- Viz tap (lock-free): `displayPlayhead`, `displayCutoffHz`/`displayK`, optional `VizRing`
  mono-sum scope (reuse O-simpleGrain `VizAnalyzer.h`). No alloc/FFT/locks on the audio thread.

## Sibling references (reuse priority)

- **O-simpleGrain** — PRIMARY: voice skeleton, `LagrangeInterpolation.h`, `GrainScheduler`/`GrainVoice`
  (Stretch), `VizAnalyzer.h`, render-harness, dual binary-data NAMESPACE pattern, `piano.wav`.
- **O-simpleSubtractive** — filter (`StateVariableTPTFilter`) / ADSR / voice patterns.
- **O-simpleFM / O-simpleAdditive** — voice skeleton + bit-depth lesson.
- **O-GrainScatter / O-Freeze** — overlap-add + loop crossfade.

## Open items for research phase

1. Exact port surface of `LagrangeInterpolation.h` + rate-tracking one-pole from O-simpleGrain
   (signatures, edge handling at region/loop boundaries).
2. Off-thread decode→resample→atomic-publish pattern (which JUCE classes; how O-simpleGrain
   publishes its buffer; how to avoid audio-thread alloc on sample swap). PERF-01.
3. `sourceSample` combo reconciliation: collapse to 1 entry vs keep 4 labels with piano-only wired
   — pick the lowest-risk approach that preserves APVTS/state stability.
4. `piano.wav` actual pitch → correct `rootKey` seed (probe method).
5. 2nd `juce_add_binary_data` target wiring (NAMESPACE `BinaryData`), confirm no namespace collision
   with the (future) UI target.
6. Velocity→amp curve (`velToAmp`) shape; `SmoothedValue` ramp lengths for click-free note-on/off.

---

# Phase 2.2 — discuss (continued) · 2026-06-25

**Trigger:** Phase 2.1 verified PASS (first audio: Repitch + region + amp ADSR + piano decode; auval + pluginval@5 SUCCESS). DAW play-test gate **CLEARED** — root pitch, transpose, Start/End region, and Tune/Fine all confirmed playing correctly; no blocking issues. Re-entering discuss to scope **Phase 2.2 (the full tone chain)**.

## Decisions resolved (Phase 2.2)

### D3 — 2.1 DAW gate: **PASSED, proceed**
- The CONTEXT-D2 STOP is cleared. O-simpleSampler-dev plays correctly in the DAW; no new issues surfaced beyond the already-captured region-end hard-cut click (a known 2.1 verify warning, scoped into 2.2 below). No 2.1 rework needed.

### D4 — Phase 2.2 execution: **SPLIT into 2.2a (tone chain) → 2.2b (Stretch)**
- Phase 2.2 bundles 5 DSP blocks; **Stretch SOLA is the highest-risk, headline component**. Split into two execute passes, each with its own DAW checkpoint (matching the 2.1 design-conscious test rhythm; isolates the risky engine from the inherited-pattern work).
- **Phase 2.2a — Tone chain (lower risk, all inherited patterns):**
  - **Region-end declick** — fold the 2.1 verify Warning 1 fix here: short declick ramp at `readPos≥endSamp` (`SampleVoice.h:185-189` currently `ampEnv.reset(); break;` → audible click when lowering End). The most likely audible artifact; address first.
  - **Loop engine** — `loopMode` (off/forward/ping-pong); `loopStart`/`loopEnd` (% of region); equal-power (sin/cos) crossfade over `loopCrossfade` ms via a second read head; zero-crossing marker snap (off-thread). `reverse` negates read direction. DSP-03, FUNC-05/06.
  - **Vintage** — per-voice S&H decimation (`fsEff=lerp(fs,3000,vintage)`) + bit-crush (`bits=lerp(clean,8,vintage)`); **full bypass at 0**; placed before the filter. DSP-04.
  - **Filter** — per-voice `juce::dsp::StateVariableTPTFilter<float>` LP; `filterCutoff` (log) + `filterResonance`→Q; lead-voice `displayCutoffHz`/`displayK` atomics staged for the Stage-3 curve. DSP-05.
  - **Checkpoint:** build VST3+AU+Standalone, auval + pluginval@5, install, **STOP for DAW play-test** (loop sustains a short sound, no seam click; ping-pong/reverse; Vintage clean-at-zero → grit; filter shapes tone) before 2.2b.
- **Phase 2.2b — Stretch SOLA (HEADLINE):**
  - Synchronous-granular SOLA — time-axis read head at 1× realtime + per-grain resample by `keyRatio`, Hann overlap-add (fixed ~60 ms grain, 2× overlap, `kMaxGrainsPerVoice=4`); mined from O-simpleGrain `GrainVoice`/`GrainScheduler`. `pitchMode` toggles Repitch↔Stretch. DSP-01.
  - **Checkpoint:** DAW A/B — toggling Pitch Mode on a sustained sample, the Repitch↔Stretch difference is obvious (low note slows/lengthens in Repitch; held note keeps duration while pitch tracks key in Stretch).

### D5 — Stretch fidelity bar (v1.0): **SOLA, tune the grain**
- Ship the planned **synchronous-granular SOLA**; invest tuning effort in grain size/overlap so the Repitch↔Stretch A/B is **obvious and clean enough to read as "same length, different pitch"** for a teaching tool. Phase-vocoder / PSOLA HQ mode **deferred to v1.1** (architecture decision, unchanged).
- Grain-tuning levers: fixed grain length (~60 ms baseline — tune for transient vs smearing), 2× Hann overlap, pool size 4. Validate with the 2.3 render-harness **single-grain autocorrelation probe** (the grain-rate comb confounds spectral probes — project memory).

## Phase 2.2 scope summary (immediate forward target)

| Pass | Components | Risk | Gate |
|------|-----------|------|------|
| **2.2a** | region-end declick · loop (fwd/ping-pong + equal-power crossfade + zero-cross snap) · reverse · Vintage (S&H + bit-crush, bypass@0) · filter (TPT LP) | Low (inherited) | DAW play-test |
| **2.2b** | Stretch SOLA (`pitchMode` toggle, granular reuse of O-simpleGrain scheduler) | **High (headline)** | DAW A/B test |

*2.3 (AA hardening + viz taps + voice-stealing + RT-safety + offline render-harness) remains the subsequent Stage-2 pass — unchanged.*

## Carry-forward constraints (still binding in 2.2)

- **21-param APVTS frozen** — read existing IDs only; the deferred params (`loopMode`, `loopStart`, `loopEnd`, `loopCrossfade`, `reverse`, `pitchMode`, `vintage`, `filterCutoff`, `filterResonance`) are already in the layout and currently inert — 2.2 wires them, no APVTS changes.
- 2.3 hardening backlog accepted in 2.1 verify (message-thread reclaim queue for the source-swap free; `std::atomic_load/store(shared_ptr)` C++20 deprecation; `setValueNotifyingHost`-in-prepare advisability) stays in 2.3 scope — do **not** pull forward into 2.2.
- Vintage/filter/loop are **per-voice**; lead-voice drives any display atomics (curve wiring lands visually in Stage 3, but stage the atomics now).

## Open items for research phase (Phase 2.2)

1. **Loop crossfade mechanics** — exact second-read-head equal-power crossfade + ping-pong direction handling at the loop seam from O-simpleGrain / O-GrainScatter / O-Freeze (signatures, how the two read heads are mixed, zero-cross snap off-thread).
2. **Region-end declick ramp** — shortest ramp length that kills the hard-cut click without truncating audible content; reconcile with the existing 0.2 s release tail (does the release already cover it, or is a dedicated end-ramp needed?).
3. **Stretch SOLA port surface** — exact `GrainVoice`/`GrainScheduler` reuse from O-simpleGrain: how to drive it at time-1× with per-grain `keyRatio` resample; grain-pool lifetime per voice; how `pitchMode` switches the read path cleanly mid-note (or only at note-on).
4. **Vintage formulation** — confirm `fsEff`/`bits` lerp endpoints and the S&H + quantize order; verify bit-for-bit clean at `vintage==0` (O-simpleAdditive bit-depth lesson reference).
5. **Filter** — `StateVariableTPTFilter` LP per-voice setup + `filterResonance`→Q mapping + closed-form `|H_LP|` g/k for the lead-voice display atomics (O-simpleSubtractive reference).
6. **Grain-tuning starting point** — O-simpleGrain's shipped grain length/overlap as the 2.2b baseline before the harness autocorrelation tuning pass.
