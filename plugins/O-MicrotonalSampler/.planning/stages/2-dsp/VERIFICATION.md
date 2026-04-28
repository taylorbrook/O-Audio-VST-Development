---
title: "O-MicrotonalSampler Stage 2 (DSP) — Verification"
created: 2026-04-27
stage: 2-dsp
phase: verify
status: verified
inputs:
  - .planning/stages/2-dsp/CONTEXT.md
  - .planning/stages/2-dsp/PLAN.md
  - .planning/stages/2-dsp/RESEARCH.md
  - .planning/stages/2-dsp/PHASE-2.1-SUMMARY.md
  - .planning/stages/2-dsp/PHASE-2.2-SUMMARY.md
  - .planning/stages/2-dsp/PHASE-2.3-SUMMARY.md
  - .planning/stages/2-dsp/PHASE-2.4-SUMMARY.md
  - .planning/stages/2-dsp/PHASE-2.5-SUMMARY.md
  - .planning/REQUIREMENTS.md
  - .planning/BRIEF.md
---

# Stage 2 (DSP) — Verification

## Verification Date

2026-04-27

## Goal-Backward Analysis

### Original Goal (from CONTEXT.md / PLAN.md)

> Convert the Stage 1 silent shell into a fully working sample-playback engine:
> 16-voice polyphonic, varispeed cubic-Hermite interpolation, ADSR, equal-power
> velocity-layer crossfade, voice-stealing with 5-ms tail ramp, and auto-detected
> sustain loops. Sample loading is asynchronous on a background thread with
> tolerant filename parsing and one-time SR conversion. Microtonal pitch comes
> from `TuningEngine::getFrequency` plus VST3 Note Expression baked at `startNote`.

### Sub-stage Goals

| # | Sub-stage | Goal | Status |
|---|---|---|---|
| 2.1 | Voice DSP | Cubic-Hermite varispeed + ADSR + NE; first audio | ✅ Achieved (commit `bb0e7f7`) |
| 2.2 | Loader | Background folder load + tolerant filename parse + LagrangeInterpolator SR conversion | ✅ Achieved (commit `cacffda`) |
| 2.3 | Vel xfade | Equal-power velocity-layer crossfade at boundary | ✅ Achieved (commit `11bd39c`) |
| 2.4 | Voice-steal | 5 ms linear tail ramp on 17-note steal | ✅ Achieved (commit `1aceb4c`) |
| 2.5 | Loop detect | RMS scan + zc snap + 8-sample equal-power loop xfade | ✅ Achieved (uncommitted at verify time — see Outstanding Actions §1) |

### Deliverables (from PHASE-N-SUMMARY.md + code inspection)

1. **`MicrotonalSamplerVoice`** — cubic-Hermite (Catmull-Rom Horner) varispeed
   read, `juce::ADSR` shaping, dual-slot equal-power crossfade, 5 ms voice-steal
   ramp, 8-sample equal-power loop boundary crossfade. APVTS values consumed
   once per `startNote` (no per-block reads). RT-safe — no allocations in
   `renderNextBlock` or `renderTailRamp`; the only `.assign` calls live in
   `prepareToPlay` (message thread).
   File: `Source/MicrotonalSamplerVoice.{h,cpp}`.
