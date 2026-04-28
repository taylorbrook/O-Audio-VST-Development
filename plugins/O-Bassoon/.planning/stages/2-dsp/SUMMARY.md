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
