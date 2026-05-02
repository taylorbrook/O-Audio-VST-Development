# O-Bassoon Stage 2 / Phase 2.1 — Execution Summary

**Phase:** 2.1 — Core Modal Voice + First Audio
**Completed:** 2026-04-27
**Templates:** O-Wind / O-Lyrica (per-voice prepareToPlay pattern), O-Formant (NaN-guard biquad pattern, reimplemented as pole-only)
**Agent:** dsp-agent + orchestrator (build / install / auval / pluginval / reference download)

## What Was Built

### New Source Files

- `Source/ModeBank.{h,cpp}` — 16-mode parallel pole-only resonator bank
  - `static constexpr` `PARTIAL_RATIOS = {1, 2, ..., 16}` (Phase 2.1 placeholder integer harmonics; Phase 2.2 replaces)
  - `static constexpr` `BASE_T60 = {2.5, 2.2, 2.0, 1.8, 1.6, 1.4, 1.2, 1.0, 0.8, 0.7, 0.6, 0.5, 0.4, 0.35, 0.30, 0.25}` seconds
  - Inner `ModeBiquad`: Direct-Form I pole-only (`y0 = b0·x − a1·y1 − a2·y2`) with `std::isfinite` guard (resets `y1=y2=0`, returns 0 if non-finite)
  - `setFundamental(f0)`: per-mode coefficients `theta = 2π·f_k/fs; tau = T60_k/6.91; R = exp(-1/(tau·fs)); b0 = (1-R)·amp; a1 = -2R·cos(theta); a2 = R²`
  - Nyquist mute: `f_k > 0.45·fs` OR `f_k <= 0` zeroes `b0=a1=a2`
  - `processSample` sums all 16 modes and scales by `1/NUM_MODES` (D2 placeholder; Phase 2.3 replaces with output_gain APVTS)
  - `setTone(float)` declared as inline no-op stub (wired live in Phase 2.2)

- `Source/Exciter.{h,cpp}` — 5 ms half-sine × exp impulse, peak-normalised, pre-baked at `prepare()`
  - `MAX_ONSET_SAMPLES = 1024` (5 ms @ 96 kHz = 480; 1024 leaves headroom)
  - In-class `std::array<float, 1024> onsetBuffer{}` (per-voice, populated once)
  - `getNextSample()` returns `onsetBuffer[onsetIdx++]` while active; else `active = false; return 0`
  - All paths `noexcept`; no allocation, no `vector`/`make_unique`/`Array::resize`

### Modified Source Files

- `Source/BassoonVoice.h` — extended Stage 1 stub
  - Added `#include "ModeBank.h"`, `#include "Exciter.h"`
  - Added public **non-virtual** `void prepareToPlay(double, int)` (D1: `juce::SynthesiserVoice` has no virtual prepare hook; PluginProcessor iterates voices)
  - Added private `static constexpr float PITCH_BEND_RANGE_SEMITONES = 2.0f;`
  - Added private members: `ModeBank modeBank; Exciter exciter; juce::ADSR adsr; int pitchWheelValue = 8192; float pitchBendSemitones = 0.0f; float currentFrequencyBase = 0.0f;`
  - Stage 1 setters (`setAPVTS`, `setTuningEngine`, `setPendingTuningSource`) and pointer members untouched

