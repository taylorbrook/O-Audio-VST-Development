# Stage 2 (DSP) — PLAN

**Plugin:** O-simpleSampler
**Stage:** 2 of 4 — DSP
**Phase:** plan → complete (re-entered for **Phase 2.2**)
**Date:** 2026-06-25
**Inputs:** stages/2-dsp/CONTEXT.md (D1/D2 + Phase-2.2 D3/D4/D5), stages/2-dsp/RESEARCH.md (Phase-2.1 §1–§16 + **Phase-2.2 deep research P0–P9**), research/ARCHITECTURE.md, ROADMAP.md, parameter-spec.md, **shipped 2.1 `Source/SampleVoice.h` + `Source/PluginProcessor.{h,cpp}`**.

---

## Goal

Turn the silent 16-voice Stage-1 shell into a playable, professional-sounding educational
sampler. Stage 2 is internally **3 phases**; per **CONTEXT D2** the execute phase implements
one phase per pass, then STOPS for a DAW play-test.

- **2.1 — ✅ COMPLETE + DAW gate cleared:** Repitch fractional-read varispeed + start/end region
  + amp ADSR + `velToAmp` + built-in `piano.wav` decode/resample/atomic-publish → **first audio**.
- **2.2 — THIS plan (split per CONTEXT D4):** **2.2a tone chain** (region-end declick · loop
  fwd/ping-pong + equal-power crossfade + zero-cross snap · reverse · Vintage S&H+bit-crush ·
  resonant LP filter) → DAW checkpoint → **2.2b Stretch SOLA** (synchronous-granular, `pitchMode`
  toggle) → DAW A/B.
- **2.3 — forward scope (final pass):** lock-free viz taps + voice-stealing audit + AA hardening
  + RT-safety + offline render-harness (the Stage-2 correctness gate).

## Frozen constraints (carry-forward — still binding in 2.2)

- **21-param APVTS is frozen** — read existing IDs only; no rename/add (breaks state checksums).
  The 9 deferred params (`loopMode`/`loopStart`/`loopEnd`/`loopCrossfade`/`reverse`/`pitchMode`/
  `vintage`/`filterCutoff`/`filterResonance`) are **already in the layout and already cached as
  atomic pointers** (`PluginProcessor.h:239-250`) — currently inert. 2.2 wires them; **no APVTS change.**
- Constants in `PluginProcessor.h`: `kMaxVoices=16`, `kMaxGrainsPerVoice=4`, `kRootNote=60`,
  `kMaxSourceSeconds=30`, `kStretchGrainMs=60`, `kNumBuiltIns=4`.
- String IDs `"start"`/`"end"` → C++ `regionStart`/`regionEnd` (bare `end` shadows `juce::end`).
  Class is `SampleVoice`/`SampleSound` (NOT `Sampler*` — `juce::SamplerVoice` collides under `using namespace juce`).
- `setLatencySamples(0)` — `getLatencySamples()` non-virtual in JUCE 8; never override. **Stretch stays zero-latency.**
- Vintage/filter/loop are **per-voice**; the **loudest-active lead voice** drives display atomics
  (curve *drawing* is Stage 3, but **stage the cutoff/K atomics now** — CONTEXT D4).
- 2.3 hardening backlog (message-thread reclaim queue for the source-swap free; `std::atomic_load/store(shared_ptr)`
  C++20 deprecation; `setValueNotifyingHost`-in-prepare advisability) stays in **2.3** — do NOT pull into 2.2.

---

# Phase 2.1 — Core Playable Sampler — ✅ COMPLETE (2026-06-25)

Shipped: `SampleSound`/`SampleVoice` (Repitch read head + 4-pt Lagrange + rate-tracking AA one-pole
+ amp ADSR + `velToAmp` blend), `piano.wav` embedded via 2nd `juce_add_binary_data` (`NAMESPACE BinaryData`),
decode→resample→atomic-`shared_ptr`-publish, `sourceSample` Listener→AsyncUpdater, per-source root seed
(piano = **48**), restore-aware `setStateInformation`. Build clean (VST3+AU+Standalone); `auval` SUCCEEDED
(21 params); pluginval@5 SUCCESS; **DAW play-test gate CLEARED** (root pitch, transpose, Start/End, Tune/Fine
all correct). Verify PASS — DSP critic clean. One warning carried into 2.2a: **region-end hard-cut click**
(`SampleVoice.h:185-189` does `ampEnv.reset(); break;`). See VERIFICATION.md.

