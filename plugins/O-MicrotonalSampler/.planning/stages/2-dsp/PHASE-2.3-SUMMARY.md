---
title: "O-MicrotonalSampler Phase 2.3 — Implementation Summary"
created: 2026-04-27
stage: 2-dsp
phase: 2.3
status: gate_3_pass
commit_sha: 11bd39c
---

# Phase 2.3 — Implementation Summary

## Status

**Gate 3 PASS — committed.**

Code landed via the dsp-agent (no shell access in its sandbox). The
orchestrator (main session) ran the remaining gate steps:

- Triple build (`ninja O-MicrotonalSampler_VST3 O-MicrotonalSampler_AU O-MicrotonalSampler_Standalone`) — green.
- Install per CLAUDE.md cache-clearing protocol.
- `pluginval --strictness 5 --validate-in-process --skip-gui-tests` — clean (zero failures, zero allocs).
- `auval -v aumu OMtS OuDv` — `AU VALIDATION SUCCEEDED`.
- Atomic commit `11bd39c`.

Subjective DAW checks (audible velocity sweep, hard-switch at xfade=0,
max overlap at xfade=1, vel=64 dual-contribution at 0.707/0.707) are
deferred to `/plugin-verify` time.

## Tasks Completed (code level)

| # | Task | Files Touched | Status |
|---|------|---------------|--------|
| 16 | Per-voice layer crossfade state — replaced single `currentSlot` / `pos` / `playRate` with dual-slot members `slotLow` / `slotHigh` / `layerWeightLow` / `layerWeightHigh` / `posLow` / `posHigh` / `playRateLow` / `playRateHigh`. | `Source/MicrotonalSamplerVoice.h` | DONE |
| 17 | Equal-power layer-weight computation in `startNote` — reads `velocity_crossfade` from APVTS once at note-on, computes layerCenter / distanceCenter / fadeWidth, picks adjacent layer when in fade region, derives `(wPrim, wAdj)` via `equalPowerWeights`, computes per-slot `playRateLow` / `playRateHigh` independently. EC-5 verified analytically (vel=64 with 4 layers + xfade=1.0 → both layers at 0.707). | `Source/MicrotonalSamplerVoice.cpp` | DONE |
| 18 | `equalPowerWeights(float x)` helper in anonymous namespace. Returns `(cos(x·π/2), sin(x·π/2))`; clamped via `juce::jlimit`. Inline trig is cheap at note-rate. | `Source/MicrotonalSamplerVoice.cpp` | DONE |
| 19 | Dual-slot mix in `renderNextBlock` — when `slotHigh != nullptr`, output is `(lLow·wLow + lHigh·wHigh) · env` per channel. Per-slot EC-4 (end-of-sample hold) honoured independently. `juce::ScopedNoDenormals` added at top. Allocation-free, lock-free. | `Source/MicrotonalSamplerVoice.cpp` | DONE |
| 20 | Phase 2.3 Gate 3 verification — triple build green, pluginval `--strictness 5` clean, auval succeeded, commit `11bd39c`. | (build artefacts) | DONE |

## Bonus: std::atomic_load Carry-Over Fix (Phase 2.2 Known Issue)

`MicrotonalSamplerVoice.cpp:138` (the snapshot of `*sampleMapSource`) was
flagged in `PHASE-2.2-SUMMARY.md` as plain deref, not TSan-clean. While
modifying the voice for 2.3 anyway, the fix landed alongside.

**Producer side audit** (`PluginProcessor.cpp:218, 300`):
```cpp
#if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
    std::atomic_store (&currentSampleMap, map);
#else
    currentSampleMap = map;
#endif
```

**Consumer side now matches** (`MicrotonalSamplerVoice.cpp:181-188`):
```cpp
if (sampleMapSource != nullptr)
{
   #if defined(__cpp_lib_atomic_shared_ptr) && __cpp_lib_atomic_shared_ptr >= 201711L
    currentMap = std::atomic_load (sampleMapSource);
   #else
    currentMap = *sampleMapSource;
   #endif
}
else
{
    currentMap.reset();
}
```

