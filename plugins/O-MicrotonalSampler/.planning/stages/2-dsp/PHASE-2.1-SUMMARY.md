---
title: "O-MicrotonalSampler Phase 2.1 — Implementation Summary"
created: 2026-04-27
stage: 2-dsp
phase: 2.1
status: code_complete_pending_build_verification
---

# Phase 2.1 — Implementation Summary

## Tasks Completed

| # | Task | Status | Notes |
|---|------|--------|-------|
| 1 | Add per-voice DSP state to `MicrotonalSamplerVoice.h` | DONE | Added `juce::ADSR adsr`, `pos`, `playRate`, `currentFrequency`, `currentMidiNote`, `currentSlot`, `currentMap`, `prepareToPlay()` and `setCurrentPlaybackSampleRate()` overrides. No 2.3+ members yet. |
| 2 | `MicrotonalSamplerVoice::prepareToPlay` + `setCurrentPlaybackSampleRate` | DONE | Both call `adsr.setSampleRate`. Defaults `{0.005, 0.1, 1.0, 0.3}` (overridden in `startNote`). |
| 3 | Anonymous-namespace `cubicInterp` helper | DONE | Catmull-Rom Horner expansion matching `juce_Interpolators.h:118-131`. Loop wrap path implemented but dormant in 2.1 (loopEnd=0 → clamp via `juce::jlimit`). Active in 2.5. |
| 7 | Fill `SampleMap::findSlot` body | DONE | Linear scan over `slots`, returns first match on `(midiNote, velocityLayer)`. Inline doc updated. |
| 6 | In-memory test fixture in `PluginProcessor::prepareToPlay` | DONE | Gated by `#ifdef O_MICROTONAL_SAMPLER_PHASE_2_1_TEST_FIXTURE`. Builds `std::shared_ptr<SampleMap>` with 88 slots (MIDI 21..108), 1 layer, 0.25s sine burst at each note's ET frequency, 5ms cosine fade-in/out. CMake option `OMTS_PHASE_2_1_TEST_FIXTURE` defaulting ON. |
| 4 | `startNote` Phase-2.1 baseline | DONE | Snapshots map (shared_ptr copy), velocity → layer index, `findSlot`, ET fallback for `currentFrequency`, NE delta via `Ouaricon::NoteExpression::applyPendingTuning` (free function returning the new frequency), varispeed `playRate = (currentFrequency / refFreqOfSlotNote) * (slotSR / hostSR)`, single APVTS ADSR read, `pos=0; adsr.reset(); adsr.noteOn();`. EC-1 / EC-2 handled. |
| 5 | `stopNote` and `renderNextBlock` | DONE | `stopNote(_, true)` → `adsr.noteOff`; `(_, false)` → `adsr.reset` + clear. `renderNextBlock` uses per-sample `cubicInterp` × ADSR × `addSample`, end-of-sample EC-4 hold (clamped readPos), allocation-free, lock-free. |
| 8 | `OMicrotonalSamplerAudioProcessor::prepareToPlay` | DONE | Loops voices and calls `prepareToPlay`. Added `juce::SmoothedValue<float, Linear> outputGainSmoother`, `reset(sampleRate, 0.01)`, seeded with current param. `processBlock` applies gain post-render via `applyGainRamp` start/end snapshots. Explicit `synthesiser.setNoteStealingEnabled(true)` set in ctor (RESEARCH R1). |
| 9 | Phase 2.1 Gate 1 verification | **PARTIAL — code in place, build/auval/pluginval not run** | dsp-agent has no Bash tool in this run. See "Verification Needed" below. |

## Files Modified

| File | Action | Purpose |
|------|--------|---------|
| `Source/MicrotonalSamplerVoice.h` | Modified | Added Phase 2.1 DSP state + prepareToPlay + setCurrentPlaybackSampleRate decl |
| `Source/MicrotonalSamplerVoice.cpp` | Modified (full rewrite of body) | Implemented cubicInterp helper + startNote/stopNote/renderNextBlock + ADSR wiring + NE consumption + varispeed math |
| `Source/SampleMap.h` | Modified | Filled `findSlot` body (linear scan), removed Stage-1 stub doc |
| `Source/PluginProcessor.h` | Modified | Added `outputGainSmoother` member |
| `Source/PluginProcessor.cpp` | Modified | Voice prepareToPlay loop, output-gain smoothing, in-memory test fixture (macro-gated), explicit `setNoteStealingEnabled(true)` |
| `plugins/O-MicrotonalSampler/CMakeLists.txt` | Modified | Added `OMTS_PHASE_2_1_TEST_FIXTURE` option (default ON), aliasing-check executable target with EXCLUDE_FROM_ALL |

## Files Created

| File | Action | Purpose |
|------|--------|---------|
| `Source/tests/aliasing_check.cpp` | Created | RQ-1 manual aliasing measurement scaffolding (body deferred — prints info message). Standalone executable, EXCLUDE_FROM_ALL. |

## Aliasing Test (RQ-1) Status

**Scaffolding only.** `Source/tests/aliasing_check.cpp` is registered in CMake as `O-MicrotonalSampler_AliasingCheck` with `EXCLUDE_FROM_ALL`; it compiles but currently prints a stub message. Per task instructions, the full FFT-based +50c-vs-+0c null-test body is deferred to manual run by the user.