---

# Phase 2.2 — Full Tone Chain → Stretch (IMMEDIATE EXECUTE TARGET, split 2.2a → 2.2b)

**Where it bolts on (RESEARCH P1).** The shipped 2.1 per-sample chain (`SampleVoice.h:167-190`) is
`readSourceLagrange(readPos)` → `aaOnePole` → `VCA (ampEnv·velLevel)` → `addSample`; advance `readPos += voiceRate`;
one-shot `if (readPos≥endSamp) { ampEnv.reset(); break; }`. Phase 2.2 inserts a shared downstream tail
**Vintage → Filter → VCA(·endRamp)** between source-generation and the existing VCA, and forks **only the read head**
on `pitchMode` (Repitch continuous head vs Stretch grain overlap-add). The processor pushes the 9 now-live params
into an extended `SamplerVoiceParams` once per block (`PluginProcessor.cpp:459-481`, alongside the existing
`startSamp/endSamp` math).

---

## Phase 2.2a — Tone chain (lower risk, inherited patterns) → DAW checkpoint

**Goal:** A sustaining, tone-shaped sampler — region loops seamlessly (fwd/ping-pong + equal-power crossfade,
zero-cross snapped), plays reverse, region-end no longer clicks, coloured by a clean-at-zero Vintage macro and a
resonant LP filter; lead-voice cutoff/K atomics staged for the Stage-3 curve.

### Tasks (2.2a)

1. [ ] **Extend `SamplerVoiceParams` + processor param-push** *(do first — every later task fills fields)*
   - Add to the push struct (`SampleVoice.h:48-57`): `int loopMode=0`, `int loopStartSamp=0`, `int loopEndSamp=0`
     (ABS source-frame, exclusive), `int xfadeSamp=0`, `bool reverse=false`, `int pitchMode=0` (**latched at note-on**),
     `float vintage=0`, `float filterCutoff=20000`, `float filterQ=0.707`.
   - Processor (`PluginProcessor.cpp:459-481`): read the already-cached atomics (`loopModeParam`…`filterResonanceParam`,
     `PluginProcessor.h:239-250`). Loop bounds = **% of region**: `loopAbsStart = startSamp + loopStart%·(endSamp−startSamp)`,
     `loopAbsEnd = startSamp + loopEnd%·(endSamp−startSamp)`, clamp `loopAbsStart < loopAbsEnd ≤ endSamp`; overwrite with the
     zero-cross-snapped indices (Task 5). `xfadeSamp = jlimit(0, (loopAbsEnd−loopAbsStart)/2, (int)(loopCrossfade·0.001·fs))`.
   - **Filter smoothing lives in the processor, NOT 16× per-voice** (no per-note filter mod in v1): add two
     `SmoothedValue<float>` (cutoff, Q; `reset(sr, 0.02)` like `outputGain` at `PluginProcessor.h:206`/`:225`); per block
     `setTargetValue` then push the current scalar `filterCutoff` (Hz, `jlimit(20, 0.45·fs)`) + `filterQ`
     (`jmap(filterResonance,0,100, 0.707f, 12.0f)` — **net-new map; do NOT reuse `resonanceToK`**).
   - Files: `Source/SampleVoice.h`, `Source/PluginProcessor.{h,cpp}`
   - Depends on: none · Ref: RESEARCH P1.1, P1.2, P5.2

2. [ ] **Region-end declick (raised-cosine end-ramp)** — fixes the 2.1 verify warning
   - The 0.2 s amp release is **structurally unreachable** from region-end: `SampleVoice.h:185-189` calls `ampEnv.reset()`
     (instant idle), not `noteOff()`. A dedicated end-ramp is **required** — do NOT widen `ampRelease`.
   - On the **one-shot path only** (`loopMode==off`): `endRampSamp = jmin((endSamp−startSamp)/4, (int)(0.005·fs))` (≤5 ms);
     when `distToEnd = endSamp − readPos < endRampSamp`, multiply the VCA by
     `0.5 − 0.5·cos(π·max(0,distToEnd)/endRampSamp)`. **Reverse** terminates at `startSamp` → taper `readPos − startSamp`.
     When looping, the seam crossfade handles continuity — bypass this ramp.
   - Files: `Source/SampleVoice.h` · Depends on: Task 1 · Ref: RESEARCH P2