- `Source/BassoonVoice.cpp` — silent-stub bodies replaced with first-audio implementation
  - `prepareToPlay`: `setCurrentPlaybackSampleRate(sr); modeBank.prepare(sr); exciter.prepare(sr); adsr.setSampleRate(sr); adsr.setParameters({0.010, 0, 1.0, 0.200});` — `setSampleRate` BEFORE `setParameters` (JUCE 8 ADSR contract, OQ#1)
  - `startNote`: capture pitch wheel, compute pitchBendSemitones, `currentFrequencyBase = (float)juce::MidiMessage::getMidiNoteInHertz(midiNote)` (plain MIDI — no TuningEngine, locked Q2), `fBent = base · 2^(bend/12); modeBank.setFundamental(fBent); exciter.start(); adsr.noteOn();`
  - `stopNote`: `if (allowTailOff) adsr.noteOff();` else full state reset (`clearCurrentNote(); adsr.reset(); modeBank.reset(); exciter.reset(); currentFrequencyBase = 0;`)
  - `pitchWheelMoved`: recompute bend; guard `if (currentFrequencyBase > 0.0f)` before `modeBank.setFundamental(fBent)`
  - `controllerMoved`: empty body (Phase 2.3 wires CC2 → breath)
  - `renderNextBlock`: early return if `!adsr.isActive()`; per-sample loop `(excite → resonate → envelope) → addSample to all channels at startSample+i`; mid-loop `!adsr.isActive()` early-exit with full state reset

- `Source/PluginProcessor.cpp` — `prepareToPlay` only
  - Renamed param `int /*samplesPerBlock*/` → `int samplesPerBlock`
  - Added per-voice prepare loop AFTER `synthesiser.setCurrentPlaybackSampleRate`:
    ```cpp
    for (int v = 0; v < synthesiser.getNumVoices(); ++v)
        if (auto* bv = dynamic_cast<BassoonVoice*>(synthesiser.getVoice(v)))
            bv->prepareToPlay(sampleRate, samplesPerBlock);
    ```
  - `processBlock`, `releaseResources`, `isBusesLayoutSupported`, parameter layout, voice/sound construction, NE drain ordering at line 170 — ALL untouched

- `CMakeLists.txt` — `target_sources` block only
  - Added `Source/ModeBank.h`, `Source/ModeBank.cpp`, `Source/Exciter.h`, `Source/Exciter.cpp` after `Source/BassoonVoice.cpp`
  - `juce_add_plugin` flags, `ouaricon_add_module(O-Bassoon note-expression)`, `target_link_libraries`, `target_compile_definitions` — ALL untouched

### Reference Recordings (Phase 2.1 archive for Phase 2.2 listening loop)

- `research/reference-recordings/bassoon-c3-sustain-v1.wav` — VSCO-2-CE PSBassoon_C3_v1_1, 16-bit / 44.1 kHz stereo PCM, ~8.7 s sustain (CC0)
- `research/reference-recordings/bassoon-c3-sustain-v2.wav` — VSCO-2-CE PSBassoon_C3_v2_1, 16-bit / 44.1 kHz stereo PCM, ~8.7 s sustain (CC0)
- `research/reference-recordings/LICENSE.md` — VSCO-2-CE provenance + CC0 dedication
- `research/reference-recordings/README.md` — source, octave-convention caveat (D4), audition checklist, Phase 2.1 spectrum-baseline procedure

`afinfo` confirmed both files are valid WAV; tuner-confirmation of fundamental Hz (D4 octave check) is part of the Logic-AU manual smoke step pending user verification.

## Build & Validation Results

- **CMake configure:** success (note-expression JUCE-NE-PATCH markers verified, ModeBank/Exciter sources picked up automatically via globbed reconfigure)
- **`ninja O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone`:** SUCCESS (0 errors; pre-existing note-expression module warnings carried from `funknown.h` and `Controller` dtor only)
- **Install:** AU cache cleared, VST3 + AU + Standalone installed fresh to `~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3` and `~/Library/Audio/Plug-Ins/Components/O-Bassoon-dev.component`
- **`auval -v aumu OBsn OuDv`:** **AU VALIDATION SUCCEEDED** — all render rates (11k/22k/44.1k/48k/96k/192k), parameter scheduling, MIDI tests PASS
- **`pluginval --strictness-level 5 --validate ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3`:** **SUCCESS** (exit 0; output-only bus confirmed: 0 in / 2 out)

## Pre-commit RT-safety Scan

```
grep -nE '\bnew\b|make_unique|make_shared|push_back|resize|malloc' \
    Source/ModeBank.{h,cpp} Source/Exciter.{h,cpp} Source/BassoonVoice.cpp
```
→ **ZERO functional matches**

```
grep -n "parameters" Source/BassoonVoice.cpp
```
→ **ZERO matches** (no APVTS reads in voice — locked Q2)

```
grep -nE "tuningEngine->|pendingTuningSource->" Source/BassoonVoice.cpp
```
→ Only a doc comment at line 45 (`// Phase 2.4 will replace this with tuningEngine->getFrequency() per-voice.`); zero functional dereferences.

```
grep -n "setLatencySamples" Source/PluginProcessor.cpp
```
→ Only the Stage 1 comment at line 136; zero functional calls.

(Constructor `new BassoonVoice()` in `PluginProcessor.cpp:120` is permitted — non-audio-thread, runs once at construction.)

## Stage 1 Invariants Preserved (Regression Check)

- ✅ All 10 APVTS parameters present (IDs, ranges, defaults unchanged)
- ✅ `juce::Synthesiser` has 16 voices (`for (int i = 0; i < 16; ++i)` in ctor untouched)
- ✅ `vst3Extensions.drainAndUpdate()` called BEFORE `synthesiser.renderNextBlock` (PluginProcessor.cpp:170 → :174)
- ✅ `BassoonSound::appliesToNote/Channel` unchanged
- ✅ `juce_generate_juce_header` AFTER `target_link_libraries`
- ✅ `IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`, `PLUGIN_CODE OBsn`, `NEEDS_WEBVIEW2 TRUE`, `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` flags unchanged
- ✅ Output-only bus (`isBusesLayoutSupported` rejects input bus) — confirmed by pluginval reading 0 input ch / 2 output ch

## Gate 1 PASS Bar — Status

| # | Gate 1 Item | Pass criterion | Status |
|---|-------------|----------------|--------|
| 1 | Sustained tone at correct pitch (±2 cents on A4=440 Hz) | Tuner | **PENDING** (manual Logic-AU verification — orchestrator cannot drive Logic UI) |
| 2 | No clicks at note-on/note-off | Subjective listen | **PENDING** (manual Logic-AU verification) |
| 3 | No NaN/inf in render | SPAN; no DC spike or runaway | **PENDING** (manual Logic + SPAN verification) |
| 4 | > 10 s sustain without amplitude drift | DAW level meter at 12 s | **PENDING** (manual Logic-AU verification) |
| 5 | 1-voice CPU < 5 % @ 48 k / 256 | Logic Performance Meter "Process" bar | **PENDING** (manual Logic verification) |
| 6 | Plays C1–C6 without instability | DAW scale sweep | **PENDING** (manual Logic-AU verification) |
| 7 | `auval -v aumu OBsn OuDv` SUCCESS | exit 0 | **✅ PASS** (orchestrator-verified) |
| 8 | `pluginval --strictness-level 5` SUCCESS | exit 0 | **✅ PASS** (orchestrator-verified) |
| 9 | Logic AU manual smoke (hold C3, sweep C1→C6) | DAW recording | **PENDING** (manual Logic-AU verification) |
| 10 | DAW spectrum baseline capture (SPAN PNG) | PNG committed at `research/reference-recordings/phase-2.1-baseline-c3-spectrum.png` | **PENDING** (manual Logic + SPAN capture) |

**Auto-verified subset:** items 7, 8 ✅. **Manual subset:** items 1–6, 9, 10 deferred to user via `/plugin-verify O-Bassoon 2-dsp` (which can document the manual-verification evidence) or direct Logic Pro audit.

The build succeeds and both static-validators (auval / pluginval-5) confirm the plugin is structurally sound, deterministic, and bus-correct. The audible verification (pitch accuracy, click detection, sustain stability, CPU, scale sweep, SPAN baseline) requires the user to load O-Bassoon in Logic Pro AU and follow the procedure in PLAN.md Task 8.

## Deviations from Plan

**Minor (non-functional):** the dsp-agent added `static_cast<size_t>` index casts on `std::array` accesses (Exciter.cpp + ModeBank.cpp) to silence signed→unsigned conversion warnings. Zero functional impact, RT-safe. Documented in agent's own deviation report.

**Process (orchestration):** Tasks 1–6 (code) + Task 7 (reference recording download) + automatable subset of Task 8 (build / install / auval / pluginval / RT-safety grep) completed by the orchestrator in this execute pass. Task 8 manual subset (Logic-AU listening + tuner + CPU + SPAN baseline) and Task 9 (atomic commit) await user verification — per CLAUDE.md ("only commit when explicitly asked"), the orchestrator does not auto-commit.

## Verifies Requirements

- **AUDIO-CORE / first-audio path** (BRIEF + ROADMAP Phase 2.1 goal): ✅ static path verified — modal voice instantiates, ADSR/exciter/mode-bank prepare without error, pluginval-5 confirms processBlock contract under fuzz parameters and state restore. Audible-tone verification deferred to Logic-AU manual smoke.
- **DSP-07** (no O-Reed dependency): ✅ confirmed via grep — no `O-Reed` / `OReed` references in any new or modified source file.
- **COMPAT-01** (pluginval strictness ≥5 pass): ✅ pluginval strictness 5 returned SUCCESS.

## Files Created / Modified

| Type | Path |
|------|------|
| NEW  | `plugins/O-Bassoon/Source/ModeBank.h` |
| NEW  | `plugins/O-Bassoon/Source/ModeBank.cpp` |
| NEW  | `plugins/O-Bassoon/Source/Exciter.h` |
| NEW  | `plugins/O-Bassoon/Source/Exciter.cpp` |
| MOD  | `plugins/O-Bassoon/Source/BassoonVoice.h` |
| MOD  | `plugins/O-Bassoon/Source/BassoonVoice.cpp` |
| MOD  | `plugins/O-Bassoon/Source/PluginProcessor.cpp` (`prepareToPlay` only) |
| MOD  | `plugins/O-Bassoon/CMakeLists.txt` (`target_sources` block only) |
| NEW  | `plugins/O-Bassoon/research/reference-recordings/bassoon-c3-sustain-v1.wav` |
| NEW  | `plugins/O-Bassoon/research/reference-recordings/bassoon-c3-sustain-v2.wav` |
| NEW  | `plugins/O-Bassoon/research/reference-recordings/LICENSE.md` |
| NEW  | `plugins/O-Bassoon/research/reference-recordings/README.md` |
| NEW  | `plugins/O-Bassoon/.planning/stages/2-dsp/SUMMARY.md` (this file) |
| MOD  | `plugins/O-Bassoon/.planning/STATUS.md` |

**Deferred to user verification step:** `phase-2.1-baseline-c3-spectrum.png` (Gate 1 item 10) and atomic commit (Task 9, on Gate 1 PASS green).

## Next

Verify phase: `/plugin-verify O-Bassoon 2-dsp` (consumes this SUMMARY + PLAN, and prompts for Gate 1 manual-verification evidence — items 1–6, 9, 10 — or accepts a pre-captured Logic-AU smoke + SPAN screenshot).

---

# O-Bassoon Stage 2 / Phase 2.2 — Execution Summary

**Phase:** 2.2 — Bassoon Spectral Tuning + Tone Control
**Executed:** 2026-04-27
**Inputs:** PLAN-rev-2 (9 tasks, single Wave) + RESEARCH-rev-2 §3 implementation skeletons (lifted verbatim) + ARCHITECTURE.md §"Bassoon Partial Table" / §"Tone / Brightness Control"
**Working tree start:** Phase 2.1 atomic commit `d1b3370` on `main`

## What Was Built

### Modified Source Files

| Op  | Path | Change |
|-----|------|--------|
| MOD | `plugins/O-Bassoon/Source/ModeBank.h`         | Added `FORMANT_F1` (475 Hz) + `FORMANT_BW` (200 Hz); replaced placeholder integer `PARTIAL_RATIOS` with bassoon-tuned 16-element near-integer ratio table; replaced inline `setTone` no-op with real `setTone` + `applyToneChange` declarations + private `computeModeAmplitude`; extended `ModeBiquad` with cached `cosTheta` + `amp`; added `currentTone` member (default 0.5f). |
| MOD | `plugins/O-Bassoon/Source/ModeBank.cpp`       | Rewrote `setFundamental` to compute formant-Gaussian × 1/k roll-off amplitude per mode, cache `cosTheta`/`amp`, apply `mix(0.3, 1.5, tone)` T60 scale to upper modes (k > 4) only; mute-path zeros cached state too; new `setTone` (jlimit clamp + currentTone cache); new `applyToneChange` (tone-scaled T60 recompute for k = 5..15, skip muted via `m.amp == 0.0f`); new private static `computeModeAmplitude`; relaxed processSample headroom scaler `1/16 → 1/8` (+6 dB). |
| MOD | `plugins/O-Bassoon/Source/BassoonVoice.h`     | Added public `void setTone (float tone01) noexcept;` declaration after `renderNextBlock`. |
| MOD | `plugins/O-Bassoon/Source/BassoonVoice.cpp`   | Added thin forwarder body — calls `modeBank.setTone` then `modeBank.applyToneChange`. No throttling here (gate is at processor dispatch site). |
| MOD | `plugins/O-Bassoon/Source/PluginProcessor.h`  | Added private `juce::SmoothedValue<float, ValueSmoothingTypes::Linear> toneSmoother;` + `float lastDispatchedTone = -1.0f;` (sentinel forces first dispatch). |
| MOD | `plugins/O-Bassoon/Source/PluginProcessor.cpp`| `prepareToPlay`: `toneSmoother.reset(sampleRate, 0.050)` + reset sentinel. `processBlock`: insert tone advance + voice dispatch BEFORE `vst3Extensions.drainAndUpdate()`; throttle gate `std::abs(toneSmoothed - lastDispatchedTone) > 0.001f`; switched `synthesiser.renderNextBlock(...)` to use cached `numSamples` local. |
| MOD | `plugins/O-Bassoon/.planning/research/ARCHITECTURE.md` | Appended `## rev-note: Phase 2.2 As-Shipped` per RESEARCH-rev-2 OQ#10-rev-2 default (append-rev-note, partial-table values shipped verbatim). Documents the one in-cycle deviation (1/16 → 1/8 scaler). |

### No New Source Files
No new files in Phase 2.2; all edits are MODIFY of existing rev-1 files. CMakeLists.txt is unchanged (no new translation units).

## Build + Static-Check Gate Status

| # | Gate | Status |
|---|------|--------|
| 1 | RT-safety grep — zero new hot-path matches in ModeBank/BassoonVoice; PluginProcessor matches are pre-existing setup-time only (createParameterLayout, ctor `new BassoonVoice/BassoonSound`, factories) | ✅ PASS |
| 2 | NE drain ordering preserved — `drainAndUpdate()` precedes `renderNextBlock()` in processBlock; new tone-dispatch block precedes BOTH | ✅ PASS |
| 3 | Mode-index zero-indexed convention — single match `for (int k = 5; k < NUM_MODES` in `applyToneChange` | ✅ PASS |
| 4 | Headroom scaler relaxation locked — `1.0f / 8.0f` present in `processSample`; `1.0f / NUM_MODES` and `1.0f / 16.0f` zero matches | ✅ PASS |
| 5 | Throttle epsilon locked — `0.001f` present at the dispatch comparator (PluginProcessor.cpp:189) | ✅ PASS |
| 6 | DSP-07 (no O-Reed dependency) regress — zero matches | ✅ PASS |
| 7 | AU validation — `auval -v aumu OBsn OuDv` SUCCEEDED | ✅ PASS |
| 8 | pluginval --strictness 5 — exit 0 SUCCESS | ✅ PASS |
| — | Build clean (VST3 + AU + Standalone, ad-hoc signed) — zero compile warnings | ✅ PASS |
| — | Install fresh per CLAUDE.md cache-clearing protocol | ✅ DONE |

## Manual Gate 2 Bar (handed to user — Task 8 of PLAN-rev-2)

10-item Logic-AU verification checklist. **Items 2, 3, 4, 8, 9 must PASS clean; items 1, 5, 6, 7 may be PARTIAL with documented deviation per CONTEXT-rev-2 Q6-rev-2 inline iteration.**

1. **Pre-flight: reference WAV pitch audition** — load `bassoon-c3-sustain-v1.wav` on an audio track, insert Logic Tuner. PASS bar: tuner reads C3 (130.8 Hz ± 30 cents). If v1 fails, switch to v2; document in VERIFICATION if both fail.
2. **A/B listening — held C3 timbre** — insert O-Bassoon-dev (AU), `tone = 0.5`, hold C3, loop reference WAV adjacent. PASS bar: ear judgment "yes, that's bassoon-like". *Inline rev-3 trigger if FAIL.*
3. **Spectrum overlay** — insert Logic Channel EQ post-O-Bassoon, set Pre-EQ Analyzer mode, all bands disabled. Hold C3. PASS bar: visible peak in 400–600 Hz region. Save screenshot to `plugins/O-Bassoon/research/reference-recordings/phase-2.2-as-shipped-c3-spectrum.png`.
4. **Tone-sweep cleanliness** — hold C3, sweep `tone` 0.0 → 1.0 → 0.0 over ~3 s. PASS bar: no zipper / clicks / pops / NaN; audible character change.
5. **Tone descriptor verification** — `tone = 0`/C3 → "woody, dark"; `tone = 1`/C3 → "brighter, present". PASS bar: clearly audible difference; both ends musical.
6. **8-voice CPU early signal** — Logic CPU meter / Process bar. Hold 8-note chord **C3 + E3 + G3 + Bb3 + C4 + E4 + G4 + Bb4**, sustain ≥ 3 s. PASS bar: < 20 %.
7. **1-voice CPU regress** — hold single C3. PASS bar: Process bar < 5 %.
8. **Pitch range C1–C6 regress** — sweep MIDI C1 → C6, sustained ~1 s each. PASS bar: all notes track pitch, no glitches/NaN/muting. C5+ thinning expected (Nyquist-muted upper modes).
9. **Long-tone stability** — hold C3 ≥ 10 s. PASS bar: no dropouts, no NaN, no exponential drift, no DC bias.
10. **Write VERIFICATION-rev-2** mapping items 1–9 to PASS / PARTIAL / DEVIATION + Phase 2.1 invariant regression confirmation.

**Iteration ceiling:** rev-3 (CONTEXT-rev-2 Q6-rev-2). After rev-3, ship and document v1.0 gap as v1.1 partial-table refinement candidate ("bassoon-inspired" framing per ARCHITECTURE Risk #2 Fallback 2).

## Files for Atomic Commit (Task 9 of PLAN-rev-2)

```
M plugins/O-Bassoon/Source/ModeBank.h
M plugins/O-Bassoon/Source/ModeBank.cpp
M plugins/O-Bassoon/Source/BassoonVoice.h
M plugins/O-Bassoon/Source/BassoonVoice.cpp
M plugins/O-Bassoon/Source/PluginProcessor.h
M plugins/O-Bassoon/Source/PluginProcessor.cpp
M plugins/O-Bassoon/.planning/research/ARCHITECTURE.md
M plugins/O-Bassoon/.planning/STATUS.md
M plugins/O-Bassoon/.planning/stages/2-dsp/CONTEXT.md     (rev-2 — discuss-phase)
M plugins/O-Bassoon/.planning/stages/2-dsp/RESEARCH.md    (rev-2 — research-phase)
M plugins/O-Bassoon/.planning/stages/2-dsp/PLAN.md        (rev-2 — plan-phase)
M plugins/O-Bassoon/.planning/stages/2-dsp/SUMMARY.md     (this addendum)
M plugins/O-Bassoon/.planning/stages/2-dsp/VERIFICATION.md (rev-2 — written at verify-phase)
?? plugins/O-Bassoon/research/reference-recordings/phase-2.2-as-shipped-c3-spectrum.png (captured at item 3)
```

**Locked atomic-commit subject** (CONTEXT-rev-2 Q9-rev-2): `feat(O-Bassoon): Phase 2.2 spectral tuning + tone control - Gate 2 PASS`. Commit lands at verify-phase only on Gate 2 PASS green.

## Deferred to User Verification Step

- Logic-AU manual checklist items 1–9 (above) — user runs in their DAW
- `phase-2.2-as-shipped-c3-spectrum.png` capture (item 3)
- VERIFICATION-rev-2.md write (item 10)
- Atomic commit (verify-phase, on Gate 2 PASS green)

## Next

Verify phase: `/plugin-verify O-Bassoon 2-dsp` — consumes this SUMMARY + PLAN-rev-2, prompts for the 10-item Gate 2 evidence in Logic-AU, writes VERIFICATION-rev-2, and lands the atomic commit on green.

---

# O-Bassoon Stage 2 / Phase 2.3 — Execution Summary (rev-3, 2026-04-28)

**Phase:** 2.3 — Per-Note Expression: Envelope, Breath, Vibrato, Output Gain
**Execute-phase completed:** 2026-04-28
**Templates:** O-Wind FluteSynthVoice (random vibrato phase per startNote, CC2 normalisation site), O-Bowed BowNoiseGenerator (deterministic per-voice noise seed `voiceIndex × 31337`)
**Agent:** orchestrator-direct (PLAN-rev-3 Tasks 1-9 executed inline; lifted RESEARCH-rev-3 §3 implementation skeletons verbatim)

## What Was Built

### New Source Files

- `Source/Vibrato.{h,cpp}` (NEW) — per-voice sine LFO + onset envelope
  - Public API: `prepare(double) noexcept`, `reset() noexcept`, `setRateHz(float) noexcept`, `setDepthCents(float) noexcept`, `setOnsetMs(float) noexcept`, `getCurrentCents() noexcept`
  - Random initial phase per `startNote`: `phase = juce::Random::getSystemRandom().nextFloat() × twoPi` (locked OQ#9-rev-3, O-Wind `FluteSynthVoice.cpp:114-116` precedent — overrides CONTEXT-rev-3 default instant-zero per D2-rev-3)
  - Variable-duration onset `juce::SmoothedValue<float, Linear>` ramp 0→1 over `vibrato_onset` ms (0 ms = instant target, D5-rev-3 confirmed safe)
  - Output: `depthCents × onsetGain × std::sin(phase)`; advance phase by `2π × rateHz / sampleRate` per sample
  - No smoothing on rate/depth — LFO modulation masks zipper (locked CONTEXT-rev-3 line 526)
  - Zero allocation in any method; `noexcept` on every public method

- `Source/NoiseExciter.{h,cpp}` (NEW) — per-voice continuous filtered-noise excitation (architectural pivot)
  - Public API: `prepare(double, int voiceIndex) noexcept`, `reset() noexcept`, `getNextSample(float breathScaled) noexcept`
  - Deterministic per-voice seed: `rng.setSeed(static_cast<juce::int64>(voiceIndex) * 31337)` (locked OQ#3-rev-3, O-Bowed `BowNoiseGenerator.h:23` precedent — overrides CONTEXT-rev-3 default `Time::currentTimeMillis() ^ voiceIndex` per D3-rev-3)
  - 1-pole low-pass: `lpCoeff = 1 - exp(-2π × 2000 / sampleRate)`; cutoff **2 kHz**
  - Output: `lpState × BASE_NOISE_GAIN × breathScaled` where `BASE_NOISE_GAIN = 0.05f` (OQ#4-rev-3 starting point; verify-phase rev-1 ear-tunes within `[0.03f, 0.20f]` if needed)
  - Per-voice `juce::Random` instance (no shared mutable state)

### Modified Source Files

- `Source/BassoonVoice.h` — Phase 2.3 surface
  - Added includes: `Vibrato.h`, `NoiseExciter.h`
  - Added public: `setExpression(float attackMs, float releaseMs, float vibRateHz, float vibDepthCents, float vibOnsetMs, float uiBreath) noexcept`, `setVoiceIndex(int idx) noexcept`
  - Added private: `Vibrato vibrato; NoiseExciter noiseExciter; juce::SmoothedValue<float, Linear> breathSmoother {0.0f}; float lastDispatchedFrequency = 0.0f;` + 5 expression dispatch shadows (`lastApplied{AttackMs,ReleaseMs,VibRate,VibDepth,VibOnsetMs}` initialised `-1.0f`) + CC2 takeover state (`cc2EverActive = false; lastCC2SampleCount = 0; currentSampleCount = 0; lastUiBreath = 0.7f;`) + `int voiceIndex = 0;`
  - Phase 2.1/2.2 public API unchanged; `Exciter exciter;` member retained verbatim (D6-rev-3 — Phase 2.4 re-wires for attack-character morph)

- `Source/BassoonVoice.cpp` — Phase 2.3 deltas
  - `prepareToPlay`: appended `vibrato.prepare(sr); noiseExciter.prepare(sr, voiceIndex); breathSmoother.reset(sr, 0.020); breathSmoother.setCurrentAndTargetValue(0.7f);`
  - `startNote`: APVTS reads of `attack_time` + `release_time` at note-on; `adsr.setParameters({attack/1000, 0, 1.0, release/1000})`; `vibrato.reset(); noiseExciter.reset(); breathSmoother.setCurrentAndTargetValue(velocity); lastUiBreath = velocity;` (velocity-as-initial-UI-breath); CC2 state reset (`cc2EverActive = false; lastCC2SampleCount = 0;`); shadow init forces first dispatch
  - `controllerMoved`: replaced no-op stub with CC2 routing (`controllerNumber == 2` → `cc2Normalised = limit(0,1, value/127)`; `cc2EverActive = true; lastCC2SampleCount = currentSampleCount; breathSmoother.setTargetValue(lastUiBreath × cc2Normalised)`); CC2 normalisation site (locked OQ#8-rev-3, O-Wind `FluteSynthVoice.cpp:227` precedent)
  - `setExpression` (NEW): per-voice sub-param epsilon throttle (`EPS = 0.001f`); ADSR re-shape only when changed; vibrato setters epsilon-throttled; CC2-takeover gate decides whether `uiBreath` is applied (`cc2RecentlyActive = cc2EverActive && (currentSampleCount - lastCC2SampleCount) < 0.500 × sampleRate`)
  - `renderNextBlock`: per-block prologue computes `vibratoCents = vibrato.getCurrentCents()`, `vibratoMult = pow(2, c/1200)`, `pbMult = pow(2, pb/12)`, `f_final = currentFrequencyBase × vibratoMult × pbMult`; throttled `modeBank.setFundamental(f_final)` when `|Δf| > 0.1 Hz`; per-sample loop pivot — `breath = breathSmoother.getNextValue(); excitation = noiseExciter.getNextSample(breath); voice = modeBank.processSample(excitation); env = adsr.getNextSample(); sample = voice × env;` equal L+R `addSample`; `currentSampleCount += numSamples` (CC2 state machine clock); ADSR-idle exit moved to post-loop (preserves clock advance)
  - **Phase 2.1 `Exciter::getNextSample()` call dropped** from per-sample loop; `Exciter exciter;` member retained per D6-rev-3 (Phase 2.4 re-wires); `exciter.prepare(sr)` call retained in `prepareToPlay`; `exciter.start()` call retained in `startNote` (D6-rev-3 retention — `Exciter::start()` is single state assignment, zero CPU when `getNextSample` not called)

- `Source/PluginProcessor.h` — Phase 2.3 surface
  - Added private: `juce::SmoothedValue<float, Linear> outputGainSmoother {1.0f};` + 6 dispatch shadows (`lastDispatched{AttackMs, ReleaseMs, VibRate, VibDepth, VibOnsetMs, UiBreath}` initialised `-1.0f`)

- `Source/PluginProcessor.cpp` — Phase 2.3 wire-up
  - **Constructor**: per-voice `setVoiceIndex(i)` wire after voice-creation loop (Phase 2.3 noise-seed deterministic per-voice)
  - `prepareToPlay`: `outputGainSmoother.reset(sampleRate, 0.030); outputGainSmoother.setCurrentAndTargetValue(1.0f);` + reset all 6 processor-scope shadows to `-1.0f`
  - `processBlock` ordering (locked OQ#6-rev-3): `ScopedNoDenormals → buffer.clear() → tone-dispatch (Phase 2.2) → expression-dispatch (Phase 2.3 NEW) → drainAndUpdate (NE) → renderNextBlock → output_gain applyGainRamp (Phase 2.3 NEW)`
  - Expression dispatch: 6 APVTS reads (`attack_time`, `release_time`, `vibrato_rate`, `vibrato_depth`, `vibrato_onset`, `breath`); single aggregated `bv->setExpression(...)` per voice ONLY when any sub-param changes > `0.001f` (saves ~96 virtual hops/block in quiescent state)
  - `output_gain` declick: `gainStart = outputGainSmoother.getCurrentValue(); outputGainSmoother.setTargetValue(decibelsToGain(outDb)); gainEnd = outputGainSmoother.skip(jmax(0, numSamples)); buffer.applyGainRamp(0, numSamples, gainStart, gainEnd);` (canonical declick-safe idiom — locked OQ#1-rev-3)

- `CMakeLists.txt` — added 4 entries to `target_sources` (`Vibrato.h`, `Vibrato.cpp`, `NoiseExciter.h`, `NoiseExciter.cpp`); existing Stage 1 build flags + `juce_generate_juce_header` ordering + `ouaricon_add_module` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` untouched; commented `Exciter.{h,cpp}` lines as Phase 2.4 retention

- `.planning/research/ARCHITECTURE.md` — appended Phase 2.3 rev-3 as-shipped note (architectural pivot, continuous-noise excitation spec, breath/dynamics CC2-takeover state machine, vibrato compose chain `f_final = base × pow(2, c/1200) × pow(2, pb/12)`, throttled-epsilon expression dispatch, post-summation `output_gain` declick, ADSR cadence, ordering invariant)

## Build + Install

- `cmake --build build --target O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone`: **SUCCESS**, clean (12/12 targets built; zero warnings on Phase 2.3 source files; no `-Wunused-private-field` on retained `Exciter exciter;` member — D6-rev-3 mitigation not needed)
- AU cache cleared (`AudioComponentRegistrar` killed; `~/Library/Caches/AudioUnitCache/` + `com.apple.audiounits.cache` removed)
- VST3 + AU installed fresh to `~/Library/Audio/Plug-Ins/{VST3, Components}/`

## Auto-Verified Static Checks (10/10 PASS)

| # | Gate | Result |
|---|------|--------|
| 1 | RT-safety grep across 8 touched source files | ✅ Zero render-path matches (all hits are construction-time `make_unique` for param layout + factory `new` — not in render path) |
| 2 | Ordering: tone-dispatch → expression-dispatch → NE-drain → renderNextBlock → applyGainRamp | ✅ PluginProcessor.cpp lines 203, 236, 249, 252, 262 — correct sequence |
| 3 | `bv->setExpression` dispatch site present | ✅ ONE match at PluginProcessor.cpp:236 (inside `if (anyChanged)` voice loop) |
| 4 | `applyGainRamp(0, numSamples,` form locked, AFTER `renderNextBlock` | ✅ PluginProcessor.cpp:262 (after line 252 renderNextBlock) |
| 5 | `modeBank.setFundamental` cadence in BassoonVoice.cpp (≥ 2 hits) | ✅ 3 matches: line 62 (startNote), line 119 (pitchWheelMoved carry-forward), line 192 (renderNextBlock per-block compose — Phase 2.3 NEW) |
| 6 | Phase 2.2 1/8 scaler retention; 1/16 absent | ✅ ModeBank.cpp:114 `1.0f / 8.0f`; zero `1.0f / NUM_MODES` matches |
| 7 | Throttle epsilon `0.001f` count (≥ 7 in PluginProcessor; ≥ 1 in BassoonVoice) | ✅ 10 matches in PluginProcessor.cpp (3 param ranges + 1 Phase 2.2 tone + 6 Phase 2.3 expression dispatch); BassoonVoice.cpp:148 `EPS = 0.001f` |
| 8 | DSP-07 (no O-Reed dependency) | ✅ Zero matches in `plugins/O-Bassoon/Source` + `CMakeLists.txt` |
| 9 | `auval -v aumu OBsn OuDv` | ✅ AU VALIDATION SUCCEEDED |
| 10 | `pluginval --strictness-level 5 --validate ~/Library/Audio/Plug-Ins/VST3/O-Bassoon-dev.vst3` | ✅ exit 0; SUCCESS; 0 in / 2 out bus confirmed |

## Discrepancies Found / Resolved at Execute-Phase

- **D-exec-1 (RESEARCH-rev-3 plan typo):** RESEARCH-rev-3 §3 BassoonVoice.cpp skeleton (and PLAN-rev-3 Task 4(e)) shows `modeBank.setFundamental(f_final, getSampleRate())` with two args. Existing Phase 2.1 `ModeBank::setFundamental(float f0)` API takes ONE arg (sample rate is cached at `prepare()`, juce_ModeBank.h:41 + ModeBank.cpp:20). Implementation matches actual API: `modeBank.setFundamental(f_final)`. **Non-blocking — plan typo, not design defect.**
- **D-exec-2 (clangd advisory pre-existing):** clangd reports `unused-includes` on `BassoonSound.h` + `BassoonVoice.h` in `PluginProcessor.h`. Both ARE used in `PluginProcessor.cpp` (`new BassoonSound()`, `new BassoonVoice()`, `dynamic_cast<BassoonVoice*>`). Header-only forward declarations would suffice but match Phase 2.1/2.2 pattern (existing convention). **Non-regression — pre-existing pattern; not introduced by Phase 2.3.**

## Files Touched at Execute-Phase

```
?? plugins/O-Bassoon/Source/Vibrato.h
?? plugins/O-Bassoon/Source/Vibrato.cpp
?? plugins/O-Bassoon/Source/NoiseExciter.h
?? plugins/O-Bassoon/Source/NoiseExciter.cpp
M  plugins/O-Bassoon/Source/BassoonVoice.h
M  plugins/O-Bassoon/Source/BassoonVoice.cpp
M  plugins/O-Bassoon/Source/PluginProcessor.h
M  plugins/O-Bassoon/Source/PluginProcessor.cpp
M  plugins/O-Bassoon/CMakeLists.txt
M  plugins/O-Bassoon/.planning/research/ARCHITECTURE.md
M  plugins/O-Bassoon/.planning/stages/2-dsp/SUMMARY.md     (this addendum)
```

**Locked atomic-commit subject** (CONTEXT-rev-3 Q4-rev-3 batch 2): `feat(O-Bassoon): Phase 2.3 expression - Gate 3 PASS`. Commit lands at verify-phase only on Gate 3 PASS green.

## Deferred to User Verification Step (Manual Gate 3)

10-item Logic-AU checklist (PLAN-rev-3 Task 10):

1. ADSR attack 0→2000 ms sweep (audibly different slopes; no clicks)
2. ADSR release 0→3000 ms sweep (audibly different tails; no clicks)
3. Breath UI sweep 0→1 (audible level modulation; no zipper; mute at 0)
4. CC2 real-time loudness (CC2 controller tracks; CC2=0 mutes; UI ignored within 500 ms after CC2 activity)
5. Vibrato 5 Hz / 50 cents at `vibrato_onset = 0` — instant audible (Logic Tuner verifiable ±50 cents)
6. Vibrato `vibrato_onset = 1000 ms` fade-in measurable (~1 s smooth ramp)
7. Vibrato `vibrato_onset = 0` instant + per-voice phase stagger across C3/C4/C5 succession (random phase O-Wind precedent)
8. **60 s held C3 + vibrato + breath QUAL-02 final gate** — bounce `phase-2.3-60s-c3-vibrato-breath.wav` (16-bit / 44.1 kHz stereo); Python `numpy.isfinite True`; 1-s-window RMS drift < 1 dB; Logic Process bar drift < 2 %; ear-stable
9. `output_gain` -24 dB → +6 dB sweep — smooth declick, no zipper, no clipping at +6 dB
10. 8-voice + vibrato + breath CPU < 20 % (Logic Process bar; locked 8-note chord C3+E3+G3+Bb3+C4+E4+G4+Bb4)

Plus VERIFICATION-rev-3.md write (item 11) with results table mapping items 1-10 to PASS/PARTIAL/DEVIATION + regression confirmation that Phase 2.1/2.2 invariants still hold + final `BASE_NOISE_GAIN` value as-shipped.

## Next

Verify phase: `/plugin-verify O-Bassoon 2-dsp` — consumes this SUMMARY + PLAN-rev-3, prompts for the 10-item Gate 3 evidence in Logic-AU, writes VERIFICATION-rev-3, and lands the atomic commit on Gate 3 PASS green.

---

# O-Bassoon Stage 2 / Phase 2.4 — Execution Summary

**Phase:** 2.4 — Voice Manager + Attack Character + Note Expression Integration
**Execute-phase completed:** 2026-04-29
**Inputs:** PLAN-rev-4 (12 tasks, single Wave) + RESEARCH-rev-4 §3 implementation skeletons (lifted verbatim) + ARCHITECTURE.md rev-4 backfill template
**Working tree start:** Phase 2.3 atomic commit `0a64b77` on `main`

## What Was Built

### New Source Files

- `Source/BassoonSynthesiser.{h,cpp}` (NEW) — voice manager subclass with active-cap
  - Header-inline `BassoonSynthesiser : public juce::Synthesiser`
  - Ctor: `setNoteStealingEnabled(true)` (explicit-for-clarity)
  - `setActiveVoiceCap(int)` clamps via `juce::jlimit(1, 16, cap)`; `getActiveVoiceCap() const`
  - Override `findFreeVoice` (const, base virtual signature): manual active-voice loop via `getNumVoices() + getVoice(i)->isVoiceActive()` (no `getNumActiveVoices` in JUCE 8.0.4 per OQ#9-rev-4); if `active < cap` delegates to `juce::Synthesiser::findFreeVoice`; else if `stealIfNoneAvailable` returns `findVoiceToSteal` (JUCE-default release-tail-first then oldest-noteOn — juce_Synthesiser.cpp:525-594); else `nullptr`
  - `.cpp` translation-unit pair contains a single placeholder symbol `touchBassoonSynthesiserTU()` (D1-rev-4 — avoids "no symbols" linker warnings per Phase 2.3 .h+.cpp pair convention)

### Modified Source Files

| Op  | Path | Change |
|-----|------|--------|
| MOD | `Source/Exciter.h` | Renamed Phase 2.1 `onsetBuffer` → `softShape` (D3-rev-4); added `tonguedShape` array + `TONGUED_DURATION_MS = 7.5f` + `VELOCITY_BIAS_MAGNITUDE = 0.3f` (OQ#3/4-rev-4); added `startOnset(attackChar01, velocity01) noexcept` (snapshot-and-latch design — `effectiveAttackChar = clamp(attackChar + (vel-0.5)*0.3, 0, 1)`, lifetime of onset window per risk #2 mitigation); retained `start()` as thin wrapper for D6-rev-4 backwards-compat; `getNextSample()` returns `juce::jmap(effectiveAttackChar, softShape[i], tonguedShape[i])`. |
| MOD | `Source/Exciter.cpp` | Replaced single-shape generation with two-pass: softShape (Phase 2.1 5 ms half-sine × exp, peak-normalised, renamed) + tonguedShape (NEW 7.5 ms `juce::Random rng(12345)` × exp-decay over 4 time-constants, peak-normalised). `onsetSamples = std::max(softN, tonguedN)`; std::array zero-init pads the shorter array (D2-rev-4). |
| MOD | `Source/BassoonVoice.cpp::startNote` | Replaced 3-line plain MIDI freq block with 9-line compose chain: `tuningEngine->getFrequency` (or `MidiMessage::getMidiNoteInHertz` fallback) → `Ouaricon::NoteExpression::applyPendingTuning` (if `pendingTuningSource` non-null) → `static_cast<float>(f_double)`. O-Lyrica `HarpSynthVoice.cpp:113-147` precedent (OQ#6-rev-4). Replaced `exciter.start()` with `parameters->getRawParameterValue("attack_character")->load()` + `exciter.startOnset(attackChar, velocity)` (DSP-05 dispatch site). |
| MOD | `Source/BassoonVoice.cpp::renderNextBlock` | Per-sample loop: split single excitation source into `noiseSample = noiseExciter.getNextSample(breath)` + `exciterSample = exciter.getNextSample()` (auto-zeros after onset window) + `excitation = noiseSample + exciterSample` (OQ#8-rev-4 additive composition). |
| MOD | `Source/PluginProcessor.h` | Added `#include "BassoonSynthesiser.h"`; type-swap `juce::Synthesiser synthesiser` → `BassoonSynthesiser synthesiser` (single-line change; member name preserved); added `int lastDispatchedVoiceCount = -1` (sentinel forces first-block dispatch — OQ#2-rev-4). |
| MOD | `Source/PluginProcessor.cpp` | Inserted 8-line voice_count snapshot at processBlock prologue head, BEFORE tone-dispatch (OQ#2-rev-4 site lock): `requestedVoices = parameters.getRawParameterValue("voice_count")->load()` + integer-comparison throttle + `synthesiser.setActiveVoiceCap(requestedVoices)`. |
| MOD | `CMakeLists.txt` | `target_sources` +2 entries (`BassoonSynthesiser.h` + `BassoonSynthesiser.cpp`) inserted between `BassoonVoice.cpp` and `ModeBank.h`. Build flags + `juce_generate_juce_header` ordering + `ouaricon_add_module(O-Bassoon note-expression)` + `JUCE_USE_WIN_WEBVIEW2_WITH_STATIC_LINKING=1` untouched. |
| MOD | `.planning/research/ARCHITECTURE.md` | Appended rev-4 as-shipped note: 6 subsections (voice manager + attack-character morph + f_base compose chain + NoiseExciter additive + MPE per-channel routing confirmation + regression invariants list) + augmented ordering invariant (8 steps including Phase 2.4 voice_count snapshot at step 3). |

## Build + Static-Check Gate Status

| # | Gate | Status |
|---|------|--------|
| Build | `cmake --build build --target O-Bassoon_VST3 O-Bassoon_AU O-Bassoon_Standalone --parallel` | ✅ **SUCCESS** — clean rebuild, 12/12 targets, ad-hoc signed |
| Install | AU cache cleared + VST3/AU installed fresh per CLAUDE.md protocol | ✅ DONE |
| 1 | RT-safety grep across 6 Phase 2.4 source files | ✅ PASS — 3 hits all benign (1 English-comment "new", 2 construction-time editor/processor factories at PluginProcessor.cpp:279/304) |
| 2 | NE drain BEFORE renderNextBlock | ✅ PASS — `vst3Extensions.drainAndUpdate()` at PluginProcessor.cpp:260, `synthesiser.renderNextBlock` at :263 |
| 3 | Type swap | ✅ PASS — `BassoonSynthesiser synthesiser` at PluginProcessor.h:62; `juce::Synthesiser synthesiser` zero matches |
| 4 | voice_count snapshot site (processBlock prologue head) | ✅ PASS — PluginProcessor.cpp:197-205, BEFORE tone-dispatch at :211 |
| 5 | applyPendingTuning call site | ✅ PASS — BassoonVoice.cpp:65 (inside startNote) |
| 6 | tuningEngine->getFrequency call site | ✅ PASS — BassoonVoice.cpp:61 (inside startNote) |
| 7 | exciter.startOnset call site | ✅ PASS — BassoonVoice.cpp:77 (inside startNote) |
| 8 | Additive composition (exciter + noiseExciter at renderNextBlock) | ✅ PASS — BassoonVoice.cpp:240-241 |
| 9 | DSP-07 (no O-Reed dependency) | ✅ PASS — zero matches in `Source/` |
| 10 | 1/8 scaler retention (Phase 2.2) | ✅ PASS — ModeBank.cpp:114 `1.0f / 8.0f` |
| 11 | Throttle epsilon `0.001f` count | ✅ PASS — exactly 10 hits in PluginProcessor.cpp (≥10 expected) |
| 12 | setExpression site | ✅ PASS — 2 hits (1 dispatch call at :236 + 1 comment at :215, Phase 2.3 carry-forward) |
| 13 | applyGainRamp form (AFTER renderNextBlock) | ✅ PASS — `applyGainRamp(0, numSamples, ...)` at :273, AFTER `renderNextBlock` at :263 |
| 14 | modeBank.setFundamental cadence | ✅ PASS — 3 sites: startNote (:71), pitchWheelMoved (:136), renderNextBlock per-sample throttled (:232) |
| 15 | onsetBuffer rename grep clean | ✅ PASS — only 2 documentation comment references (Exciter.h:67 + Exciter.cpp:16); zero functional uses |
| 16 | softShape + tonguedShape both present | ✅ PASS — Exciter.h has 6 ranges, Exciter.cpp has 11 ranges |
| auval | `auval -v aumu OBsn OuDv` | ✅ PASS — `AU VALIDATION SUCCEEDED.` |
| pluginval-5 | `pluginval --strictness-level 5 --validate ~/.../O-Bassoon-dev.vst3` | ✅ PASS — exit 0, SUCCESS, output bus 0 in / 2 out |

**16/16 auto static-check gates PASS** (16 grep + auval + pluginval-5 = 18 effective auto-checks).

## Manual Gate 4 Bar (handed to user — Task 9 of PLAN-rev-4)

10-item Logic-AU verification checklist. **Items 1–6, 9, 10 must PASS clean; item 7 PASS or documented Stage 4 deferral; item 8 PASS via synthetic fixture.**

1. **8 simultaneous notes audibly distinct** — Logic-AU, hold C2/E2/G2/Bb2/C3/E3/G3/Bb3, confirm 8 voices ring distinctly.
2. **Voice cap + stealing** — `voice_count = 3`, play 4 sequential notes; only 3 sound; oldest (or release-tail-first) is stolen.
3. **Rapid retrigger** — 10 Hz alternating noteOn/noteOff × 30 s with `voice_count = 1`. No stuck notes.
4. **Attack-character soft + low velocity** — `attack_character = 0.0`, vel ≈ 20, play C3. Audibly soft.
5. **Attack-character tongued + high velocity** — `attack_character = 1.0`, vel ≈ 120, play C3. Audibly percussive.
6. **Mid-morph** — `attack_character = 0.5`, vel ≈ 70, play C3. Smooth blend.
7. **MPE per-channel pitch-bend** — Bitwig + MPE controller; bend one note in a chord. **OR** documented Stage 4 deferral (OQ#10-rev-4 fallback).
8. **VST3 NE pitch event** — synthetic test fixture (debug TextButton writing `pendingTuningSource[60] = +50 c`). Must remove fixture before atomic commit (Task 11).
9. **QUAL-02 60 s long-tone** — Logic-AU bounce of held C3 + vibrato 5 Hz / 50 c + breath 0.7 + `attack_character = 0.5`. Python `numpy.isfinite` True; 1-s-window RMS drift < 0.5 dB; CPU drift < 2 % steady-state.
10. **PERF-02 8-voice CPU** — `voice_count = 8`, 8-note chord C2-Bb3 + vibrato + breath + `attack_character = 0.5`. Process bar < 25 % steady-state.

**Iteration ceiling:** rev-3 (CONTEXT-rev-4 Q3-rev-4 batch 2) per Phase 2.2/2.3 precedent. In-cycle adjustments allowed: velocity-bias bracket [0.2f, 0.4f]; tonguedShape decay bracket [5 ms, 10 ms]; softShape extension to 30 ms with LP filter.

## Files for Atomic Commit (Task 11 of PLAN-rev-4)

```
?? plugins/O-Bassoon/Source/BassoonSynthesiser.h
?? plugins/O-Bassoon/Source/BassoonSynthesiser.cpp
M  plugins/O-Bassoon/Source/Exciter.h
M  plugins/O-Bassoon/Source/Exciter.cpp
M  plugins/O-Bassoon/Source/BassoonVoice.cpp
M  plugins/O-Bassoon/Source/PluginProcessor.h
M  plugins/O-Bassoon/Source/PluginProcessor.cpp
M  plugins/O-Bassoon/CMakeLists.txt
M  plugins/O-Bassoon/.planning/research/ARCHITECTURE.md
M  plugins/O-Bassoon/.planning/STATUS.md                  (rev-4)
M  plugins/O-Bassoon/.planning/REQUIREMENTS.md            (rev-4)
M  plugins/O-Bassoon/.planning/stages/2-dsp/SUMMARY.md    (this addendum)
M  plugins/O-Bassoon/.planning/stages/2-dsp/VERIFICATION.md (rev-4 — written at verify-phase)
?? plugins/O-Bassoon/research/reference-recordings/phase-2.4-{60s-c3,8voice-cpu,attack-character-AB}.{wav,png} (optional verification artefacts)
```

**Locked atomic-commit subject** (CONTEXT-rev-4 Q4-rev-4 batch 2): `feat(O-Bassoon): Phase 2.4 polyphony + NE/MPE + attack-character - Gate 4 PASS`. Commit lands at verify-phase only on Gate 4 PASS green.

## Deferred to User Verification Step

- Logic-AU manual checklist items 1–6, 9, 10 (above) — user runs in their DAW
- Item 7 — MPE controller test or documented Stage 4 deferral
- Item 8 — synthetic test fixture (debug TextButton) for VST3 NE pitch event verification, removed before commit
- Optional verification artefacts: phase-2.4-{60s-c3,8voice-cpu,attack-character-AB}.{wav,png}
- VERIFICATION-rev-4.md write
- Atomic commit (verify-phase, on Gate 4 PASS green)

## Next

Verify phase: `/plugin-verify O-Bassoon 2-dsp` — consumes this SUMMARY + PLAN-rev-4, prompts for the 10-item Gate 4 evidence in Logic-AU, writes VERIFICATION-rev-4, and lands the atomic commit on Gate 4 PASS green. **Closes Stage 2** (FUNC-02, FUNC-05, DSP-05, DSP-06, PERF-02, QUAL-02 → complete).