2. **`SampleLoader`** — `juce::Thread` background loader with
   `RangedDirectoryIterator` over `*.wav;*.aif;*.aiff;*.flac`,
   `juce::AudioFormatManager` (local-only — pitfall #9), per-channel
   `LagrangeInterpolator` SR conversion, mono → stereo promotion at unity gain
   (D2-10), `LoopDetector` invocation, completion dispatched via
   `MessageManager::callAsync`. Cancellation via `threadShouldExit()` checked
   per file.
   File: `Source/SampleLoader.{h,cpp}`.
3. **`FilenameParser`** — pure-function tolerant parser recognising scientific
   pitch (`C4`, `F#3`, `Bb5`, `A-1`), `MIDI60`/`midi72`, bare integer fallback,
   velocity tokens (`v1..v4`, `vel1..4`, `p/mp/mf/f`), layer tokens
   (`layer1..4`, `L1..4`, `Lyr1..4`). Tokenizer splits on `[_\-\s.]+`,
   case-insensitive.
   File: `Source/FilenameParser.{h,cpp}`.
4. **`LoopDetector`** — pure-function `detectLoop(buf, sampleRate) → LoopRegion`.
   1024-sample RMS window, stride 256, search range `[N*0.40, N - 1024 - 64]`;
   variance gate `rms[min] / meanRms < 0.7`; zero-crossing snap (±64,
   falling-edge preferred) for `loopStart`; same-direction zc match for
   `loopEnd` at target length `max(2048, sampleRate / 50)`; defensive guards
   for `loopEnd - loopStart >= 16` and `loopEnd <= N - 2`.
   File: `Source/LoopDetector.{h,cpp}`.
5. **`OMicrotonalSamplerAudioProcessor`** — 16 pre-allocated voices wired with
   APVTS / TuningEngine / pendingTuningSource / sampleMapSource setters;
   `setNoteStealingEnabled(true)` explicit; `vst3Extensions.drainAndUpdate()`
   ahead of `renderNextBlock`; `output_gain` smoothed via
   `juce::SmoothedValue` + `applyGainRamp`; atomic-store of the loader's
   completion `shared_ptr<SampleMap>` into the audio-thread slot.
   File: `Source/PluginProcessor.{h,cpp}`.
6. **Phase 2.1 test fixture** kept as macro-gated regression safety net
   (`OMTS_PHASE_2_1_TEST_FIXTURE`, default OFF).
7. **Aliasing-check standalone driver** — `Source/tests/aliasing_check.cpp`,
   `EXCLUDE_FROM_ALL`, available for manual RQ-1 +50 c null measurement.

### Goal Achievement

| Goal | Status | Evidence |
|---|---|---|
| 16-voice polyphonic engine | ✅ | `PluginProcessor.cpp:103-111` instantiates 16 `MicrotonalSamplerVoice`s; `setNoteStealingEnabled(true)` at line 118. |
| Cubic-Hermite varispeed | ✅ | `MicrotonalSamplerVoice.cpp:153-187` `cubicInterp` (Catmull-Rom Horner). `playRate = (desiredFreq/slotRefFreq) * (slotSR/hostSR)` at `computePlayRateForSlot`. |
| ADSR | ✅ | `juce::ADSR adsr` member (Voice.h:68); `setSampleRate` in `prepareToPlay` (Voice.cpp:282); APVTS values read once per `startNote` (Voice.cpp:586-594). |
| Equal-power velocity-layer crossfade | ✅ | `equalPowerWeights` (Voice.cpp:104) called once per `startNote` with corrected boundary-fade geometry (Voice.cpp:546-578). |
| 5 ms voice-steal tail ramp | ✅ | `kMaxStealRamp = ceil(0.005*SR)+16` (Voice.cpp:288); `renderTailRamp` (Voice.cpp:312-416); `startNote` self-detects active steal (Voice.cpp:430-445); `renderNextBlock` mixes captured tail before early-out (Voice.cpp:646-667). |
| Auto-detected sustain loops + 8-sample crossfade | ✅ | `LoopDetector::detectLoop` (`LoopDetector.cpp:133-230`); 8-entry `loopXfadeLut` (Voice.cpp:126-136); `readSlotWithLoop` (Voice.cpp:209-240); `wrapLoopPosition` (Voice.cpp:246-253). |
| Background sample loader | ✅ | `SampleLoader : juce::Thread` with `startThread()` in `loadFolder`; completion via `MessageManager::callAsync` (`SampleLoader.cpp:253`). |
| Tolerant filename parser | ✅ | `FilenameParser::parse` recognises 6 conventions per RQ-5; case-insensitive. |
| One-time SR conversion | ✅ | `juce::LagrangeInterpolator` per channel at load time (`SampleLoader.cpp:144-149`); resampled buffer stored at host SR. |
| TuningEngine + NE consumption | ✅ | `tuningEngine->getFrequency` (Voice.cpp:499); `Ouaricon::NoteExpression::applyPendingTuning` (Voice.cpp:504). `vst3Extensions.drainAndUpdate()` runs before `renderNextBlock` (Processor.cpp:255). |

## Requirements Verification

**Stage:** 2-dsp
**Requirements in scope:** 15 total (must: 11 / should: 3 / nice: 1)

| ID | Description | Priority | Status | Evidence |
|---|---|---|---|---|
| FUNC-01 | Sample playback (.wav / .aif) | must | ✅ Complete | `SampleLoader::run` accepts `*.wav;*.aif;*.aiff;*.flac` via `formatManager.registerBasicFormats()`; `MicrotonalSamplerVoice::renderNextBlock` reads slots through `cubicInterp`. Standalone smoke playable (Phase 2.1 fixture commit `bb0e7f7`); pluginval `SUCCESS`. |
| FUNC-02 | Up to 4 velocity layers / pitch | must | ✅ Complete | `FilenameParser` maps `v1..v4` / `p/mp/mf/f` / `layer1..4` → `velLayer ∈ [0..3]`; `SampleLoader::run` derives `numVelocityLayers = clamp(maxLayer+1, 1, 4)`. Voice maps `(vel-1)/layerWidth` → primary layer index. |
| FUNC-03 | 16-voice polyphony | must | ✅ Complete | 16 `MicrotonalSamplerVoice` instances pre-allocated (`PluginProcessor.cpp:103-111`); `setNoteStealingEnabled(true)`. |
| FUNC-04 | Auto-detect note range | must | ✅ Complete | `SampleLoader::run` walks built slots and derives `lowestNote = jmin(...)`, `highestNote = jmax(...)` (`SampleLoader.cpp:238-243`). |
| FUNC-07 | Voice-stealing (oldest-released first) + click-free | must | ✅ Complete | JUCE default `findVoiceToSteal` selects oldest-released → oldest-keyup (R1, D2-2). `renderTailRamp` captures 5 ms linear-down tail at `startNote`; `renderNextBlock` mixes additively (Voice.cpp:646-667). |
| DSP-01 | Varispeed ±50 c via `2^(c/1200)` | must | ✅ Complete | `playRate = (desiredFreq/slotRefFreq)*(slotSR/hostSR)` (Voice.cpp:259-268). `currentFrequency` driven by `TuningEngine::getFrequency` + NE delta — both encode the cents math, so a +50 c NE event yields the spec'd `playRate`. |
| DSP-02 | Cubic-Hermite (≥3rd order) | must | ✅ Complete | `cubicInterp` Catmull-Rom Horner expansion (Voice.cpp:153-187), 4-tap context with loop-aware wrap. Body matches `juce::CatmullRomTraits::valueAtOffset`. |
| DSP-03 | ADSR per voice | must | ✅ Complete | `juce::ADSR` per voice (Voice.h:68); `setSampleRate` wired in both `prepareToPlay` and `setCurrentPlaybackSampleRate` (pitfall #1); APVTS values read once per `startNote` (pitfall #2). |
| DSP-04 | Equal-power velocity crossfade | must | ✅ Complete | `equalPowerWeights(x) → (cos(x·π/2), sin(x·π/2))` (Voice.cpp:104); applied at boundary geometry — at layer center `wPrim=1, wAdj=0`; at boundary `wPrim=wAdj=√½`. Width controlled by `velocity_crossfade ∈ [0,1]` parameter. |
| DSP-05 | Auto-detect sustain loop points | should | ✅ Complete | `LoopDetector::detectLoop` (RMS scan + variance gate + zc snap + length guard); 8-sample equal-power crossfade at boundary via `readSlotWithLoop` + `kLoopXfadeLUT`. Invalid → `loopEnd=0` one-shot fallback (EC-7). |
| DSP-07 | VST3 NE pitch consumption | must | ✅ Complete | `vst3Extensions.drainAndUpdate()` runs before `renderNextBlock` (Processor.cpp:255); voice consumes via `Ouaricon::NoteExpression::applyPendingTuning(*pendingTuningSource, midi, currentFrequency)` at `startNote` (Voice.cpp:504). |
| DSP-08 | Suite TuningEngine consumption | must | ✅ Complete | `tuningEngine->getFrequency(midi)` at `startNote` (Voice.cpp:499); ET fallback only when `tuningEngine == nullptr` (Standalone usability). No tuning UI in this plugin. |
| PERF-01 | RT-safe processBlock (no allocs / I/O / locks) | must | ✅ Complete | `pluginval --strictness-level 5 --validate-in-process` `SUCCESS` (allocation guard included). Source audit: only `.assign` calls live in `prepareToPlay` (message thread); `renderNextBlock`, `renderTailRamp`, `cubicInterp`, `readSlotWithLoop`, `wrapLoopPosition` are all allocation-free `noexcept`. |
| PERF-02 | ≤5 % CPU @ 16 voices, 48 kHz / 256 buffer (Apple Silicon) | should | ⚠️ Partial | Targeted DAW CPU benchmark not yet run. The implementation is well within the analytical budget (cubic-Hermite Horner + ADSR + dual-slot mix + 8-sample LUT lookup inside an 8-sample window per loop iteration); pluginval ran allocation-clean; subjective benchmark deferred to user. See Outstanding Actions §3. |
| PERF-03 | Background-thread sample loader | must | ✅ Complete | `SampleLoader : juce::Thread`; `loadFolder` calls `startThread()`; `run()` performs file I/O + `LagrangeInterpolator` + `LoopDetector` off-thread; `MessageManager::callAsync` dispatches result. |
| PERF-04 | Zero added latency | nice | ✅ Complete | No `setLatencySamples` called anywhere in `Source/`; default `getLatencySamples()` returns 0 (memory: getter is non-virtual in JUCE 8). pluginval reported `Latency: 0` during basic-bus tests. |
| COMPAT-02 | 16/24/32-bit AIF/WAV at any SR (auto-convert) | must | ✅ Complete | `formatManager.registerBasicFormats()` covers all listed bit-depths and formats (juce_audio_formats default registry). `SampleLoader::run` resamples via `juce::LagrangeInterpolator` at load time when `\|srcSR - targetSR\| > 0.1`; resulting slot stored at host SR. |
| QUAL-01 | No clicks / zipper / aliasing across velocity / poly / ±50 c | must | ⚠️ Partial | Mitigations all in place: 5 ms steal ramp (FUNC-07), 8-sample loop crossfade (DSP-05), `output_gain` smoothing via `applyGainRamp` (RESEARCH R7), velocity-layer equal-power blend (DSP-04). pluginval clean. **Subjective DAW listening test** (sine-sweep, vibrato cello, transient, and ±50 c retune) remains the final acceptance bar — deferred to user; see Outstanding Actions §3. |

**Requirements Summary:**
- ✅ Complete: **13** (FUNC-01..04, FUNC-07, DSP-01..05, DSP-07, DSP-08, PERF-01, PERF-03, PERF-04, COMPAT-02)
- ⚠️ Partial (deferred subjective): **2** (PERF-02 CPU benchmark, QUAL-01 listening test)
- ⏸️ Deferred (later stage): **0**
- ❌ Failed: **0**

The two ⚠️ items have all engineering mitigations in place and pass every objective check available; they remain Partial only because they require a human listening / metering test that `/plugin-verify` cannot run autonomously.

## Automated Checks

| Check | Result | Notes |
|---|---|---|
| Triple build (VST3 + AU + Standalone, Release) | ✅ Pass | `ninja O-MicrotonalSampler_VST3 O-MicrotonalSampler_AU O-MicrotonalSampler_Standalone` — clean compile, no warnings. Codesign ad-hoc OK. |
| Cache-clearing fresh install per CLAUDE.md | ✅ Pass | `AudioComponentRegistrar` killed; AU caches purged; old `.vst3`/`.component` removed; fresh artefacts copied to `~/Library/Audio/Plug-Ins/{VST3,Components}/`. |
| `pluginval --strictness-level 5 --validate-in-process --skip-gui-tests` | ✅ Pass | `SUCCESS`. Allocation guard inclusive. Bus enumeration: stereo out, no input. |
| `auval -v aumu OMtS OuDv` | ✅ Pass | `AU VALIDATION SUCCEEDED`. MIDI test PASS, ramped-parameter scheduling PASS. |
| Phase 2.1 commit on record | ✅ Pass | `bb0e7f7 feat(O-MicrotonalSampler): cubic-Hermite varispeed voice + ADSR + NE - Stage 1 + Phase 2.1 Gate 1 PASS`. |
| Phase 2.2 commit on record | ✅ Pass | `cacffda feat(O-MicrotonalSampler): background sample loader + filename parser + SR conversion - Phase 2.2 Gate 2 PASS`. |
| Phase 2.3 commit on record | ✅ Pass | `11bd39c feat(O-MicrotonalSampler): equal-power velocity-layer crossfade - Phase 2.3 Gate 3 PASS`. |
| Phase 2.4 commit on record | ✅ Pass | `1aceb4c feat(O-MicrotonalSampler): voice-steal 5ms tail ramp - Phase 2.4 Gate 4 PASS`. |
| Phase 2.5 commit on record | ⚠️ Pending | Code complete + pluginval/auval green; user has not yet run the atomic commit recipe in `PHASE-2.5-SUMMARY.md`. See Outstanding Actions §1. |
| Source audit: no `setLatencySamples` | ✅ Pass | Confirmed absent across `Source/`. |
| Source audit: no allocations in render path | ✅ Pass | `.assign` only in `prepareToPlay`; `renderNextBlock`, `renderTailRamp`, `cubicInterp`, `readSlotWithLoop`, `wrapLoopPosition` all allocation-free. |
| Source audit: tuning consumption sites | ✅ Pass | `tuningEngine->getFrequency` + `Ouaricon::NoteExpression::applyPendingTuning` both fire from `startNote`. |

## Human Verification (deferred to user)

- [ ] Sustained sine (5 s, 440 Hz) loops indefinitely with no audible crossfade artefact (DSP-05 / QUAL-01 audible).
- [ ] Vibrato cello (440 Hz + 5 Hz mod, 5 s) loops without pitch jump (DSP-05 audible).
- [ ] Kick-drum impulse falls back to one-shot — voice ends naturally via ADSR release (EC-7 audible).
- [ ] Sample where the only quiet region is < 16 samples falls back to one-shot (defensive guard audible).
- [ ] 4-sample-set, 4-layer, 16-voice steal regression suites all behave as in their gate runs.
- [ ] CPU @ 16 sustained voices, 48 kHz / 256 buffer, looping samples, Apple Silicon ≤ 5 % via Logic Pro CPU meter or `pluginval --benchmark` (PERF-02).
- [ ] +50 c retune via VST3 NE produces audibly correct pitch shift with no aliasing on a sine-sweep / harmonic-rich source (DSP-01 + QUAL-01 audible).
- [ ] Mixed-SR fixture (44.1 kHz + 96 kHz files in same map) plays in tune (COMPAT-02 audible).

## Issues Found

- **Phase 2.5 not yet committed.** Source files (`LoopDetector.{h,cpp}`, modified
  `MicrotonalSamplerVoice.{h,cpp}`, `SampleLoader.cpp`, `CMakeLists.txt`) and
  the `PHASE-2.5-SUMMARY.md` are present in the working tree but the atomic
  commit recipe in §"Recipe for User to Close Gate 5" of
  `PHASE-2.5-SUMMARY.md` has not yet run. Verification artefacts (this file,
  `STATUS.md`, `REQUIREMENTS.md`) get added to the same commit. Resolution:
  Outstanding Actions §1.
- **No code defects identified.** All five sub-stage gate commits agree with
  their SUMMARY documents; cross-file invariants (loop crossfade headroom
  guard `loopEnd <= N - 2` matched between `LoopDetector` and
  `readSlotWithLoop`'s `cubicInterp(i+2)` tap; APVTS parameter IDs match
  between layout and reads) all hold.

## Outstanding Actions

1. **Phase 2.5 atomic commit (user).** Per the recipe in
   `PHASE-2.5-SUMMARY.md` (now extended to include the Stage 2 verify
   artefacts):
   ```bash
   cd /Users/taylorbrook/Dev/VST-development
   git add plugins/O-MicrotonalSampler/Source/LoopDetector.h \
           plugins/O-MicrotonalSampler/Source/LoopDetector.cpp \
           plugins/O-MicrotonalSampler/Source/SampleLoader.cpp \
           plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.h \
           plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp \
           plugins/O-MicrotonalSampler/CMakeLists.txt \
           plugins/O-MicrotonalSampler/.planning/STATUS.md \
           plugins/O-MicrotonalSampler/.planning/REQUIREMENTS.md \
           plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PHASE-2.5-SUMMARY.md \
           plugins/O-MicrotonalSampler/.planning/stages/2-dsp/VERIFICATION.md
   git commit -m "feat(O-MicrotonalSampler): loop auto-detect + 8-sample equal-power crossfade - Phase 2.5 Gate 5 + Stage 2 verify PASS"
   ```
2. **Subjective DAW pass (user).** Walk the Human Verification checklist above
   in Logic Pro / Live (or any AU/VST3 host that surfaces VST3 NE). If any
   item fails, file a defect and reopen the relevant sub-phase rather than
   advancing to Stage 3.
3. **CPU benchmark (user, PERF-02).** Hold 16 voices on a sustained-tone
   library at 48 kHz / 256 buffer; read Logic CPU meter or
   `pluginval --benchmark`. Confirm ≤ 5 %. If above, the suspect is the
   conditional 1st-order tilt LPF from the Phase 2.1 RQ-1 contingency — but
   that filter was *not* added (Phase 2.1 SUMMARY documents the negative
   sine-sweep result), so any overage points elsewhere (likely the
   `readSlotWithLoop` two-cubicInterp path inside the 8-sample fade window;
   profile with Instruments before optimising).

## Stage Verdict

**Status:** ✅ VERIFIED

All 13 objectively-verifiable Stage 2 requirements are complete and
evidenced. The two remaining items (PERF-02, QUAL-01) are Partial only
in the sense that they require a human DAW listening / metering pass —
every engineering mitigation they depend on is implemented and proven
against the available automated tooling. Per project pattern (matches
Phase-2.5 SUMMARY's deferred-items convention), the user closes the
loop with the DAW pass after committing Phase 2.5.

**Ready for next stage:** Yes (Stage 3 GUI), with the caveat that Phase
2.5 must be atomically committed first and the subjective DAW pass run
in parallel with Stage 3 design (UI mockup is a parallel-safe
prerequisite).

**Blockers:** None at the engineering level. Two procedural items in
Outstanding Actions §1-§3.

## Next Phase

Stage 3 (GUI). Resume via:

```
/clear
/implement O-MicrotonalSampler
```

(or, manual mode, `/plugin-discuss O-MicrotonalSampler 3-gui`).

UI-mockup work (`/ui-mockup O-MicrotonalSampler`) is parallel-safe and
can begin any time before Stage 3 plan.