3. [ ] **Loop engine — forward dual-head equal-power crossfade (PORT)**
   - PORT `O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp:31-111` (`equalPowerWeights` + `readVariantWithLoop` +
     `wrapLoopPosition`), generalizing the fixed-8 fade to `xfadeSamp`. Equal-power primitive:
     `t = jlimit(0,1,x)·halfPi; return {cos(t), sin(t)}`. When `readPos ≥ loopEnd − xfadeSamp`, mix a 2nd head from
     `loopStart`: `s = out·w.first + in·w.second`, `in` read at `readPos − loopLen + xfadeSamp`. Advance then wrap by
     **subtracting `loopLen`** (`while (readPos ≥ loopEnd) readPos −= loopLen`) — preserves sub-sample phase, never hard-jumps.
     AA one-pole runs on the mixed `s` exactly as 2.1.
   - Files: `Source/SampleVoice.h` · Depends on: Task 1 · Ref: RESEARCH P3.1, P3.2

4. [ ] **Loop engine — ping-pong reflect + reverse seed (NET-NEW)**
   - Per-voice `int dir = +1`. Advance `readPos += voiceRate·dir`; reflect at bounds:
     `if (dir>0 && readPos≥loopEnd) { readPos = 2·loopEnd − readPos; dir = −1; }` and the symmetric start case; crossfade the
     turnaround with the same equal-power pair (reflected head reads the opposite direction).
   - **Reverse** composes independently: seed `dir = −1`, `readPos = endSamp` at note-on (ARCHITECTURE: reverse negates the
     base increment independent of loop direction).
   - Files: `Source/SampleVoice.h` · Depends on: Task 3 · Ref: RESEARCH P3.3

5. [ ] **Zero-cross snap (NET-NEW, off-thread) — extend the existing Listener+AsyncUpdater**
   - The processor is **already** an APVTS `Listener` + `AsyncUpdater` (sourceSample, `PluginProcessor.cpp:199,407,417`).
     Add `start`/`end`/`loopStart`/`loopEnd` to `addParameterListener`; in `parameterChanged` set a `pendingSnap` flag +
     `triggerAsyncUpdate()`. On the message thread, scan ±W (≈256) source samples around each nominal marker for the nearest
     sign change minimizing `|src[i]|`; store snapped indices in `std::atomic<int>` members the audio thread reads once/block
     (Task 1 overwrites the %-of-region bounds with these).
   - **Secondary defense only** — the crossfade is the primary seam smoother; the snap keeps even `loopCrossfade=0` reasonable.
   - Files: `Source/PluginProcessor.{h,cpp}` · Depends on: Task 1 · Ref: RESEARCH P3.4

6. [ ] **Vintage — S&H decimate (NET-NEW) → bit-crush (VERBATIM), bypass at 0, before the filter**
   - Per-voice, gated `if (vintage > 0)` on **both** stages (full bypass at 0 = bit-for-bit clean = hard acceptance DSP-04).
     **Order: decimate → quantize.**
   - **S&H (net-new — grep-confirmed no in-repo idiom):** per-voice `float shPhase=0, shHeld=0`;
     `fsEff = jmap(vintage,0,100, fs, 3000)`; `shPhase += fsEff/fs; if (shPhase≥1){ shPhase−=1; shHeld=s; } s = shHeld;`.
     Reset `shPhase=shHeld=0` on note-on.
   - **Bit-crush (verbatim idiom — `O-simpleAdditive/Source/AdditiveVoice.h:335-346`):** precompute per block
     `bits = jmap(vintage,0,100, 16, 8)`, `qLevel = exp2(bits−1)`, `qInv = 1/qLevel`; per sample `s = round(s·qLevel)·qInv`.
     No dither. Bounded ops → covered by the existing block `isfinite` scrub.
   - Files: `Source/SampleVoice.h` · Depends on: Task 1 · Ref: RESEARCH P4.1, P4.2, P4.3

7. [ ] **Resonant LP filter — per-voice `juce::dsp::StateVariableTPTFilter<float>`**
   - In `SampleVoice::prepareToPlay`: `filt.prepare({ fs, (uint32) blockSize, 1 })` (mono), `filt.setType(lowpass)`.
     `filt.reset()` in `startNote` (clear state so a stolen voice carries no tail). Per sample: `s = filt.processSample(0, s)`
     **after Vintage, before the VCA**.
   - `setCutoffFrequency`/`setResonance` push the **processor-smoothed** scalars **once per block** (one `tan` recompute — NOT
     per sample). `setResonance` is **Q-like** and **asserts `>0`** — never pass 0 (Task-1 `jmap` floor is 0.707).
   - Files: `Source/SampleVoice.h` · Depends on: Task 1 · Ref: RESEARCH P5.1

