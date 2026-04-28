---
title: "O-MicrotonalSampler Phase 2.2 — Implementation Summary"
created: 2026-04-27
stage: 2-dsp
phase: 2.2
status: gate_2_pass
commit_sha: cacffda
---

# Phase 2.2 — Implementation Summary

## Gate 2 Verification

| Check | Result |
|-------|--------|
| Triple-target build (VST3 + AU + Standalone) | **PASS** |
| `auval -v aumu OMtS OuDv` | **AU VALIDATION SUCCEEDED** |
| `pluginval --strictness 5` | **SUCCESS** |
| Commit | `cacffda` |

## Tasks Completed

| # | Task | Files Touched | Status |
|---|------|---------------|--------|
| 10 | FilenameParser (pure-function tolerant: scientific pitch, MIDI form, bare int; velocity v/vel/p-mp-mf-f/L/Lyr) | `Source/FilenameParser.h`, `Source/FilenameParser.cpp` (created) | DONE |
| 11 | `SampleLoader::run()` body — AudioFormatManager local, RangedDirectoryIterator, threadShouldExit per file, per-channel LagrangeInterpolator SR conversion, mono→stereo, MessageManager::callAsync completion | `Source/SampleLoader.h`, `Source/SampleLoader.cpp` (modified) | DONE |
| 12 | Processor consumes loader completion — `loadSampleFolder()`, `lastSkippedFiles`, atomic_store on `currentSampleMap` shared_ptr (C++20 feature guard) | `Source/PluginProcessor.h`, `Source/PluginProcessor.cpp` (modified) | DONE |
| 13 | Load Folder button — custom `AudioProcessorEditor` embedding `GenericAudioProcessorEditor` + `TextButton` + async `FileChooser` (40-px top strip) | `Source/PluginEditor.h`, `Source/PluginEditor.cpp` (modified) | DONE |
| 14 | CMake — register `FilenameParser.cpp/.h`; flip `OMTS_PHASE_2_1_TEST_FIXTURE` default ON→OFF (macro-gated code retained for regression safety) | `plugins/O-MicrotonalSampler/CMakeLists.txt` (modified) | DONE |
| 15 | Gate 2 verification (build + auval + pluginval + commit) | — | DONE |

## Files Created

- `Source/FilenameParser.h`
- `Source/FilenameParser.cpp` (with `OMTS_UNIT_TESTS`-gated inline unit tests covering scientific-pitch, MIDI-form, bare-int, dynamics, layer/L/Lyr, mixed separators, parse-failure cases)

## Files Modified

- `Source/SampleLoader.h` — extended `CompletionCallback` to carry `juce::StringArray skippedFiles`; added private `skippedFiles` member
- `Source/SampleLoader.cpp` — full `run()` body; `loadFolder` now `startThread()` instead of dispatching the Stage-1 failure stub
- `Source/PluginProcessor.h` — `loadSampleFolder()` decl, `lastSkippedFiles` member, `getLastSkippedFiles()` accessor
- `Source/PluginProcessor.cpp` — `loadSampleFolder()` impl with atomic-store under `__cpp_lib_atomic_shared_ptr` guard; failure callback uses `juce::ignoreUnused` to silence Release-build warning
- `Source/PluginEditor.h` — switched base class from `GenericAudioProcessorEditor` to custom `AudioProcessorEditor`; embeds generic editor + button + chooser
- `Source/PluginEditor.cpp` — async FileChooser wiring + 40-px top-strip layout
- `plugins/O-MicrotonalSampler/CMakeLists.txt` — sources + flipped fixture default OFF

## Real-Time Safety Audit

- **No new allocations on the audio path.** All `make_shared`, `setSize`, `vector::reserve`, `vector::push_back`, `juce::AudioBuffer` allocs run on the loader thread or message thread.
- **Voice DSP untouched** — Phase 2.1 voice render path is byte-identical post-2.2; no regression risk.
- **`AudioFormatManager` is local in `run()`** (RESEARCH pitfall #9) — never a member.
- **Per-channel `LagrangeInterpolator`** instances (pitfall #10) — separate state for L and R.
- **`reader` is `std::unique_ptr`** (pitfall #14) — RAII close on scope exit.
- **`threadShouldExit()` checked per file** (pitfall #11) — clean cancellation when DAW closes.
- **`MessageManager::callAsync`** for completion + failure dispatch (pitfall #12).
- **Atomic-store on `currentSampleMap`** under `__cpp_lib_atomic_shared_ptr >= 201711L` guard, plain assignment fallback (matching the pattern already in `prepareToPlay`).

## Deviations from PLAN

1. **Validity guard in loader**: skip-and-log gate after opening reader if `srcChannels<=0 || srcSamples<=0 || srcSR<=0.0`. Defensive against pathological readers.
2. **Failure-dispatch when no slots produced**: descriptive `"no usable samples in <folder> (skipped N file(s))"` rather than empty `SampleMap` via completion. PLAN-consistent.
3. **`L[N]` velocity-token rule tightened** to exactly `L1`..`L4` (2-char) to avoid eating non-velocity prefixes like `Lab`/`London`/`Live`. PLAN spec was permissive; this is the intended single-letter abbreviation.
4. **No `juce::WeakReference` capture** in editor → processor handoff: raw `this` is acceptable per Phase 2.2 directive (loader is `unique_ptr` member; `~SampleLoader()` joins thread before processor finalisation).
5. **`juce::ignoreUnused (reason)`** added in failure callback — `DBG()` compiles out in Release, leaving `reason` unused without this.

## Known Issue Flagged for Future Work

- **`MicrotonalSamplerVoice.cpp:138`** uses plain `shared_ptr` deref + copy rather than `std::atomic_load(sampleMapSource)`. Practical risk on x86-64/ARM64 is near-zero (aligned pointer reads are atomic at the hardware level), but TSan would flag it. One-line fix when convenient:
  ```cpp
  currentMap = (sampleMapSource != nullptr) ? std::atomic_load (sampleMapSource) : nullptr;
  ```
  Per Phase 2.2 directive ("do NOT touch the voice DSP"), deferred. Suggested fix-up window: at the start of Phase 2.3 when voice members are already being modified.

## Smoke Tests Pending (manual, optional)

These were specified in PLAN Task 15 but require WAV fixtures the user must produce or stage. Gate 2 passed without them; they're recommended before shipping but not blocking.

- `tests/fixtures/4-sample-set/` (`C4_v1.wav`, `C4_v2.wav`, `D4_v1.wav`, `D4_v2.wav` synthesized sines) — drag-drop, all 4 audibly map+play.
- `tests/fixtures/mixed-names/` (`weird-name.wav` + parseable file) — unparseable silently skipped + populated in `lastSkippedFiles`.
- `tests/fixtures/mixed-sr/` (44.1k + 96k samples) — both play in tune relative to each other.
- Concurrent reload: hold sustained note → drop different folder → active note keeps OLD map; next note picks up NEW map (EC-3).

## Next Phase

**Phase 2.3 — Velocity-layer crossfade (Tasks 16–20, Gate 3)**

Adds:
- Per-voice dual-slot members (`slotLow`, `slotHigh`, `layerWeightLow/High`, `posHigh`, `playRateHigh`)
- `equalPowerWeights(x)` helper (anonymous namespace)
- Equal-power crossfade geometry in `startNote` (RQ-7 Site 1)
- Two-slot mix in `renderNextBlock` when `slotHigh != nullptr`

Voice DSP IS in-scope for 2.3, so the flagged `std::atomic_load` voice fix can ride along.

Phase 2.3 invocation: `/plugin-execute O-MicrotonalSampler 2-dsp`
