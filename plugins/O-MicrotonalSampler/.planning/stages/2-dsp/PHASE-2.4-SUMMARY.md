---
title: "O-MicrotonalSampler Phase 2.4 — Implementation Summary"
created: 2026-04-27
stage: 2-dsp
phase: 2.4
status: gate_4_pass
commit_sha: pending
---

# Phase 2.4 — Implementation Summary

## Status

**Phase 2.4 Gate 4 PASS** — code complete + automated gate green.

Triple build (VST3 + AU + Standalone) green via `ninja`. Cache-clearing
fresh install completed per CLAUDE.md protocol. `pluginval --strictness 5
--validate-in-process --skip-gui-tests` reports `SUCCESS`. `auval -v aumu
OMtS OuDv` reports `AU VALIDATION SUCCEEDED`. All gates passed at the
orchestrator level (main session ran the shell pipeline; the dsp-agent did
the code edits). Awaiting atomic commit by user.

Subjective DAW checks (16-voice steal, ADSR release=0 click test, polyphony
reduction EC-9) are deferred to `/plugin-verify` time, consistent with the
Phase 2.3 protocol.

## Tasks Completed (code level)

| # | Task | Files Touched | Status |
|---|------|---------------|--------|
| 21 | Steal-tail buffer members added to `MicrotonalSamplerVoice.h` — `stealTailBufferL`, `stealTailBufferR`, `stealTailSamplesRemaining`, `kMaxStealRamp`, plus private `renderTailRamp(int)` declaration. `<vector>` include added. | `Source/MicrotonalSamplerVoice.h` | DONE |
| 22 | `prepareToPlay` extended — `kMaxStealRamp = ceil(0.005 * sampleRate) + 16`; both buffers `assign`-zero-initialized to that size; `stealTailSamplesRemaining = 0`. Message-thread alloc only (PERF-01). | `Source/MicrotonalSamplerVoice.cpp` | DONE |
| 23 | `renderTailRamp(int rampSamples)` private helper added before `startNote`. Mirrors `renderNextBlock`'s dual-slot read pattern (deliberate duplication — keeps Phase 2.3 verified path untouched). One-sample `adsr.getNextSample()` snapshot for `lastEnv`; linear ramp `lastEnv * (1 - i/N)` baked into mix; per-slot EC-4 hold honored. Defensive guards for `slotLow == nullptr`, empty buffers, degenerate slots. Allocation-free, lock-free, deterministic. `noexcept`. | `Source/MicrotonalSamplerVoice.cpp` | DONE |
| 24 | Voice-steal detection wired at top of `startNote` (Section 0, BEFORE Section 1's SampleMap snapshot). `adsr.isActive() && slotLow != nullptr` → clamp `rampSamples = jmin(kMaxStealRamp, buffer.size())` → `renderTailRamp(rampSamples)` → `stealTailSamplesRemaining = rampSamples`. Idle-voice branch resets `stealTailSamplesRemaining = 0` defensively. | `Source/MicrotonalSamplerVoice.cpp` | DONE |
| 25 | Steal-tail mix added at top of `renderNextBlock` — AFTER `juce::ScopedNoDenormals` and BEFORE the `slotLow == nullptr || !adsr.isActive()` early-out. `n = jmin(stealTailSamplesRemaining, numSamples)`; `offset = kMaxStealRamp - stealTailSamplesRemaining`; mixes via `out.addFrom` per channel. Defensive bounds check; resets counter on degenerate state. EC-1/EC-2 edge case: tail still plays out even when new-note slot lookup fails (mix runs before early-out). | `Source/MicrotonalSamplerVoice.cpp` | DONE |
| 26 | `synthesiser.setNoteStealingEnabled(true)` verified at `Source/PluginProcessor.cpp:118`. No change needed (R1: JUCE default already correct). | (no edit) | VERIFIED |
| 27 | `stopNote` graceful release verified at `Source/MicrotonalSamplerVoice.cpp:332-346`. No change needed (correct from Phase 2.1). | (no edit) | VERIFIED |
| 28 | Phase 2.4 Gate 4 verification — orchestrator ran triple build (8/8 ninja targets), cache clear + fresh install, `pluginval --strictness 5 --validate-in-process --skip-gui-tests` (SUCCESS), `auval -v aumu OMtS OuDv` (AU VALIDATION SUCCEEDED). Atomic commit deferred to user. | (build artefacts) | DONE |

## Geometry Note (linear-down ramp behavior)

The ramp formula `lastEnv * (1.0f - (float) i / (float) rampSamples)` produces:

| i | Value | Notes |
|---|-------|-------|
| 0 | `lastEnv` (≈ 1.0 at sustain) | First sample full envelope |
| rampSamples / 2 | `lastEnv * 0.5` | Mid-point |
| rampSamples - 1 | `lastEnv * (1/rampSamples)` | Last non-zero step |
| rampSamples (not reached) | 0.0 | Exclusive endpoint |

At 48 kHz, `rampSamples = ceil(0.005 * 48000) + 16 = 240 + 16 = 256`.
At 44.1 kHz, `rampSamples = ceil(0.005 * 44100) + 16 = 221 + 16 = 237`.
At 96 kHz, `rampSamples = ceil(0.005 * 96000) + 16 = 480 + 16 = 496`.

The 16-sample safety margin guarantees the buffer is never undersized when
the ceiling rounds tightly.

## Real-Time Safety Audit

- **`processBlock` allocation-free**: no new heap touch in audio path.
- **`renderNextBlock` allocation-free**: tail mix uses `out.addFrom` (memcpy-style); no `new` / `make_shared` / `vector::push_back`. `juce::ScopedNoDenormals` preserved.
- **`renderTailRamp` allocation-free**: writes to pre-allocated `stealTailBufferL/R`; same `cubicInterp` inline reads as `renderNextBlock`. No heap allocs, no locks. Marked `noexcept`.
- **`startNote` allocation cost**: unchanged from Phase 2.3 — only `currentMap = std::atomic_load(...)` (one refcount inc); `renderTailRamp` adds zero allocs (writes into pre-allocated buffers).
- **Lock-free**: no mutex / `juce::CriticalSection` / `std::lock_guard` introduced.
- **TSan-clean**: no inter-thread reads/writes added (steal-tail buffers are per-voice, only touched from the audio thread when the voice's `startNote` and `renderNextBlock` are invoked).

## Files Touched (Phase 2.4 only)

| File | Action |
|------|--------|
| `Source/MicrotonalSamplerVoice.h` | Modified — added `<vector>` include; updated docstring; added `stealTailBufferL`/`stealTailBufferR`/`stealTailSamplesRemaining`/`kMaxStealRamp` member fields; declared private `renderTailRamp(int) noexcept`. |
| `Source/MicrotonalSamplerVoice.cpp` | Modified — extended `prepareToPlay` to size and zero-init steal buffers; added `renderTailRamp` definition before `startNote`; added Section 0 steal detection at top of `startNote`; added tail-mix block at top of `renderNextBlock` (before early-out); updated docstring header for Phase 2.4. |
| `.planning/stages/2-dsp/PHASE-2.4-SUMMARY.md` | Created (this file). |
| `.planning/STATUS.md` | Updated resume point to Phase 2.4 code-complete. |

**No source files outside that list touched** (`PluginProcessor.cpp`,
`PluginProcessor.h`, `PluginEditor`, `SampleLoader`, `SampleMap`,
`FilenameParser`, `CMakeLists.txt`, `MicrotonalSamplerSound.h` — unchanged).
No new module dependencies. No CMake changes.

## Items Deferred to /plugin-verify (subjective DAW + pluginval audits)

Per the project's pattern, the DSP agent does not exercise these
autonomously. Becomes Gate 4 acceptance items for the user to verify
before declaring Phase 2.4 closed:

1. **Triple build** (VST3 + AU + Standalone) — green in Release config.
2. **Cache-clearing install** per CLAUDE.md (kill `AudioComponentRegistrar`, clear `~/Library/Caches/AudioUnit*`, fresh-install `.vst3` + `.component`).
3. **`pluginval --strictness 5`** clean — including allocation guard (zero allocs in `processBlock`).
4. **`auval -v aumu OMtS OuDv`** — `AU VALIDATION SUCCEEDED`.
5. **Subjective DAW test (Standalone or DAW)** — load a SampleMap and:
   - **16-voice steal:** play 17 notes in rapid succession; verify the 17th note steals cleanly without audible click at the transition.
   - **ADSR release=0 click test:** set release to 0 (or near-0) so the previous note's envelope is mid-attack/sustain when stolen; verify the 5 ms tail ramp prevents discontinuity. Without the ramp, this would click; with it, transition should be inaudible or at most a soft fade.
   - **Polyphony reduction EC-9:** rapid alternation between two notes on the same voice (held legato) — voice-steal path triggers each time; should be glitch-free.
   - **Continuity check:** sustained chord with one note replaced mid-flight; the unaffected voices should not be disturbed; only the stolen voice fades.

If any subjective check fails, file a defect and pause Phase 2.5 entry.

## Recipe for User to Close Gate 4

```bash
# Build (project root)
cd /Users/taylorbrook/Dev/VST-development/build
ninja O-MicrotonalSampler_VST3 O-MicrotonalSampler_AU O-MicrotonalSampler_Standalone

# Cache clear + fresh install (per CLAUDE.md)
killall -9 AudioComponentRegistrar 2>/dev/null || true
rm -rf ~/Library/Caches/AudioUnitCache/
rm -rf ~/Library/Caches/com.apple.audiounits.cache
rm -rf ~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler-dev.vst3
rm -rf ~/Library/Audio/Plug-Ins/Components/O-MicrotonalSampler-dev.component
cp -R /Users/taylorbrook/Dev/VST-development/build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_artefacts/Release/VST3/O-MicrotonalSampler-dev.vst3 ~/Library/Audio/Plug-Ins/VST3/
cp -R /Users/taylorbrook/Dev/VST-development/build/plugins/O-MicrotonalSampler/O-MicrotonalSampler_artefacts/Release/AU/O-MicrotonalSampler-dev.component ~/Library/Audio/Plug-Ins/Components/

# pluginval (strictness 5, allocation-guard ON)
pluginval --strictness 5 --validate-in-process --skip-gui-tests --validate ~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler-dev.vst3

# auval
auval -v aumu OMtS OuDv

# Atomic commit (only after build + pluginval + auval green)
cd /Users/taylorbrook/Dev/VST-development
git add plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.h
git add plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp
git add plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PHASE-2.4-SUMMARY.md
git add plugins/O-MicrotonalSampler/.planning/STATUS.md
git commit -m "feat(O-MicrotonalSampler): voice-steal 5ms tail ramp - Phase 2.4 Gate 4 PASS"
```

## Next Phase

**Phase 2.5 — Loop auto-detect + 8-sample boundary crossfade (Tasks 29–33, Gate 5)**

Activates:
- RMS scan (1024 window, latter 60% of sample) + zero-crossing snap (±64) — populates `loopStart` / `loopEnd` per `SampleSlot` at load time (one-time, background thread).
- 8-sample equal-power boundary crossfade in `cubicInterp`'s loop-wrap branch — eliminates click on loop wrap.
- Fallback to one-shot when RMS scan fails to find a stable region.
- Final Stage 2 sub-stage. Closes the 5-stage Stage 2 plan.

Phase 2.5 invocation: `/plugin-execute O-MicrotonalSampler 2-dsp` (after Gate 4 closes).