8. [ ] **Lead-voice display atomics (loudest-active) — stage cutoff/K now (CONTEXT D4)**
   - Copy `O-simpleSubtractive` loudest-active selection (`PluginProcessor.cpp:283/410` pattern): once per block pick the
     max amp-env voice and publish `displayCutoffHz = smoothedCutoff` and `displayK = 1/Q` (= JUCE's `R2`) into `std::atomic`
     members. The bridge for Stage 3: copy `O-simpleSubtractive/Source/SubVizAnalyzer.h:71-110` `SubFilterCurve::magnitudeDb`
     **verbatim** — it expects `k = 1/Q`, so audio and curve match by construction (QUAL-02). **Curve drawing is Stage 3;**
     only the atomics + the helper header land now. (`displayPlayhead` + VizRing scope stay in 2.3.)
   - Files: `Source/PluginProcessor.{h,cpp}`, `Source/SubVizAnalyzer.h` (copy, for Stage 3) · Depends on: Task 7 · Ref: RESEARCH P5.3

9. [ ] **Build, validate, STOP for DAW checkpoint (2.2a gate)**
   - `ninja O-simpleSampler_VST3 O-simpleSampler_AU O-simpleSampler_Standalone` clean; `auval -v aumu OsSm OuDv` SUCCEEDED
     (still 21 params); pluginval@5 SUCCESS; install (build-and-install dual-variant sweep) + **STOP**.
   - DAW play-test: loop sustains a short sound with **no seam click** (crossfade 0/10/100 ms); ping-pong + reverse correct;
     region-end no longer clicks when End is dragged down on a held note; Vintage **clean at 0** → grit as raised; filter
     shapes tone, LP open at default, **no zipper** on cutoff/res sweeps.
   - Files: none (build/validation) · Depends on: Tasks 1–8

---

## Phase 2.2b — Stretch SOLA (HEADLINE, high risk) → DAW A/B

**Goal:** `pitchMode` toggles Repitch ↔ synchronous-granular **Stretch** — a held note keeps its duration while pitch tracks
the key, distinct enough from Repitch to read as "same length, different pitch" for a teaching tool. Feeds the same
Vintage→Filter→VCA tail; loop-wrap/end-ramp operate on the time axis.

### Tasks (2.2b)

10. [ ] **Copy grain machinery (verbatim)**
    - `cp O-simpleGrain/Source/dsp/Grain.h` and `cp O-simpleGrain/Source/dsp/WindowLuts.h` (Hann = shape 4) into
      `Source/dsp/`. The `Grain` POD is **SOLA-native** (independent `readPos` = time axis, `rate` = pitch axis; per-grain
      `aaCoeff/aaEngaged/aaState` + Hann `phase/phaseInc` already present). `readSourceLagrange`/`aaOnePole` already in
      `SampleVoice.h` from 2.1 — reuse per grain.
    - Files: `Source/dsp/Grain.h`, `Source/dsp/WindowLuts.h` (new) · Depends on: none · Ref: RESEARCH P6, P6.1

11. [ ] **Voice-local time axis + SOLA grain scheduler**
    - Add per-voice `std::array<Grain, kMaxGrainsPerVoice>` (pool=4 — active grains = overlap = 2 at 2× overlap; pool 4 = 2×
      headroom; `spawnGrain` steal-oldest never allocates), `double timePos`, `int samplesUntilNextGrain`.
    - **The one structural gap (RESEARCH P6.2):** advance `timePos` **per output sample** (`timePos += 1.0·dir` — 1× realtime,
      duration preserved regardless of key), NOT per block. Loop-wrap/reverse operate on `timePos` (the time axis is what loops).
    - **Fixed SOLA hop (replaces O-simpleGrain's density scheduler, P6.3):** `lenSamp = max(2, kStretchGrainMs·0.001·fs)` (60 ms),
      `phaseInc = 1/lenSamp`; on fire `spawnGrainSOLA(g.readPos = timePos; g.rate = voiceRate;` zero ALL spray —
      `positionSpray/pitchSpray/scatter/panSpray = 0`; per-grain AA coeff from `g.rate`); then
      `samplesUntilNextGrain = max(1, (int)(lenSamp·0.5))` ⇒ hop = grainSize/2 ⇒ **2× Hann overlap, no jitter**.
    - Render = overlap-add the active grains (reuse 2.1 `readSourceLagrange`+`aaOnePole`+Hann LUT); **drop the `√overlap`
      normalizer** (`O-simpleGrain/PluginProcessor.cpp:755-756`) — 2× Hann is already unity-gain (COLA).
    - Files: `Source/SampleVoice.h` · Depends on: Task 10 · Ref: RESEARCH P6.2, P6.3, P6.4, P6.5

12. [ ] **`pitchMode` latched at note-on + shared downstream**
    - Latch `pitchMode` per voice in `startNote` (mid-note switch clicks — truncates in-flight grain windows; drain pattern
      deferred to v1.1). On note-on for Stretch: reset the pool (`g.active=false` for all), `samplesUntilNextGrain=0` (fire
      immediately), seed `timePos` at the read position so there's no jump.
    - The overlap-add sum `s` feeds the **same Vintage → Filter → VCA(·endRamp)** tail as Repitch (P1). The end-ramp (Task 2)
      and loop-wrap (Tasks 3–4) act on `timePos` in Stretch.
    - Files: `Source/SampleVoice.h` · Depends on: Tasks 11, 2, 3 · Ref: RESEARCH P6.5, P6.6

13. [ ] **Build, validate, STOP for DAW A/B (2.2b gate)**
    - Build VST3+AU+Standalone clean; `auval` SUCCEEDED; pluginval@5 SUCCESS; install + **STOP**.
    - DAW A/B: toggle Pitch Mode on a sustained note — **Repitch** slows/lengthens a low note (pitch+time coupled);
      **Stretch** holds duration while pitch tracks the key. Difference must be **obvious and clean**. Tune grain length
      against the 2.3 single-grain autocorr harness if the A/B smears (levers: longer grains = less transient smear;
      3× overlap fallback — phase-vocoder stays v1.1-deferred).
    - Files: none (build/validation) · Depends on: Tasks 10–12

---

## Phase 2.2 Success Criteria

- [ ] **Loop forward** sustains without dropout; **no click at the seam** (crossfade 0/10/100 ms); **ping-pong + reverse** correct.
- [ ] **Region-end no longer clicks** when End is lowered on a held one-shot note (raised-cosine end-ramp).
- [ ] **Vintage 0% bit-for-bit clean**; increasing adds grit; no NaNs across the range; **before** the filter.
- [ ] **Filter** cutoff/resonance audibly shape tone; LP open (Butterworth) at default; **no zipper**; `setResonance` never 0.
- [ ] Lead-voice `displayCutoffHz`/`displayK`(=1/Q) atomics published once/block (curve drawing deferred to Stage 3).
- [ ] **Repitch vs Stretch obvious** on a sustained note (Repitch couples pitch+time; Stretch holds duration, pitch tracks key);
      `pitchMode` latched at note-on (no mid-note click); zero latency retained.
- [ ] Both DAW checkpoints (2.2a tone chain, 2.2b A/B) pass before proceeding to 2.3.

---

# Phase 2.3 — Hardening + Viz + Render-Harness (forward scope; final pass)

> The Stage-2 correctness gate. Captured for completeness; not part of this execute.

## Tasks (2.3)

14. [ ] **`displayPlayhead` + viz tap** — copy `O-simpleGrain/Source/VizAnalyzer.h` verbatim → `SamplerVizAnalyzer.h`;
    `VizRing` write at tail of `processBlock` (≤4096 stack chunks, post-gain); FFT on editor Timer (Stage 3 consumes);
    publish `displayPlayhead` (lead-voice `readPos`/`timePos`). No alloc/FFT/locks on the audio thread. — RESEARCH §12
15. [ ] **Voice-stealing audit + AA hardening + RT-safety** — confirm 16-voice stealing, no stuck notes; high-key + extreme-Stretch
    AA budget; denormal/`isfinite` audit; the 2.1 hardening backlog (message-thread reclaim queue for the source-swap free;
    `std::atomic_load/store(shared_ptr)` C++20 deprecation; `setValueNotifyingHost`-in-prepare advisability). — RESEARCH §14
16. [ ] **Offline render-harness (correctness gate)** — port `O-simpleGrain/tests/render-harness/{main.cpp,CMakeLists.txt}`;
    `target_sources` plugin `PluginProcessor.cpp` + `PluginEditor.cpp`; `add_dependencies` on plugin + both binary-data targets;
    `JUCE_MODAL_LOOPS_PERMITTED=1` (pump AsyncUpdater decode); gate `option(OUARICON_BUILD_TESTS OFF)`. — RESEARCH §13

## Success Criteria (2.3) — Stage-2 correctness gate

- [ ] `processBlock` allocation-free; sample load does not glitch/block the audio thread; latency reported 0.
- [ ] 16 voices, graceful voice-stealing, no stuck notes.
- [ ] Render-harness asserts: **Repitch tuning** (f0 at root = 131 Hz, octave = 2×, `autocorrPitchHz`); **Stretch pitch/time
      independence** (**single-grain autocorr** probe — spectral bins confounded by grain comb, project memory); **loop-seam
      click absence** (`continuityFraction`, fwd + ping-pong); **region-end declick** (`continuityFraction`, End lowered mid-hold);
      **Vintage clean-at-zero** (bit-for-bit); **anti-alias budget** on high keys + extreme Stretch.

---

## Files (Phase 2.2)

**New (2.2b):** `Source/dsp/Grain.h`, `Source/dsp/WindowLuts.h`
**New (2.2a, for Stage 3 curve):** `Source/SubVizAnalyzer.h` (copy verbatim; atomics consumed now, drawing in Stage 3)
**Modified (2.2a/2.2b):** `Source/SampleVoice.h` (loop/declick/Vintage/filter/Stretch render path + `SamplerVoiceParams`),
`Source/PluginProcessor.{h,cpp}` (param-push loop bounds/xfade/Vintage/filter; processor `SmoothedValue` cutoff/Q;
zero-cross-snap AsyncUpdater; loudest-active display atomics)

## Reuse map (copy paths — Phase 2.2)

| Component | From | Pass | Kind |
|-----------|------|------|------|
| `equalPowerWeights` + dual-head crossfade + `wrapLoopPosition` | `O-MicrotonalSampler/MicrotonalSamplerVoice.cpp:31-111` | 2.2a | PORT (8→xfadeSamp) |
| raised-cosine end-ramp | `O-MicrotonalSampler/PluginProcessor.cpp:489-510` | 2.2a | port idiom |
| bit-crush | `O-simpleAdditive/AdditiveVoice.h:335-346` | 2.2a | verbatim idiom |
| `SubFilterCurve::magnitudeDb` (`k=1/Q`) | `O-simpleSubtractive/SubVizAnalyzer.h:71-110` | 2.2a (atomics) / Stage 3 (draw) | verbatim |
| `StateVariableTPTFilter` | JUCE 8.0.9 (`setResonance` Q-like, asserts >0) | 2.2a | JUCE class |
| `Grain.h`, `WindowLuts.h` | `O-simpleGrain/Source/dsp/` | 2.2b | verbatim |
| grain spawn/render/AA | `O-simpleGrain/GrainVoice.h:228-263,308-422` | 2.2b | port + SOLA edits |

**Net-new (no port):** ping-pong reflection (P3.3), zero-cross snap (P3.4), S&H decimation (P4.2),
res%→Q `jmap` (P5.2 — NOT `resonanceToK`), voice-local `timePos` + fixed SOLA hop (P6.2/P6.3).

## Key risks (top 4)

- **Stretch not clearly distinct from Repitch (DSP-01, HIGH)** → voice-local 1× `timePos` + fixed `grainSize/2` hop +
  60 ms/2× baseline (NOT O-simpleGrain's 30 ms/1.2×); drop √overlap normalizer; tune vs 2.3 single-grain autocorr harness.
- **Loop/region-end clicks (MED)** → PORT the shipped equal-power dual-head crossfade (never hard-jump, wrap subtracts loopLen);
  dedicated raised-cosine end-ramp (the 0.2 s release is unreachable from the region-end path); zero-cross snap as secondary defense.
- **Vintage not bit-clean at 0 (MED)** → explicit `vintage>0` gate on BOTH S&H (net-new, mandatory gate) and bit-crush.
- **Filter `setResonance(0)` assert / Q-vs-k confusion (LOW)** → map res%→Q `0.707..12` (never 0); curve fed `k=1/Q=R2` (verified vs JUCE source).

---
*Plan complete 2026-06-25. Execute target: **Phase 2.2 — split 2.2a tone chain → DAW checkpoint → 2.2b Stretch SOLA → DAW A/B**
(CONTEXT D4/D5). Supersedes the pre-2.1 coarse forward-scope where it conflicts (loop = PORT not net-new; declick mandatory;
S&H net-new; TPT filter wiring net-new; SOLA `timePos` gap; pitchMode latched at note-on). Next: execute phase.*
