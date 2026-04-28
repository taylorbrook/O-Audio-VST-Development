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