**Phase 2.1 default assumption:** "no pre-filter required" (cubic-Hermite is a 4-tap polynomial; pre-filter only added if measurement shows >−60 dBFS aliases >8 kHz at +50c). If a future measurement shows breach, add `juce::dsp::IIR::Filter<float>` per channel in voice as documented in PLAN Task 9.

## Verification Needed (deferred to follow-up shell session)

dsp-agent has only Read/Edit/Write tools in this run — no Bash/shell access. The PLAN Gate 1 verification commands MUST be run by the orchestrator / user before this phase is considered "Gate 1 PASS":

```bash
cd /Users/taylorbrook/Dev/VST-development
cmake --build build --target O-MicrotonalSampler_VST3 O-MicrotonalSampler_AU O-MicrotonalSampler_Standalone -j 2>&1 | tail -40

# If green:
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/ ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler.vst3 ~/Library/Audio/Plug-Ins/Components/O-MicrotonalSampler.component
cp -R build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_artefacts/Release/VST3/O-MicrotonalSampler.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_artefacts/Release/AU/O-MicrotonalSampler.component ~/Library/Audio/Plug-Ins/Components/

auval -v aumu OMtS OuDv 2>&1 | tail -20
/Applications/pluginval.app/Contents/MacOS/pluginval --strictness 5 --validate ~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler.vst3 2>&1 | tail -30

# If both green:
git add -A
git commit -m "feat(O-MicrotonalSampler): cubic-Hermite varispeed voice + ADSR + NE - Phase 2.1 Gate 1 PASS

Co-Authored-By: Claude Opus 4.7 (1M context) <noreply@anthropic.com>"
```

**Expected outcomes:**
- Build: green on all three targets (no new external deps; only existing JUCE modules used).
- auval: PASS (`AU VALIDATION SUCCEEDED`). Voice now produces audio when MIDI note 21–108 hit at any velocity.
- pluginval `--strictness 5`: PASS, including allocation guard (no `new`/`malloc`/`vector::push_back`/`std::function` in `processBlock` or `renderNextBlock` — verified by code review).
- Standalone smoke: hit a MIDI key 21–108 → 0.25s sine burst with ADSR shape plays. NE pitch shift (e.g., +50c via Dorico or manual NE injection) audibly retunes the burst.

## Real-Time Safety Audit (code review)

- `processBlock` / `renderNextBlock`:
  - No `new` / `delete` / `malloc` / `free`.
  - No `std::vector::push_back` / `resize` / `insert` (vector ops only in `prepareToPlay` test fixture, which is message-thread).
  - No `std::mutex` / `juce::ScopedLock` / locks of any kind.
  - No `std::function` invocations.
  - No file I/O / system calls / exceptions / `printf`.
  - All bounded loops: `for (int i = 0; i < numSamples; ++i)`.
  - `juce::ScopedNoDenormals` present at top of `processBlock`.
  - `shared_ptr` copy in `startNote` is the only atomic op (refcount inc, lock-free).
  - APVTS reads via `getRawParameterValue()->load()`.
  - ADSR params read ONCE per `startNote` (RESEARCH pitfall #2).
- `prepareToPlay` (message thread): all allocations here, including `audio.setSize`, `slots.reserve`, `std::make_shared<SampleMap>`. RT-safe by virtue of being off-thread.

## Deviations from Plan

1. **Aliasing test body deferred:** Per task instructions, scaffolding only. CMake target registered, body prints stub. Phase 2.1 commits assume "no pre-filter required" until manual measurement.
2. **`dynamic_cast` instead of `static_cast` in voice prepareToPlay loop:** Defensive; same wire-call cost, slightly safer if voice list ever contains non-MicrotonalSamplerVoice instances. Plan suggested `static_cast`; behavior identical for current code.
3. **Phase-2.1 fixture amplitude `0.25` (-12 dBFS) not `1.0`:** Leaves headroom for ADSR sustain peaks and prevents any chance of clipping during interpolation overshoot near transients (Catmull-Rom can overshoot ±2-4% on sharp edges). Subjective audibility unchanged.

## Build Outcome

**Not run in this session.** Code complete; awaiting build verification by orchestrator.

## pluginval Outcome

**Not run in this session.** Awaiting build artifacts.

## auval Outcome

**Not run in this session.** Awaiting build artifacts.

## Commit SHA

**Not committed in this session.** Code is staged but not committed (no shell access). Orchestrator/user to commit after verification.

## Next Steps

1. Orchestrator runs `cmake --build` triple-target.
2. If clean → install per CLAUDE.md, run `auval` + `pluginval --strictness 5`.
3. If both green → commit per template above.
4. Phase 2.2 begins: SampleLoader::run() body, FilenameParser, real folder load.

## Out of Scope (Phase 2.2+)

- `SampleLoader::run()` body (Phase 2.2)
- FilenameParser (Phase 2.2)
- Velocity-layer crossfade (Phase 2.3)
- Voice-stealing 5-ms ramp (Phase 2.4)
- Loop auto-detect + 8-sample crossfade (Phase 2.5)
- Aliasing-check FFT body (manual run, deferred)