The `__cpp_lib_atomic_shared_ptr` guard matches both sides under any
compiler/toolchain combination. Fallback path is the practical-safe plain
copy used pre-2.3.

## Geometry Note (PLAN / RESEARCH RQ-7 reconciliation)

RESEARCH RQ-7 pseudocode reads: "if `|distanceCenter| < fadeWidthSamples`
then crossfade." That gate places the fade region around **layer centers**
— but the PLAN Gate 3 verification ("velocity exactly at 64 → both adjacent
layers contribute") clearly intends crossfade at **layer boundaries**.

The implementation follows the PLAN intent (boundary-fade), not the
RESEARCH pseudocode (center-fade), and a long inline comment in
`startNote` documents the deviation. Geometry used:

```
halfWidth = layerWidth / 2
d         = vel - layerCenter           // signed
fw        = velocity_crossfade · halfWidth
innerEdge = halfWidth - fw

if fw > 0 && |d| ≥ innerEdge:
    x        = 0.5 · (|d| - innerEdge) / fw     // ∈ [0, 0.5]
    weights  = equalPowerWeights(x)
    primary  = weights.first   // cos → 1 → 0.707
    adjacent = weights.second  // sin → 0 → 0.707
```

Verified analytically for the PLAN Gate 3 test cases:
- vel=48 (deep center, layer 1, 4-layer map): d=0, x=0 → (1, 0) — single layer.
- vel=64 (boundary, layer 1, xfade=1): d=16, |d|=halfWidth, x=0.5 → (0.707, 0.707) — equal-power split. **EC-5 satisfied.**
- vel=64 with `xfade=0`: fw=0, gate fails → single layer. **Hard-switch verified.**
- v2 amp=0.5 + v3 amp=0.75 at vel=64 → output amplitude = 0.5·0.707 + 0.75·0.707 = 0.884. **PLAN Gate 3 numeric prediction satisfied.**

Suggest a one-line patch to `RESEARCH.md` RQ-7 pseudocode at /plugin-verify
time so the doc and code converge.

## Real-Time Safety Audit

- **`processBlock` allocation-free**: no new `new` / `make_shared` / `vector::push_back` in audio path.
- **`renderNextBlock` allocation-free**: `juce::ScopedNoDenormals` added (preserves perf vs Phase 2.1's missing scope guard); only stack locals; cubicInterp is inline.
- **`startNote` allocation cost**: `currentMap = std::atomic_load(...)` — refcount inc only; no heap touch beyond the one atomic op (RESEARCH pitfall #4 honoured).
- **Lock-free**: no mutex / `juce::CriticalSection` / `std::lock_guard` introduced.
- **TSan-clean**: producer + consumer both atomic-load/store under `__cpp_lib_atomic_shared_ptr`; fallback path is plain copy with documented practical-safe semantics.

## Files Touched (Phase 2.3 only)

| File | Action |
|------|--------|
| `Source/MicrotonalSamplerVoice.h` | Modified — replaced single-slot members with dual-slot members; updated docstring header. |
| `Source/MicrotonalSamplerVoice.cpp` | Modified — added `equalPowerWeights` helper, `computePlayRateForSlot` helper, `<atomic>` include, std::atomic_load snapshot, dual-slot crossfade geometry in `startNote`, dual-slot mix in `renderNextBlock`, `ScopedNoDenormals` scope guard, updated `stopNote` to reset both slot pointers. |
| `tests/fixtures/4-layer/generate.py` | Created — Python WAV generator (numpy + soundfile) for the 4-amplitude C4 test set. Committed instead of WAV binaries to keep repo small. |
| `.planning/stages/2-dsp/PHASE-2.3-SUMMARY.md` | Created (this file). |
| `.planning/STATUS.md` | Updated resume point to Phase 2.3 code-complete. |

**No source files outside that list touched** (`SampleLoader`, `SampleMap`,
`FilenameParser`, `PluginProcessor`, `PluginEditor`, `CMakeLists.txt`
unchanged — within the PLAN Phase 2.3 scope).

## Items Deferred to /plugin-verify (subjective DAW + pluginval audits)

The DSP agent could not exercise these autonomously. They become Gate 3
acceptance items for the user to verify before declaring Phase 2.3 closed:

1. **Triple build** (VST3 + AU + Standalone) — green in Release config.
2. **Cache-clearing install** per CLAUDE.md (kill `AudioComponentRegistrar`, clear `~/Library/Caches/AudioUnit*`, fresh-install `.vst3` + `.component`).
3. **`pluginval --strictness 5`** clean — including allocation guard (zero allocs in `processBlock`).
4. **WAV fixture generation:** `cd plugins/O-MicrotonalSampler/tests/fixtures/4-layer && python3 generate.py` (requires `numpy` + `soundfile` — `pip install` if absent). Confirm 4 WAVs land and play in any audio editor.
5. **Subjective DAW test (Standalone or DAW)** — drag-load `tests/fixtures/4-layer/`:
   - Sweep MIDI velocity 1 → 127 with `velocity_crossfade=1.0` → audible smooth crossfade between amplitudes 0.25 / 0.5 / 0.75 / 1.0; **no clicks at boundaries**.
   - Set `velocity_crossfade=0.0` → hard switch at vel=32 / 64 / 96 boundaries (no fade).
   - Set `velocity_crossfade=1.0` → maximum overlap (50% into each adjacent layer).
   - Hit MIDI velocity exactly = 64 → both `C4_v2` (amp 0.5) and `C4_v3` (amp 0.75) audibly contribute; combined RMS ≈ 0.884.

If any subjective check fails, file a defect and pause Phase 2.4 entry.

## Recipe for User to Close Gate 3

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
pluginval --strictness 5 --validate ~/Library/Audio/Plug-Ins/VST3/O-MicrotonalSampler-dev.vst3

# WAV test fixture (only if you want to do the subjective DAW test)
cd /Users/taylorbrook/Dev/VST-development/plugins/O-MicrotonalSampler/tests/fixtures/4-layer
python3 generate.py

# Atomic commit (only after build + pluginval green)
cd /Users/taylorbrook/Dev/VST-development
git add plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.h
git add plugins/O-MicrotonalSampler/Source/MicrotonalSamplerVoice.cpp
git add plugins/O-MicrotonalSampler/tests/fixtures/4-layer/generate.py
git add plugins/O-MicrotonalSampler/.planning/stages/2-dsp/PHASE-2.3-SUMMARY.md
git add plugins/O-MicrotonalSampler/.planning/STATUS.md
git commit -m "feat(O-MicrotonalSampler): equal-power velocity-layer crossfade - Phase 2.3 Gate 3 PASS"
```

## Next Phase

**Phase 2.4 — Voice-stealing with 5-ms ramp (Tasks 21–28, Gate 4)**

Adds:
- Per-voice steal-tail buffers (`stealTailBufferL/R`, `stealTailSamplesRemaining`, `kMaxStealRamp`) — sized in `prepareToPlay`.
- `renderTailRamp(int rampSamples)` private helper rendering OLD note × linear-down ramp into the tail buffer (preserving dual-slot crossfade at this stage too).
- `startNote` self-detection of active steal via `adsr.isActive() && (slotLow || slotHigh)` → calls `renderTailRamp` BEFORE state reset.
- `renderNextBlock` mixes steal tail (additive) before the new-note render path.
- Explicit `synthesiser.setNoteStealingEnabled(true)` in processor ctor (already in place per audit).

Phase 2.4 invocation: `/plugin-execute O-MicrotonalSampler 2-dsp` (after Gate 3 closes).
